#include "rlgame.base/cvulkan/cvulkan_command_pool.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_NewCommandPool (
    struct R_CVulkan_CommandPool*  pCommandPool,
    const struct R_CVulkan_Device* pDevice,
    uint32_t                       queueFamilyIndex,
    VkCommandPoolCreateFlags       flags)
{
    R_CVULKAN_ASSERT (pCommandPool);
    R_CVULKAN_ASSERT (pDevice);

    pCommandPool->device = R_CVulkan_DeviceGetLogicalDevice (pDevice);
    pCommandPool->queueFamilyIndex = queueFamilyIndex;
    VkCommandPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = flags;

    VkResult result = vkCreateCommandPool (pCommandPool->device, &poolInfo, NULL, &pCommandPool->handle);
    if (result != VK_SUCCESS)
    {
        return R_CVULKAN_ERROR_COMMAND_POOL_CREATE_FAILED;
    }
    return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_DeleteCommandPool (struct R_CVulkan_CommandPool* pCommandPool)
{
    R_CVULKAN_ASSERT (pCommandPool);
    vkDestroyCommandPool (pCommandPool->device, pCommandPool->handle, NULL);
#if defined(R_CVULKAN_DEBUG)
    pCommandPool->handle = VK_NULL_HANDLE;
    pCommandPool->device = VK_NULL_HANDLE;
    pCommandPool->queueFamilyIndex = 0;
#endif
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_CommandPoolReset (struct R_CVulkan_CommandPool* pCommandPool, VkCommandPoolResetFlags flags)
{
    R_CVULKAN_ASSERT (pCommandPool);

    if (!pCommandPool)
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }

#if defined(R_CVULKAN_DEBUG)

    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }
#endif

    VkResult result = vkResetCommandPool (pCommandPool->device, pCommandPool->handle, flags);
    if (result != VK_SUCCESS)
    {
        return R_CVULKAN_ERROR_FAILED;
    }

    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_CommandPoolTrim (struct R_CVulkan_CommandPool* pCommandPool)
{
    R_CVULKAN_ASSERT (pCommandPool);

    if (!pCommandPool)
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }

#if defined(R_CVULKAN_DEBUG)

    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }
#endif
    (void)pCommandPool;
    return R_CVULKAN_OK;
}

R_CVULKAN_API VkCommandPool
R_CVulkan_CommandPoolGetHandle (const struct R_CVulkan_CommandPool* pCommandPool)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pCommandPool);
#endif
    return pCommandPool->handle;
}

R_CVULKAN_API VkDevice
R_CVulkan_CommandPoolGetDevice (const struct R_CVulkan_CommandPool* pCommandPool)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pCommandPool);
#endif
    return pCommandPool->device;
}

R_CVULKAN_API uint32_t
R_CVulkan_CommandPoolGetQueueFamilyIndex (const struct R_CVulkan_CommandPool* pCommandPool)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pCommandPool);
#endif
    return pCommandPool->queueFamilyIndex;
}

R_CVULKAN_API int
R_CVulkan_CommandPoolIsInitialized (const struct R_CVulkan_CommandPool* pCommandPool)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pCommandPool);
    return 1;
#else
    (void)pCommandPool;
    return 1;
#endif
}
