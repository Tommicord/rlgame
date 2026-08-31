#include "rlgame.base/cvulkan/cvulkan_descriptor_set.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

#include <string.h>
#include <stdlib.h>

enum R_CVulkan_Error
R_CVulkan_NewDescriptorSetLayout (
    struct R_CVulkan_DescriptorSetLayout*                 pLayout,
    const struct R_CVulkan_DescriptorSetLayoutCreateInfo* pCreateInfo)
{
    R_CVULKAN_ASSERT (pLayout);
    R_CVULKAN_ASSERT (pCreateInfo);

    pLayout->device = R_CVulkan_DeviceGetLogicalDevice (pCreateInfo->device);
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

#endif
    return R_CVULKAN_OK;
}

void
R_CVulkan_DeleteDescriptorSetLayout (struct R_CVulkan_DescriptorSetLayout* pLayout)
{
    R_CVULKAN_ASSERT (pLayout);

    vkDestroyDescriptorSetLayout (pLayout->device, pLayout->handle, NULL);
#if defined(R_CVULKAN_DEBUG)
    pLayout->device = VK_NULL_HANDLE;
#endif
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_NewDescriptorPool (
    struct R_CVulkan_DescriptorPool*                 pPool,
    const struct R_CVulkan_DescriptorPoolCreateInfo* pCreateInfo)
{
    R_CVULKAN_ASSERT (pPool);
    R_CVULKAN_ASSERT (pCreateInfo);

    pPool->device = R_CVulkan_DeviceGetLogicalDevice (pCreateInfo->device);
    pPool->handle = VK_NULL_HANDLE;
    pPool->maxSets = pCreateInfo->maxSets;

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
    return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_DeleteDescriptorPool (struct R_CVulkan_DescriptorPool* pPool)
{
    R_CVULKAN_ASSERT (pPool);

    vkDestroyDescriptorPool (pPool->device, pPool->handle, NULL);
#if defined(R_CVULKAN_DEBUG)
    pPool->device = VK_NULL_HANDLE;
    pPool->maxSets = 0;
#endif
}

R_CVULKAN_API enum R_CVulkan_Error
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

R_CVULKAN_API void
R_CVulkan_DescriptorSetFree (
    const struct R_CVulkan_DescriptorPool* pPool,
    const VkDescriptorSet*                 pSets,
    uint32_t                               setCount)
{
    R_CVULKAN_ASSERT (pPool);
    R_CVULKAN_ASSERT (pSets);
    R_CVULKAN_ASSERT (setCount > 0);

    vkFreeDescriptorSets (pPool->device, pPool->handle, setCount, pSets);
}

R_CVULKAN_API void
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

    vkUpdateDescriptorSets (
        R_CVulkan_DeviceGetLogicalDevice (pDevice),
        descriptorWriteCount,
        pDescriptorWrites,
        descriptorCopyCount,
        pDescriptorCopies);
}

R_CVULKAN_API VkDescriptorSetLayout
R_CVulkan_DescriptorSetLayoutGetHandle (const struct R_CVulkan_DescriptorSetLayout* pLayout)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pLayout);
#endif
    return pLayout->handle;
}

R_CVULKAN_API VkDevice
R_CVulkan_DescriptorSetLayoutGetDevice (const struct R_CVulkan_DescriptorSetLayout* pLayout)
{
    R_CVULKAN_ASSERT (pLayout);
    return pLayout->device;
}

R_CVULKAN_API VkDescriptorPool
R_CVulkan_DescriptorPoolGetHandle (const struct R_CVulkan_DescriptorPool* pPool)
{
    R_CVULKAN_ASSERT (pPool);
    return pPool->handle;
}

R_CVULKAN_API VkDevice
R_CVulkan_DescriptorPoolGetDevice (const struct R_CVulkan_DescriptorPool* pPool)
{
    R_CVULKAN_ASSERT (pPool);
    return pPool->device;
}

R_CVULKAN_API uint32_t
R_CVulkan_DescriptorPoolGetMaxSets (const struct R_CVulkan_DescriptorPool* pPool)
{
    R_CVULKAN_ASSERT (pPool);
    return pPool->maxSets;
}
