#include "rlgame.base/cvulkan/cvulkan_descriptor_set.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

#include <string.h>
#include <stdlib.h>

enum R_CVulkanError
R_CVulkan_NewDescriptorSetLayout (
    struct R_CVulkan_DescriptorSetLayout*                 pLayout,
    const struct R_CVulkan_DescriptorSetLayoutCreateInfo* pCreateInfo)
{
    R_CVULKAN_ASSERT (pLayout);
    R_CVULKAN_ASSERT (pCreateInfo);
    R_CVULKAN_ASSERT (pCreateInfo->device);
    R_CVULKAN_ASSERT (pCreateInfo->bindings);
    R_CVULKAN_ASSERT (pCreateInfo->bindingCount > 0);

#if defined(R_CVULKAN_DEBUG)
    if (!pLayout || !pCreateInfo || !pCreateInfo->device || !pCreateInfo->bindings
        || pCreateInfo->bindingCount == 0)
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }
    if (!R_CVulkan_DeviceIsInitialized (pCreateInfo->device))
    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }
#endif

    pLayout->device = R_CVulkan_DeviceGetLogicalDevice (pCreateInfo->device);
    pLayout->handle = VK_NULL_HANDLE;
#if defined(R_CVULKAN_DEBUG)
    pLayout->booted = false;
#endif

    VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = pCreateInfo->bindingCount;
    layoutInfo.pBindings = pCreateInfo->bindings;

    VkResult result = vkCreateDescriptorSetLayout (pLayout->device, &layoutInfo, NULL, &pLayout->handle);
    if (result != VK_SUCCESS)
    {
        return R_CVULKAN_ERROR_DESCRIPTOR_SET_LAYOUT_CREATE_FAILED;
    }

#if defined(R_CVULKAN_DEBUG)
    pLayout->booted = true;
#endif
    return R_CVULKAN_OK;
}

void
R_CVulkan_DeleteDescriptorSetLayout (struct R_CVulkan_DescriptorSetLayout* pLayout)
{
    R_CVULKAN_ASSERT (pLayout);

#if defined(R_CVULKAN_DEBUG)
    if (!pLayout)
    {
        return;
    }
#endif

    if (pLayout->handle != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout (pLayout->device, pLayout->handle, NULL);
        pLayout->handle = VK_NULL_HANDLE;
    }

    pLayout->device = VK_NULL_HANDLE;
#if defined(R_CVULKAN_DEBUG)
    pLayout->booted = false;
#endif
}

enum R_CVulkanError
R_CVulkan_NewDescriptorPool (
    struct R_CVulkan_DescriptorPool*                 pPool,
    const struct R_CVulkan_DescriptorPoolCreateInfo* pCreateInfo)
{
    R_CVULKAN_ASSERT (pPool);
    R_CVULKAN_ASSERT (pCreateInfo);
    R_CVULKAN_ASSERT (pCreateInfo->device);
    R_CVULKAN_ASSERT (pCreateInfo->poolSizes);
    R_CVULKAN_ASSERT (pCreateInfo->poolSizeCount > 0);
    R_CVULKAN_ASSERT (pCreateInfo->maxSets > 0);

#if defined(R_CVULKAN_DEBUG)
    if (!pPool || !pCreateInfo || !pCreateInfo->device || !pCreateInfo->poolSizes
        || pCreateInfo->poolSizeCount == 0 || pCreateInfo->maxSets == 0)
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }

    if (!R_CVulkan_DeviceIsInitialized (pCreateInfo->device))
    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }
#endif

    pPool->device = R_CVulkan_DeviceGetLogicalDevice (pCreateInfo->device);
    pPool->handle = VK_NULL_HANDLE;
    pPool->maxSets = pCreateInfo->maxSets;
#if defined(R_CVULKAN_DEBUG)
    pPool->booted = false;
#endif

    VkDescriptorPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = pCreateInfo->poolSizeCount;
    poolInfo.pPoolSizes = pCreateInfo->poolSizes;
    poolInfo.maxSets = pCreateInfo->maxSets;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VkResult result = vkCreateDescriptorPool (pPool->device, &poolInfo, NULL, &pPool->handle);
    if (result != VK_SUCCESS)
    {
        return R_CVULKAN_ERROR_DESCRIPTOR_POOL_CREATE_FAILED;
    }

#if defined(R_CVULKAN_DEBUG)
    pPool->booted = true;
#endif
    return R_CVULKAN_OK;
}

void
R_CVulkan_DeleteDescriptorPool (struct R_CVulkan_DescriptorPool* pPool)
{
    R_CVULKAN_ASSERT (pPool);

#if defined(R_CVULKAN_DEBUG)
    if (!pPool)
    {
        return;
    }
#endif

    if (pPool->handle != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool (pPool->device, pPool->handle, NULL);
        pPool->handle = VK_NULL_HANDLE;
    }

    pPool->device = VK_NULL_HANDLE;
    pPool->maxSets = 0;
#if defined(R_CVULKAN_DEBUG)
    pPool->booted = false;
#endif
}

enum R_CVulkanError
R_CVulkan_DescriptorSetAllocate (
    const struct R_CVulkan_DescriptorPool* pPool,
    const VkDescriptorSetLayout*           pLayouts,
    uint32_t                               layoutCount,
    VkDescriptorSet*                       pOutSets)
{
    R_CVULKAN_ASSERT (pPool);
    R_CVULKAN_ASSERT (pLayouts);
    R_CVULKAN_ASSERT (layoutCount > 0);
    R_CVULKAN_ASSERT (pOutSets);

    if (!pPool || !pLayouts || layoutCount == 0 || !pOutSets)
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }

#if defined(R_CVULKAN_DEBUG)
    if (!pPool->booted)
    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }
#endif

    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pPool->handle;
    allocInfo.descriptorSetCount = layoutCount;
    allocInfo.pSetLayouts = pLayouts;

    VkResult result = vkAllocateDescriptorSets (pPool->device, &allocInfo, pOutSets);
    if (result != VK_SUCCESS)
    {
        return R_CVULKAN_ERROR_FAILED;
    }

    return R_CVULKAN_OK;
}

void
R_CVulkan_DescriptorSetFree (
    const struct R_CVulkan_DescriptorPool* pPool,
    const VkDescriptorSet*                 pSets,
    uint32_t                               setCount)
{
    R_CVULKAN_ASSERT (pPool);
    R_CVULKAN_ASSERT (pSets);
    R_CVULKAN_ASSERT (setCount > 0);

    if (!pPool || !pSets || setCount == 0)
    {
        return;
    }

#if defined(R_CVULKAN_DEBUG)
    if (!pPool->booted)
    {
        return;
    }
#endif

    vkFreeDescriptorSets (pPool->device, pPool->handle, setCount, pSets);
}

void
R_CVulkan_DescriptorSetUpdate (
    const struct R_CVulkan_Device* pDevice,
    const VkWriteDescriptorSet*    pDescriptorWrites,
    uint32_t                       descriptorWriteCount,
    const VkCopyDescriptorSet*     pDescriptorCopies,
    uint32_t                       descriptorCopyCount)
{
    R_CVULKAN_ASSERT (pDevice);
    R_CVULKAN_ASSERT (pDescriptorWrites);
    R_CVULKAN_ASSERT (descriptorWriteCount > 0);

    if (!pDevice || !pDescriptorWrites || descriptorWriteCount == 0)
    {
        return;
    }

#if defined(R_CVULKAN_DEBUG)
    if (!R_CVulkan_DeviceIsInitialized (pDevice))
    {
        return;
    }
#endif

    vkUpdateDescriptorSets (
        R_CVulkan_DeviceGetLogicalDevice (pDevice),
        descriptorWriteCount,
        pDescriptorWrites,
        descriptorCopyCount,
        pDescriptorCopies);
}

VkDescriptorSetLayout
R_CVulkan_DescriptorSetLayoutGetHandle (const struct R_CVulkan_DescriptorSetLayout* pLayout)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pLayout);
#endif
    return pLayout->handle;
}

VkDevice
R_CVulkan_DescriptorSetLayoutGetDevice (const struct R_CVulkan_DescriptorSetLayout* pLayout)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pLayout);
#endif
    return pLayout->device;
}

int
R_CVulkan_DescriptorSetLayoutIsInitialized (const struct R_CVulkan_DescriptorSetLayout* pLayout)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pLayout);
    return pLayout->booted;
#else
    (void)pLayout;
    return 1;
#endif
}

VkDescriptorPool
R_CVulkan_DescriptorPoolGetHandle (const struct R_CVulkan_DescriptorPool* pPool)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pPool);
#endif
    return pPool->handle;
}

VkDevice
R_CVulkan_DescriptorPoolGetDevice (const struct R_CVulkan_DescriptorPool* pPool)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pPool);
#endif
    return pPool->device;
}

uint32_t
R_CVulkan_DescriptorPoolGetMaxSets (const struct R_CVulkan_DescriptorPool* pPool)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pPool);
#endif
    return pPool->maxSets;
}

int
R_CVulkan_DescriptorPoolIsInitialized (const struct R_CVulkan_DescriptorPool* pPool)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pPool);
    return pPool->booted;
#else
    (void)pPool;
    return 1;
#endif
}
