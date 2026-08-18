#include "rlgame.base/cvulkan/cvulkan_buffer.h"
#include "rlgame.base/cvulkan/cvulkan_memory_allocator.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_NewBuffer (
    struct R_CVulkan_Buffer*       pBuffer,
    const struct R_CVulkan_Device* device,
    VkPhysicalDevice               physicalDevice,
    R_CVulkanDeviceSize            size,
    R_CVulkanBufferUsageFlags      usage,
    R_CVulkanMemoryPropertyFlags   properties)
{
        if (!pBuffer || !device || physicalDevice == VK_NULL_HANDLE)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

#if defined(R_CVULKAN_DEBUG)
        if (!R_CVulkan_DeviceIsInitialized (device))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        if (size == 0)
        {
                return R_CVULKAN_ERROR_INVALID_ARGUMENT;
        }

        pBuffer->device = R_CVulkan_DeviceGetLogicalDevice (device);
        pBuffer->handle = VK_NULL_HANDLE;
        pBuffer->memory = VK_NULL_HANDLE;
        pBuffer->size = size;
        pBuffer->usage = usage;
        pBuffer->properties = properties;
        pBuffer->pMapped = NULL;
        pBuffer->isMapped = 0;
#if defined(R_CVULKAN_DEBUG)
        pBuffer->isInitialized = false;
#endif

        VkBufferCreateInfo bufferInfo = {0};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateBuffer (pBuffer->device, &bufferInfo, NULL, &pBuffer->handle);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_BUFFER_CREATE_FAILED;
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements (pBuffer->device, pBuffer->handle, &memRequirements);

        uint32_t             memoryTypeIndex = 0;
        enum R_CVulkan_Error error
            = R_CVulkan_FindMemoryType (physicalDevice, &memRequirements, properties, &memoryTypeIndex);
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
#if defined(R_CVULKAN_DEBUG)
        pBuffer->isInitialized = true;
#endif
        return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_DeleteBuffer (struct R_CVulkan_Buffer* pBuffer)
{
#if defined(R_CVULKAN_DEBUG)
        if (!pBuffer)
        {
                return;
        }
#endif

        if (pBuffer->isMapped)
        {
                R_CVulkan_BufferUnmap (pBuffer);
        }

        if (pBuffer->memory != VK_NULL_HANDLE)
        {
                vkFreeMemory (pBuffer->device, pBuffer->memory, NULL);
                pBuffer->memory = VK_NULL_HANDLE;
        }

        if (pBuffer->handle != VK_NULL_HANDLE)
        {
                vkDestroyBuffer (pBuffer->device, pBuffer->handle, NULL);
                pBuffer->handle = VK_NULL_HANDLE;
        }

#if defined(R_CVULKAN_DEBUG)
        pBuffer->isInitialized = false;
#endif
        pBuffer->device = VK_NULL_HANDLE;
        pBuffer->size = 0;
        pBuffer->usage = 0;
        pBuffer->properties = 0;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_BufferMap (
    struct R_CVulkan_Buffer* pBuffer,
    R_CVulkanDeviceSize      offset,
    R_CVulkanDeviceSize      size,
    void**                   ppOutData)
{
        R_CVULKAN_ASSERT (pBuffer != NULL);
        R_CVULKAN_ASSERT (ppOutData != NULL);

        if (!pBuffer || !ppOutData)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pBuffer->isInitialized);
        if (!pBuffer->isInitialized)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        if (pBuffer->isMapped)
        {
                return R_CVULKAN_ERROR_FAILED;
        }

        VkResult result = vkMapMemory (pBuffer->device, pBuffer->memory, offset, size, 0, &pBuffer->pMapped);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_MAP_MEMORY_FAILED;
        }

        pBuffer->isMapped = 1;
        *ppOutData = pBuffer->pMapped;
        return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_BufferUnmap (struct R_CVulkan_Buffer* pBuffer)
{
        R_CVULKAN_ASSERT (pBuffer != NULL);

        if (!pBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pBuffer->isInitialized);
        if (!pBuffer->isInitialized)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        if (!pBuffer->isMapped)
        {
                return R_CVULKAN_ERROR_FAILED;
        }

        vkUnmapMemory (pBuffer->device, pBuffer->memory);
        pBuffer->pMapped = NULL;
        pBuffer->isMapped = 0;

        return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_BufferCopyData (
    struct R_CVulkan_Buffer* pBuffer,
    R_CVulkanDeviceSize      offset,
    R_CVulkanDeviceSize      size,
    const void*              data)
{
        R_CVULKAN_ASSERT (pBuffer != NULL);
        R_CVULKAN_ASSERT (data != NULL);

        if (!pBuffer || !data)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pBuffer->isInitialized);
        if (!pBuffer->isInitialized)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        if (size == 0)
        {
                return R_CVULKAN_ERROR_INVALID_ARGUMENT;
        }

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
R_CVulkan_BufferInvalidate (
    struct R_CVulkan_Buffer* pBuffer,
    R_CVulkanDeviceSize      offset,
    R_CVulkanDeviceSize      size)
{
        R_CVULKAN_ASSERT (pBuffer != NULL);

        if (!pBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pBuffer->isInitialized);
        if (!pBuffer->isInitialized)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

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
R_CVulkan_BufferFlush (struct R_CVulkan_Buffer* pBuffer, R_CVulkanDeviceSize offset, R_CVulkanDeviceSize size)
{
        R_CVULKAN_ASSERT (pBuffer != NULL);

        if (!pBuffer)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pBuffer->isInitialized);
        if (!pBuffer->isInitialized)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

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
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pBuffer != NULL);
#endif
        return pBuffer->handle;
}

R_CVULKAN_API VkDeviceMemory
R_CVulkan_BufferGetMemory (const struct R_CVulkan_Buffer* pBuffer)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pBuffer != NULL);
#endif
        return pBuffer->memory;
}

R_CVULKAN_API VkDevice
R_CVulkan_BufferGetDevice (const struct R_CVulkan_Buffer* pBuffer)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pBuffer != NULL);
#endif
        return pBuffer->device;
}

R_CVULKAN_API R_CVulkanDeviceSize
R_CVulkan_BufferGetSize (const struct R_CVulkan_Buffer* pBuffer)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pBuffer != NULL);
#endif
        return pBuffer->size;
}

R_CVULKAN_API R_CVulkanBufferUsageFlags
R_CVulkan_BufferGetUsage (const struct R_CVulkan_Buffer* pBuffer)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pBuffer != NULL);
#endif
        return pBuffer->usage;
}

R_CVULKAN_API R_CVulkanMemoryPropertyFlags
R_CVulkan_BufferGetProperties (const struct R_CVulkan_Buffer* pBuffer)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pBuffer != NULL);
#endif
        return pBuffer->properties;
}

R_CVULKAN_API void*
R_CVulkan_BufferGetMapped (const struct R_CVulkan_Buffer* pBuffer)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pBuffer != NULL);
#endif
        return pBuffer->pMapped;
}

R_CVULKAN_API int
R_CVulkan_BufferIsMapped (const struct R_CVulkan_Buffer* pBuffer)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pBuffer != NULL);
#endif
        return pBuffer->isMapped;
}

R_CVULKAN_API int
R_CVulkan_BufferIsInitialized (const struct R_CVulkan_Buffer* pBuffer)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pBuffer != NULL);
        return pBuffer->isInitialized;
#else
        (void)pBuffer;
        return 1;
#endif
}
