#include "rlgame.base/cvulkan/cvulkan_buffer.h"

#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cvulkan/cvulkan_memory_allocator.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_NewBuffer (struct R_CVulkan_Buffer* pBuffer, const struct R_CVulkan_BufferCreateInfo* pCreateInfo)
{
    R_CVULKAN_ASSERT (pBuffer);
    R_CVULKAN_ASSERT (pCreateInfo);
    R_CVULKAN_ASSERT (pCreateInfo->size == 0);

    pBuffer->device = R_CVulkan_DeviceGetLogicalDevice (pCreateInfo->device);
    pBuffer->handle = VK_NULL_HANDLE;
    pBuffer->memory = VK_NULL_HANDLE;
    pBuffer->size = pCreateInfo->size;
    pBuffer->usage = pCreateInfo->usage;
    pBuffer->properties = pCreateInfo->properties;
    pBuffer->pMapped = NULL;

    VkBufferCreateInfo bufferInfo = {0};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = pCreateInfo->size;
    bufferInfo.usage = pCreateInfo->usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer (pBuffer->device, &bufferInfo, NULL, &pBuffer->handle);
    if (result != VK_SUCCESS)
    {
        return R_CVULKAN_ERROR_BUFFER_CREATE_FAILED;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements (pBuffer->device, pBuffer->handle, &memRequirements);

    uint32_t             memoryTypeIndex = 0;
    enum R_CVulkan_Error error = R_CVulkan_FindMemoryType (
        pCreateInfo->physicalDevice,
        &memRequirements,
        pCreateInfo->properties,
        &memoryTypeIndex);
    if (error != R_CVULKAN_OK)
    {
        vkDestroyBuffer (pBuffer->device, pBuffer->handle, NULL);
        pBuffer->handle = VK_NULL_HANDLE;
        return error;
    }

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    result = vkAllocateMemory (pBuffer->device, &allocInfo, NULL, &pBuffer->memory);
    if (result != VK_SUCCESS)
    {
        vkDestroyBuffer (pBuffer->device, pBuffer->handle, NULL);
        pBuffer->handle = VK_NULL_HANDLE;
        return R_CVULKAN_ERROR_MEMORY_ALLOCATE_FAILED;
    }
    result = vkBindBufferMemory (pBuffer->device, pBuffer->handle, pBuffer->memory, 0);
    if (result != VK_SUCCESS)
    {
        vkFreeMemory (pBuffer->device, pBuffer->memory, NULL);
        vkDestroyBuffer (pBuffer->device, pBuffer->handle, NULL);
        pBuffer->memory = VK_NULL_HANDLE;
        pBuffer->handle = VK_NULL_HANDLE;
        return R_CVULKAN_ERROR_FAILED;
    }
    return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_DeleteBuffer (struct R_CVulkan_Buffer* pBuffer)
{
    R_CVULKAN_ASSERT (pBuffer);
    if (pBuffer->pMapped) R_CVulkan_BufferUnmap (pBuffer);
    vkFreeMemory (pBuffer->device, pBuffer->memory, NULL);
    vkDestroyBuffer (pBuffer->device, pBuffer->handle, NULL);
#if defined(R_CVULKAN_DEBUG)
    pBuffer->device = VK_NULL_HANDLE;
    pBuffer->size = 0;
    pBuffer->usage = 0;
    pBuffer->properties = 0;
#endif
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_BufferMap (
    struct R_CVulkan_Buffer* pBuffer,
    VkDeviceSize             offset,
    VkDeviceSize             size,
    void**                   ppOutData)
{
    R_CVULKAN_ASSERT (pBuffer);
    R_CVULKAN_ASSERT (ppOutData);

    if (pBuffer->pMapped != NULL)
    {
        R_CSTL_LOG_WARN ("Buffer has already been mapped again without unmap call, Skipping memory map");
        R_CSTL_LOG_WARN ("Mapped: %p", (void*)pBuffer->pMapped);
        return R_CVULKAN_OK;
    }
    VkResult result = vkMapMemory (pBuffer->device, pBuffer->memory, offset, size, 0, &pBuffer->pMapped);
    if (result != VK_SUCCESS)
    {
        return R_CVULKAN_ERROR_MAP_MEMORY_FAILED;
    }
    *ppOutData = pBuffer->pMapped;
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_BufferUnmap (struct R_CVulkan_Buffer* pBuffer)
{
    R_CVULKAN_ASSERT (pBuffer);

    vkUnmapMemory (pBuffer->device, pBuffer->memory);
    pBuffer->pMapped = NULL;
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_BufferCopyData (
    struct R_CVulkan_Buffer* pBuffer,
    VkDeviceSize             offset,
    VkDeviceSize             size,
    const void*              data)
{
    R_CVULKAN_ASSERT (pBuffer);
    R_CVULKAN_ASSERT (data);

    void*                mapped = NULL;
    enum R_CVulkan_Error error = R_CVulkan_BufferMap (pBuffer, offset, size, &mapped);
    if (error != R_CVULKAN_OK)
    {
        return error;
    }
    memcpy (mapped, data, size);

    error = R_CVulkan_BufferUnmap (pBuffer);
    if (error != R_CVULKAN_OK)
    {
        return error;
    }

    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_BufferInvalidate (struct R_CVulkan_Buffer* pBuffer, VkDeviceSize offset, VkDeviceSize size)
{
    R_CVULKAN_ASSERT (pBuffer);

    VkMappedMemoryRange mappedRange = {0};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = pBuffer->memory;
    mappedRange.offset = offset;
    mappedRange.size = size;

    VkResult result = vkInvalidateMappedMemoryRanges (pBuffer->device, 1, &mappedRange);
    if (result != VK_SUCCESS)
    {
        return R_CVULKAN_ERROR_FAILED;
    }
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_BufferFlush (struct R_CVulkan_Buffer* pBuffer, const VkDeviceSize offset, const VkDeviceSize size)
{
    R_CVULKAN_ASSERT (pBuffer);

    VkMappedMemoryRange mappedRange = {0};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = pBuffer->memory;
    mappedRange.offset = offset;
    mappedRange.size = size;

    VkResult result = vkFlushMappedMemoryRanges (pBuffer->device, 1, &mappedRange);
    if (result != VK_SUCCESS)
    {
        return R_CVULKAN_ERROR_FAILED;
    }

    return R_CVULKAN_OK;
}

R_CVULKAN_API VkBuffer
R_CVulkan_BufferGetHandle (const struct R_CVulkan_Buffer* pBuffer)
{
    R_CVULKAN_ASSERT (pBuffer);
    return pBuffer->handle;
}

R_CVULKAN_API VkDeviceMemory
R_CVulkan_BufferGetMemory (const struct R_CVulkan_Buffer* pBuffer)
{
    R_CVULKAN_ASSERT (pBuffer);
    return pBuffer->memory;
}

R_CVULKAN_API VkDevice
R_CVulkan_BufferGetDevice (const struct R_CVulkan_Buffer* pBuffer)
{
    R_CVULKAN_ASSERT (pBuffer);
    return pBuffer->device;
}

R_CVULKAN_API VkDeviceSize
R_CVulkan_BufferGetSize (const struct R_CVulkan_Buffer* pBuffer)
{
    R_CVULKAN_ASSERT (pBuffer);
    return pBuffer->size;
}

R_CVULKAN_API VkBufferUsageFlags
R_CVulkan_BufferGetUsage (const struct R_CVulkan_Buffer* pBuffer)
{
    R_CVULKAN_ASSERT (pBuffer);
    return pBuffer->usage;
}

R_CVULKAN_API VkMemoryPropertyFlags
R_CVulkan_BufferGetProperties (const struct R_CVulkan_Buffer* pBuffer)
{
    R_CVULKAN_ASSERT (pBuffer);
    return pBuffer->properties;
}

R_CVULKAN_API void*
R_CVulkan_BufferGetMapped (const struct R_CVulkan_Buffer* pBuffer)
{
    R_CVULKAN_ASSERT (pBuffer);
    return pBuffer->pMapped;
}