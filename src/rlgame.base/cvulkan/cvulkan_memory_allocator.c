#include "rlgame.base/cvulkan/cvulkan_memory_allocator.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cvulkan/cvulkan_defragmentation.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#define R_CVULKAN_DEFAULT_MIN_BLOCK_SIZE (256 * 1024) /* 256 KB */
#define R_CVULKAN_DEFAULT_MAX_BLOCK_SIZE (256 * 1024 * 1024) /* 256MB */

static enum R_CVulkanError R_CVulkan_NewMemoryBlock (
    struct R_CVulkan_MemoryBlock* block,
    VkDevice                      device,
    VkPhysicalDevice              physicalDevice,
    VkDeviceSize                  size,
    VkBufferUsageFlags            usage,
    VkMemoryPropertyFlags         properties);

static void R_CVulkan_DeleteMemoryBlock (struct R_CVulkan_MemoryBlock* block);

static int R_CVulkan_MemoryBlockAllocate (
    struct R_CVulkan_MemoryBlock*   block,
    VkDeviceSize                    size,
    VkDeviceSize                    alignment,
    struct R_CVulkan_Suballocation* outAllocation);

static void R_CVulkan_MemoryBlockFree (
    struct R_CVulkan_MemoryBlock*         block,
    const struct R_CVulkan_Suballocation* pAllocation);

static VkDeviceSize R_CVulkan_GetAdjustedAlignment (
    VkPhysicalDevice   physicalDevice,
    VkDeviceSize       requestedAlignment,
    VkBufferUsageFlags usage);

static VkDeviceSize
R_CVulkan_GetBlockSize (VkDeviceSize requestedSize, VkDeviceSize minBlockSize, VkDeviceSize maxBlockSize);

static int
R_CVulkan_FreeRegionAdd (struct R_CVulkan_MemoryBlock* block, VkDeviceSize offset, VkDeviceSize size);

static void R_CVulkan_FreeRegionMergeAdjacent (struct R_CVulkan_MemoryBlock* block);

static void R_CVulkan_MemoryBlockInitializeFields (
    struct R_CVulkan_MemoryBlock* block,
    VkDevice                      device,
    VkPhysicalDevice              physicalDevice,
    VkDeviceSize                  size,
    VkBufferUsageFlags            usage,
    VkMemoryPropertyFlags         properties);

static enum R_CVulkanError R_CVulkan_MemoryBlockCreateBuffer (struct R_CVulkan_MemoryBlock* block);

static enum R_CVulkanError R_CVulkan_MemoryBlockAllocateAndBindMemory (struct R_CVulkan_MemoryBlock* block);

static int R_CVulkan_MemoryBlockFindSuitableRegion (
    struct R_CVulkan_MemoryBlock* block,
    VkDeviceSize                  size,
    VkDeviceSize                  alignment,
    uint32_t*                     outRegionIndex,
    VkDeviceSize*                 outAlignedOffset,
    VkDeviceSize*                 outPadding);

static void R_CVulkan_MemoryBlockUpdateAllocation (
    struct R_CVulkan_MemoryBlock*   block,
    uint32_t                        regionIndex,
    VkDeviceSize                    alignedOffset,
    VkDeviceSize                    size,
    VkDeviceSize                    padding,
    struct R_CVulkan_Suballocation* outAllocation);

static int R_CVulkan_MemoryBlockTryMergeWithNext (struct R_CVulkan_MemoryBlock* block, uint32_t regionIndex);

static int
R_CVulkan_MemoryBlockTryMergeWithPrevious (struct R_CVulkan_MemoryBlock* block, uint32_t regionIndex);

static void R_CVulkan_MemoryBlockRemoveFreeRegion (struct R_CVulkan_MemoryBlock* block, uint32_t regionIndex);

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_NewMemoryAllocator (
    struct R_CVulkan_MemoryAllocator* pAllocator,
    VkDevice                          device,
    VkPhysicalDevice                  physicalDevice)
{
        R_CVULKAN_ASSERT (pAllocator != NULL);
        R_CVULKAN_ASSERT (device != VK_NULL_HANDLE);
        R_CVULKAN_ASSERT (physicalDevice != VK_NULL_HANDLE);

        if (!pAllocator || device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        pAllocator->device = device;
        pAllocator->physicalDevice = physicalDevice;
        pAllocator->ppBlocks = NULL;
        pAllocator->blockCount = 0;
        pAllocator->blockCapacity = 0;
        pAllocator->minBlockSize = R_CVULKAN_DEFAULT_MIN_BLOCK_SIZE;
        pAllocator->defaultMaxBlockSize = R_CVULKAN_DEFAULT_MAX_BLOCK_SIZE;

        return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_DeleteMemoryAllocator (struct R_CVulkan_MemoryAllocator* pAllocator)
{
#if defined(R_CVULKAN_DEBUG)
        if (!pAllocator)
        {
                return;
        }
#endif
        R_CVULKAN_ASSERT (pAllocator->blockCount == 0 || pAllocator->ppBlocks != NULL);
        for (uint32_t i = 0; i < pAllocator->blockCount; ++i)
        {
                if (pAllocator->ppBlocks[i])
                {
                        R_CVulkan_DeleteMemoryBlock (pAllocator->ppBlocks[i]);
                        R_CSTL_HeapFree (pAllocator->ppBlocks[i]);
                        pAllocator->ppBlocks[i] = NULL;
                }
        }

        if (pAllocator->ppBlocks)
        {
                R_CSTL_HeapFree (pAllocator->ppBlocks);
                pAllocator->ppBlocks = NULL;
        }
#if defined(R_CVULKAN_DEBUG)
        pAllocator->blockCount = 0;
        pAllocator->blockCapacity = 0;
        pAllocator->device = VK_NULL_HANDLE;
        pAllocator->physicalDevice = VK_NULL_HANDLE;
#endif
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_MemoryAllocatorAllocate (
    struct R_CVulkan_MemoryAllocator*            pAllocator,
    const struct R_CVulkan_MemoryAllocationInfo* pAllocInfo,
    struct R_CVulkan_Suballocation*              outAllocation)
{
        R_CVULKAN_ASSERT (pAllocator != NULL);
        R_CVULKAN_ASSERT (pAllocInfo != NULL);
        R_CVULKAN_ASSERT (outAllocation != NULL);
        R_CVULKAN_ASSERT (pAllocInfo->size > 0);
#if defined(R_CVULKAN_DEBUG)
        if (!pAllocator || !pAllocInfo || !outAllocation)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (pAllocInfo->size == 0)
        {
                return R_CVULKAN_ERROR_INVALID_ARGUMENT;
        }
#endif
        VkDeviceSize adjustedAlignment = R_CVulkan_GetAdjustedAlignment (
            pAllocator->physicalDevice,
            pAllocInfo->alignment,
            pAllocInfo->usage);

        for (uint32_t i = 0; i < pAllocator->blockCount; ++i)
        {
                struct R_CVulkan_MemoryBlock* block = pAllocator->ppBlocks[i];
                if (block->usage == pAllocInfo->usage && block->properties == pAllocInfo->properties)
                {
                        if (R_CVulkan_MemoryBlockAllocate (
                                block,
                                pAllocInfo->size,
                                adjustedAlignment,
                                outAllocation))
                        {
                                outAllocation->blockIndex = i;
                                return R_CVULKAN_OK;
                        }
                }
        }

        VkDeviceSize blockSize = R_CVulkan_GetBlockSize (
            pAllocInfo->size,
            pAllocator->minBlockSize,
            pAllocator->defaultMaxBlockSize);

        struct R_CVulkan_MemoryBlock* newBlock
            = (struct R_CVulkan_MemoryBlock*)R_CSTL_HeapAlloc (sizeof (struct R_CVulkan_MemoryBlock));
        if (!newBlock)
        {
                return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }
        memset (newBlock, 0, sizeof (struct R_CVulkan_MemoryBlock));

        enum R_CVulkanError result = R_CVulkan_NewMemoryBlock (
            newBlock,
            pAllocator->device,
            pAllocator->physicalDevice,
            blockSize,
            pAllocInfo->usage,
            pAllocInfo->properties);
        if (result != R_CVULKAN_OK)
        {
                R_CSTL_HeapFree (newBlock);
                return result;
        }

        if (pAllocator->blockCount >= pAllocator->blockCapacity)
        {
                uint32_t newCapacity = pAllocator->blockCapacity == 0 ? 4 : pAllocator->blockCapacity * 2;
                struct R_CVulkan_MemoryBlock** newBlocks
                    = (struct R_CVulkan_MemoryBlock**)R_CSTL_HeapRealloc (
                        pAllocator->ppBlocks,
                        newCapacity * sizeof (struct R_CVulkan_MemoryBlock*));
                if (!newBlocks)
                {
                        R_CVulkan_DeleteMemoryBlock (newBlock);
                        R_CSTL_HeapFree (newBlock);
                        return R_CVULKAN_ERROR_OUT_OF_MEMORY;
                }
                pAllocator->ppBlocks = newBlocks;
                pAllocator->blockCapacity = newCapacity;
        }

        pAllocator->ppBlocks[pAllocator->blockCount] = newBlock;
        uint32_t blockIndex = pAllocator->blockCount++;
        pAllocator->ppBlocks[blockIndex] = newBlock;

        if (!R_CVulkan_MemoryBlockAllocate (newBlock, pAllocInfo->size, adjustedAlignment, outAllocation))
        {
                R_CVulkan_DeleteMemoryBlock (newBlock);
                R_CSTL_HeapFree (newBlock);
                pAllocator->blockCount--;
                return R_CVULKAN_ERROR_FAILED;
        }

        outAllocation->blockIndex = blockIndex;
        return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_MemoryAllocatorFree (
    struct R_CVulkan_MemoryAllocator*     pAllocator,
    const struct R_CVulkan_Suballocation* allocation)
{
        R_CVULKAN_ASSERT (pAllocator != NULL);
        R_CVULKAN_ASSERT (allocation != NULL);

        if (!pAllocator || !allocation)
        {
                return;
        }

        if (allocation->blockIndex >= pAllocator->blockCount)
        {
                return;
        }

        struct R_CVulkan_MemoryBlock* block = pAllocator->ppBlocks[allocation->blockIndex];
        if (block)
        {
                R_CVulkan_MemoryBlockFree (block, allocation);
        }
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_FindMemoryType (
    VkPhysicalDevice            physicalDevice,
    const VkMemoryRequirements* memRequirements,
    VkMemoryPropertyFlags       properties,
    uint32_t*                   outTypeIndex)
{
        R_CVULKAN_ASSERT (physicalDevice != VK_NULL_HANDLE);
        R_CVULKAN_ASSERT (memRequirements != NULL);
        R_CVULKAN_ASSERT (outTypeIndex != NULL);

        if (!physicalDevice || !memRequirements || !outTypeIndex)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties (physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
        {
                if ((memRequirements->memoryTypeBits & (1 << i))
                    && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                {
                        *outTypeIndex = i;
                        return R_CVULKAN_OK;
                }
        }

        return R_CVULKAN_ERROR_FAILED;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_CopyDataToMemory (
    VkDevice       device,
    VkDeviceMemory bufferMemory,
    VkDeviceSize   offset,
    VkDeviceSize   size,
    const void*    data)
{
        R_CVULKAN_ASSERT (device != VK_NULL_HANDLE);
        R_CVULKAN_ASSERT (bufferMemory != VK_NULL_HANDLE);
        R_CVULKAN_ASSERT (data != NULL);
        R_CVULKAN_ASSERT (size > 0);

        if (!data || size == 0)
        {
                return R_CVULKAN_ERROR_INVALID_ARGUMENT;
        }
        void*    mapped = NULL;
        VkResult result = vkMapMemory (device, bufferMemory, offset, size, 0, &mapped);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_MAP_MEMORY_FAILED;
        }

        memcpy (mapped, data, size);
        vkUnmapMemory (device, bufferMemory);

        return R_CVULKAN_OK;
}

R_CVULKAN_API VkDevice
R_CVulkan_MemoryAllocatorGetDevice (const struct R_CVulkan_MemoryAllocator* pAllocator)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pAllocator != NULL);
#endif
        return pAllocator->device;
}

R_CVULKAN_API VkPhysicalDevice
R_CVulkan_MemoryAllocatorGetPhysicalDevice (const struct R_CVulkan_MemoryAllocator* pAllocator)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pAllocator != NULL);
#endif
        return pAllocator->physicalDevice;
}

R_CVULKAN_API VkDeviceSize
R_CVulkan_MemoryAllocatorGetTotalSize (const struct R_CVulkan_MemoryAllocator* pAllocator)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pAllocator != NULL);
        R_CVULKAN_ASSERT (pAllocator->blockCount == 0 || pAllocator->ppBlocks != NULL);
#else
        if (!pAllocator)
        {
                return 0;
        }
#endif

        VkDeviceSize total = 0;
        for (uint32_t i = 0; i < pAllocator->blockCount; ++i)
        {
                if (pAllocator->ppBlocks[i])
                {
                        total += pAllocator->ppBlocks[i]->size;
                }
        }
        return total;
}

R_CVULKAN_API VkDeviceSize
R_CVulkan_MemoryAllocatorGetUsedSize (const struct R_CVulkan_MemoryAllocator* pAllocator)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pAllocator != NULL);
        R_CVULKAN_ASSERT (pAllocator->blockCount == 0 || pAllocator->ppBlocks != NULL);
#else
        if (!pAllocator)
        {
                return 0;
        }
#endif

        VkDeviceSize total = 0;
        for (uint32_t i = 0; i < pAllocator->blockCount; ++i)
        {
                if (pAllocator->ppBlocks[i])
                {
                        total += pAllocator->ppBlocks[i]->usedSize;
                }
        }
        return total;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_MemoryAllocatorAllocateImageMemory (
    VkDevice              device,
    VkPhysicalDevice      physicalDevice,
    VkImage               image,
    VkMemoryPropertyFlags properties,
    VkDeviceMemory*       outMemory)
{
        R_CVULKAN_ASSERT (device != VK_NULL_HANDLE);
        R_CVULKAN_ASSERT (physicalDevice != VK_NULL_HANDLE);
        R_CVULKAN_ASSERT (image != VK_NULL_HANDLE);
        R_CVULKAN_ASSERT (outMemory != NULL);

        if (!device || physicalDevice == VK_NULL_HANDLE || image == VK_NULL_HANDLE || !outMemory)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements (device, image, &memRequirements);

        uint32_t            memoryTypeIndex = 0;
        enum R_CVulkanError error
            = R_CVulkan_FindMemoryType (physicalDevice, &memRequirements, properties, &memoryTypeIndex);
        if (error != R_CVULKAN_OK)
        {
                return error;
        }

        VkMemoryAllocateInfo allocInfo = {0};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = memoryTypeIndex;

        VkResult result = vkAllocateMemory (device, &allocInfo, NULL, outMemory);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_MEMORY_ALLOCATE_FAILED;
        }

        return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_MemoryAllocatorFreeImageMemory (VkDevice device, VkDeviceMemory memory)
{
        R_CVULKAN_ASSERT (device != VK_NULL_HANDLE);
        R_CVULKAN_ASSERT (memory != VK_NULL_HANDLE);

        if (!device || memory == VK_NULL_HANDLE)
        {
                return;
        }

        vkFreeMemory (device, memory, NULL);
}

static enum R_CVulkanError R_CVulkan_MemoryAllocatorValidateDefragContext (
    const struct R_CVulkan_MemoryAllocator* pAllocator,
    const struct R_CVulkan_DefragContext*   pContext);

static enum R_CVulkanError R_CVulkan_MemoryAllocatorCreateDefragContext (
    struct R_CVulkan_MemoryAllocator*    pAllocator,
    const struct R_CVulkan_DefragConfig* pConfig,
    struct R_CVulkan_DefragContext**     ppContext);

static void R_CVulkan_MemoryAllocatorDestroyDefragContext (struct R_CVulkan_DefragContext* pContext);

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_MemoryAllocatorBeginDefragmentation (
    struct R_CVulkan_MemoryAllocator*    pAllocator,
    struct R_CVulkan_DefragContext**     ppContext,
    const struct R_CVulkan_DefragConfig* pConfig)
{
        enum R_CVulkanError result = R_CVULKAN_OK;

        R_CVULKAN_ASSERT (pAllocator != NULL);
        R_CVULKAN_ASSERT (ppContext != NULL);

        if (!pAllocator || !ppContext)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        result = R_CVulkan_MemoryAllocatorCreateDefragContext (pAllocator, pConfig, ppContext);
        if (result != R_CVULKAN_OK)
        {
                return result;
        }

        result = R_CVulkan_DefragBegin (*ppContext);
        if (result != R_CVULKAN_OK)
        {
                R_CVulkan_MemoryAllocatorDestroyDefragContext (*ppContext);
                *ppContext = NULL;
                return result;
        }

        return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_MemoryAllocatorExecuteDefragPass (
    struct R_CVulkan_MemoryAllocator* pAllocator,
    struct R_CVulkan_DefragContext*   pContext,
    VkCommandBuffer                   commandBuffer)
{
        enum R_CVulkanError result = R_CVULKAN_OK;

        R_CVULKAN_ASSERT (pAllocator != NULL);
        R_CVULKAN_ASSERT (pContext != NULL);

        if (!pAllocator || !pContext)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        result = R_CVulkan_MemoryAllocatorValidateDefragContext (pAllocator, pContext);
        if (result != R_CVULKAN_OK)
        {
                return result;
        }

        result = R_CVulkan_DefragExecutePass (pContext, commandBuffer);
        return result;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_MemoryAllocatorEndDefragmentation (
    struct R_CVulkan_MemoryAllocator* pAllocator,
    struct R_CVulkan_DefragContext*   pContext,
    struct R_CVulkan_DefragStats*     pStats)
{
        enum R_CVulkanError result = R_CVULKAN_OK;

        R_CVULKAN_ASSERT (pAllocator != NULL);
        R_CVULKAN_ASSERT (pContext != NULL);

        if (!pAllocator || !pContext)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        result = R_CVulkan_MemoryAllocatorValidateDefragContext (pAllocator, pContext);
        if (result != R_CVULKAN_OK)
        {
                return result;
        }

        result = R_CVulkan_DefragEnd (pContext, pStats);
        R_CVulkan_MemoryAllocatorDestroyDefragContext (pContext);

        return result;
}

static enum R_CVulkanError
R_CVulkan_MemoryAllocatorValidateDefragContext (
    const struct R_CVulkan_MemoryAllocator* pAllocator,
    const struct R_CVulkan_DefragContext*   pContext)
{
        if (!pAllocator || !pContext)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (pContext->pAllocator != pAllocator)
        {
                return R_CVULKAN_ERROR_INVALID_ARGUMENT;
        }

        return R_CVULKAN_OK;
}

static enum R_CVulkanError
R_CVulkan_MemoryAllocatorCreateDefragContext (
    struct R_CVulkan_MemoryAllocator*    pAllocator,
    const struct R_CVulkan_DefragConfig* pConfig,
    struct R_CVulkan_DefragContext**     ppContext)
{
        enum R_CVulkanError             result = R_CVULKAN_OK;
        struct R_CVulkan_DefragContext* pContext = NULL;

        pContext
            = (struct R_CVulkan_DefragContext*)R_CSTL_HeapAlloc (sizeof (struct R_CVulkan_DefragContext));
        if (!pContext)
        {
                return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }

        result = R_CVulkan_DefragInitialize (pContext, pAllocator, pConfig);
        if (result != R_CVULKAN_OK)
        {
                R_CSTL_HeapFree (pContext);
                return result;
        }

        *ppContext = pContext;
        return R_CVULKAN_OK;
}

static void
R_CVulkan_MemoryAllocatorDestroyDefragContext (struct R_CVulkan_DefragContext* pContext)
{
        if (!pContext)
        {
                return;
        }

        R_CVulkan_DefragCleanup (pContext);
        R_CSTL_HeapFree (pContext);
}

static enum R_CVulkanError
R_CVulkan_NewMemoryBlock (
    struct R_CVulkan_MemoryBlock* block,
    VkDevice                      device,
    VkPhysicalDevice              physicalDevice,
    VkDeviceSize                  size,
    VkBufferUsageFlags            usage,
    VkMemoryPropertyFlags         properties)
{
        R_CVulkan_MemoryBlockInitializeFields (block, device, physicalDevice, size, usage, properties);

        enum R_CVulkanError error = R_CVulkan_MemoryBlockCreateBuffer (block);
        if (error != R_CVULKAN_OK)
        {
                return error;
        }

        error = R_CVulkan_MemoryBlockAllocateAndBindMemory (block);
        if (error != R_CVULKAN_OK)
        {
                vkDestroyBuffer (block->device, block->buffer, NULL);
                block->buffer = VK_NULL_HANDLE;
                return error;
        }

        R_CVulkan_FreeRegionAdd (block, 0, size);
        return R_CVULKAN_OK;
}

static void
R_CVulkan_MemoryBlockInitializeFields (
    struct R_CVulkan_MemoryBlock* block,
    VkDevice                      device,
    VkPhysicalDevice              physicalDevice,
    VkDeviceSize                  size,
    VkBufferUsageFlags            usage,
    VkMemoryPropertyFlags         properties)
{
        block->device = device;
        block->physicalDevice = physicalDevice;
        block->size = size;
        block->usage = usage;
        block->properties = properties;
        block->buffer = VK_NULL_HANDLE;
        block->memory = VK_NULL_HANDLE;
        block->usedSize = 0;
        block->pFreeRegions = NULL;
        block->freeRegionCount = 0;
        block->freeRegionCapacity = 0;
}

static enum R_CVulkanError
R_CVulkan_MemoryBlockCreateBuffer (struct R_CVulkan_MemoryBlock* block)
{
        VkBufferCreateInfo bufferInfo = {0};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = block->size;
        bufferInfo.usage = block->usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateBuffer (block->device, &bufferInfo, NULL, &block->buffer);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_BUFFER_CREATE_FAILED;
        }
        return R_CVULKAN_OK;
}

static enum R_CVulkanError
R_CVulkan_MemoryBlockAllocateAndBindMemory (struct R_CVulkan_MemoryBlock* block)
{
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements (block->device, block->buffer, &memRequirements);

        uint32_t            memoryTypeIndex = 0;
        enum R_CVulkanError error = R_CVulkan_FindMemoryType (
            block->physicalDevice,
            &memRequirements,
            block->properties,
            &memoryTypeIndex);
        if (error != R_CVULKAN_OK)
        {
                return error;
        }

        VkMemoryAllocateInfo allocInfo = {0};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = block->size;
        allocInfo.memoryTypeIndex = memoryTypeIndex;

        VkResult result = vkAllocateMemory (block->device, &allocInfo, NULL, &block->memory);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_MEMORY_ALLOCATE_FAILED;
        }

        result = vkBindBufferMemory (block->device, block->buffer, block->memory, 0);
        if (result != VK_SUCCESS)
        {
                vkFreeMemory (block->device, block->memory, NULL);
                block->memory = VK_NULL_HANDLE;
                return R_CVULKAN_ERROR_FAILED;
        }
        return R_CVULKAN_OK;
}

static void
R_CVulkan_DeleteMemoryBlock (struct R_CVulkan_MemoryBlock* block)
{
        if (!block)
        {
                return;
        }

        if (block->buffer != VK_NULL_HANDLE)
        {
                vkDestroyBuffer (block->device, block->buffer, NULL);
                block->buffer = VK_NULL_HANDLE;
        }

        if (block->memory != VK_NULL_HANDLE)
        {
                vkFreeMemory (block->device, block->memory, NULL);
                block->memory = VK_NULL_HANDLE;
        }

        if (block->pFreeRegions)
        {
                R_CSTL_HeapFree (block->pFreeRegions);
                block->pFreeRegions = NULL;
        }

        block->freeRegionCount = 0;
        block->freeRegionCapacity = 0;
        block->usedSize = 0;
}

static int
R_CVulkan_MemoryBlockAllocate (
    struct R_CVulkan_MemoryBlock*   block,
    VkDeviceSize                    size,
    VkDeviceSize                    alignment,
    struct R_CVulkan_Suballocation* outAllocation)
{
        uint32_t     regionIndex = 0;
        VkDeviceSize alignedOffset = 0;
        VkDeviceSize padding = 0;

        if (!R_CVulkan_MemoryBlockFindSuitableRegion (
                block,
                size,
                alignment,
                &regionIndex,
                &alignedOffset,
                &padding))
        {
                return 0;
        }

        R_CVulkan_MemoryBlockUpdateAllocation (
            block,
            regionIndex,
            alignedOffset,
            size,
            padding,
            outAllocation);
        return 1;
}

static int
R_CVulkan_MemoryBlockFindSuitableRegion (
    struct R_CVulkan_MemoryBlock* block,
    VkDeviceSize                  size,
    VkDeviceSize                  alignment,
    uint32_t*                     outRegionIndex,
    VkDeviceSize*                 outAlignedOffset,
    VkDeviceSize*                 outPadding)
{
        for (uint32_t i = 0; i < block->freeRegionCount; ++i)
        {
                VkDeviceSize alignedOffset
                    = (block->pFreeRegions[i].offset + alignment - 1) & ~(alignment - 1);
                VkDeviceSize padding = alignedOffset - block->pFreeRegions[i].offset;

                if (block->pFreeRegions[i].size >= size + padding)
                {
                        *outRegionIndex = i;
                        *outAlignedOffset = alignedOffset;
                        *outPadding = padding;
                        return 1;
                }
        }
        return 0;
}

static void
R_CVulkan_MemoryBlockUpdateAllocation (
    struct R_CVulkan_MemoryBlock*   block,
    uint32_t                        regionIndex,
    VkDeviceSize                    alignedOffset,
    VkDeviceSize                    size,
    VkDeviceSize                    padding,
    struct R_CVulkan_Suballocation* outAllocation)
{
        outAllocation->offset = alignedOffset;
        outAllocation->size = size;
        outAllocation->buffer = block->buffer;
        outAllocation->memory = block->memory;
        outAllocation->blockIndex = 0;

        block->usedSize += size;

        if (block->pFreeRegions[regionIndex].size == size + padding)
        {
                for (uint32_t j = regionIndex; j < block->freeRegionCount - 1; ++j)
                {
                        block->pFreeRegions[j] = block->pFreeRegions[j + 1];
                }
                block->freeRegionCount--;
        }
        else
        {
                // Shrink the free region
                block->pFreeRegions[regionIndex].offset = alignedOffset + size;
                block->pFreeRegions[regionIndex].size -= size + padding;
        }
}

static void
R_CVulkan_MemoryBlockFree (
    struct R_CVulkan_MemoryBlock*         block,
    const struct R_CVulkan_Suballocation* allocation)
{
        VkDeviceSize allocEnd = allocation->offset + allocation->size;

        for (uint32_t i = 0; i < block->freeRegionCount; ++i)
        {
                if (block->pFreeRegions[i].offset == allocEnd)
                {
                        block->pFreeRegions[i].offset = allocation->offset;
                        block->pFreeRegions[i].size += allocation->size;

                        if (i > 0)
                        {
                                R_CVulkan_MemoryBlockTryMergeWithPrevious (block, i);
                        }

                        block->usedSize -= allocation->size;
                        return;
                }

                if (block->pFreeRegions[i].offset + block->pFreeRegions[i].size == allocation->offset)
                {
                        block->pFreeRegions[i].size += allocation->size;

                        if (i + 1 < block->freeRegionCount)
                        {
                                R_CVulkan_MemoryBlockTryMergeWithNext (block, i);
                        }

                        block->usedSize -= allocation->size;
                        return;
                }
        }

        R_CVulkan_FreeRegionAdd (block, allocation->offset, allocation->size);
        block->usedSize -= allocation->size;
}

static int
R_CVulkan_MemoryBlockTryMergeWithNext (struct R_CVulkan_MemoryBlock* block, uint32_t regionIndex)
{
        if (regionIndex + 1 >= block->freeRegionCount)
        {
                return 0;
        }

        if (block->pFreeRegions[regionIndex].offset + block->pFreeRegions[regionIndex].size
            == block->pFreeRegions[regionIndex + 1].offset)
        {
                block->pFreeRegions[regionIndex].size += block->pFreeRegions[regionIndex + 1].size;
                R_CVulkan_MemoryBlockRemoveFreeRegion (block, regionIndex + 1);
                return 1;
        }
        return 0;
}

static int
R_CVulkan_MemoryBlockTryMergeWithPrevious (struct R_CVulkan_MemoryBlock* block, uint32_t regionIndex)
{
        if (regionIndex == 0)
        {
                return 0;
        }

        if (block->pFreeRegions[regionIndex - 1].offset + block->pFreeRegions[regionIndex - 1].size
            == block->pFreeRegions[regionIndex].offset)
        {
                block->pFreeRegions[regionIndex - 1].size += block->pFreeRegions[regionIndex].size;
                R_CVulkan_MemoryBlockRemoveFreeRegion (block, regionIndex);
                return 1;
        }
        return 0;
}

static void
R_CVulkan_MemoryBlockRemoveFreeRegion (struct R_CVulkan_MemoryBlock* block, uint32_t regionIndex)
{
        for (uint32_t j = regionIndex; j < block->freeRegionCount - 1; ++j)
        {
                block->pFreeRegions[j] = block->pFreeRegions[j + 1];
        }
        block->freeRegionCount--;
}

static VkDeviceSize
R_CVulkan_GetAdjustedAlignment (
    VkPhysicalDevice   physicalDevice,
    VkDeviceSize       requestedAlignment,
    VkBufferUsageFlags usage)
{
        VkDeviceSize deviceAlignment = requestedAlignment;

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties (physicalDevice, &deviceProperties);
        VkPhysicalDeviceLimits limits = deviceProperties.limits;

        if (usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        {
                if (limits.minUniformBufferOffsetAlignment > deviceAlignment)
                {
                        deviceAlignment = limits.minUniformBufferOffsetAlignment;
                }
        }
        if (usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        {
                if (limits.minStorageBufferOffsetAlignment > deviceAlignment)
                {
                        deviceAlignment = limits.minStorageBufferOffsetAlignment;
                }
        }

        return deviceAlignment;
}

static VkDeviceSize
R_CVulkan_GetBlockSize (VkDeviceSize requestedSize, VkDeviceSize minBlockSize, VkDeviceSize maxBlockSize)
{
        if (requestedSize < 65536)
        {
                VkDeviceSize blockSize = requestedSize;
                if (blockSize < minBlockSize)
                {
                        blockSize = minBlockSize;
                }
                blockSize = (blockSize + 255) & ~255;
                return blockSize;
        }

        VkDeviceSize alignedSize = requestedSize;
        if (alignedSize < minBlockSize)
        {
                alignedSize = minBlockSize;
        }

        VkDeviceSize powerOf2 = minBlockSize;
        while (powerOf2 < alignedSize && powerOf2 < maxBlockSize)
        {
                powerOf2 *= 2;
        }

        VkDeviceSize blockSize = (powerOf2 < maxBlockSize) ? powerOf2 : maxBlockSize;

        if (requestedSize > blockSize)
        {
                blockSize = ((requestedSize + 4095) / 4096) * 4096; // Round up to 4KB
        }

        return blockSize;
}

static int
R_CVulkan_FreeRegionAdd (struct R_CVulkan_MemoryBlock* block, VkDeviceSize offset, VkDeviceSize size)
{
        if (block->freeRegionCount >= block->freeRegionCapacity)
        {
                uint32_t newCapacity = block->freeRegionCapacity == 0 ? 4 : block->freeRegionCapacity * 2;
                struct R_CVulkan_FreeRegion* newRegions = (struct R_CVulkan_FreeRegion*)R_CSTL_HeapRealloc (
                    block->pFreeRegions,
                    newCapacity * sizeof (struct R_CVulkan_FreeRegion));
                if (!newRegions)
                {
                        return 0;
                }
                block->pFreeRegions = newRegions;
                block->freeRegionCapacity = newCapacity;
        }

        block->pFreeRegions[block->freeRegionCount].offset = offset;
        block->pFreeRegions[block->freeRegionCount].size = size;
        block->freeRegionCount++;

        for (uint32_t i = 0; i < block->freeRegionCount - 1; ++i)
        {
                for (uint32_t j = i + 1; j < block->freeRegionCount; ++j)
                {
                        if (block->pFreeRegions[i].offset > block->pFreeRegions[j].offset)
                        {
                                struct R_CVulkan_FreeRegion temp = block->pFreeRegions[i];
                                block->pFreeRegions[i] = block->pFreeRegions[j];
                                block->pFreeRegions[j] = temp;
                        }
                }
        }

        return 1;
}

static void
R_CVulkan_FreeRegionMergeAdjacent (struct R_CVulkan_MemoryBlock* block)
{
        (void)block;
}
