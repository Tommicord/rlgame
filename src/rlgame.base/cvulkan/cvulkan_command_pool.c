#include "rlgame.base/cvulkan/cvulkan_command_pool.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_NewCommandPool (
    struct R_CVulkan_CommandPool*  pCommandPool,
    const struct R_CVulkan_Device* pDevice,
    uint32_t                       queueFamilyIndex,
    VkCommandPoolCreateFlags       flags)
{
    R_CVULKAN_ASSERT (pCommandPool);
    R_CVULKAN_ASSERT (pDevice);

    if (!pCommandPool || !pDevice)
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }

#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (R_CVulkan_DeviceIsInitialized (pDevice));
    if (!R_CVulkan_DeviceIsInitialized (pDevice))
    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }
#endif

    pCommandPool->device = R_CVulkan_DeviceGetLogicalDevice (pDevice);
    pCommandPool->handle = VK_NULL_HANDLE;
    pCommandPool->queueFamilyIndex = queueFamilyIndex;
#if defined(R_CVULKAN_DEBUG)
    pCommandPool->booted = false;
#endif

    VkCommandPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = flags;

    VkResult result = vkCreateCommandPool (pCommandPool->device, &poolInfo, NULL, &pCommandPool->handle);
    if (result != VK_SUCCESS)
    {
        return R_CVULKAN_ERROR_COMMAND_POOL_CREATE_FAILED;
    }

#if defined(R_CVULKAN_DEBUG)
    pCommandPool->booted = true;
#endif
    return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_DeleteCommandPool (struct R_CVulkan_CommandPool* pCommandPool)
{
    R_CVULKAN_ASSERT (pCommandPool);

#if defined(R_CVULKAN_DEBUG)
    if (!pCommandPool)
    {
        return;
    }
#endif

    if (pCommandPool->handle != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool (pCommandPool->device, pCommandPool->handle, NULL);
        pCommandPool->handle = VK_NULL_HANDLE;
    }

#if defined(R_CVULKAN_DEBUG)
    pCommandPool->booted = false;
#endif
    pCommandPool->device = VK_NULL_HANDLE;
    pCommandPool->queueFamilyIndex = 0;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_CommandPoolReset (struct R_CVulkan_CommandPool* pCommandPool, VkCommandPoolResetFlags flags)
{
    R_CVULKAN_ASSERT (pCommandPool);

    if (!pCommandPool)
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }

#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pCommandPool->booted);
    if (!pCommandPool->booted)
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

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_CommandPoolTrim (struct R_CVulkan_CommandPool* pCommandPool)
{
    R_CVULKAN_ASSERT (pCommandPool);

    if (!pCommandPool)
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }

#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pCommandPool->booted);
    if (!pCommandPool->booted)
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
    return pCommandPool->booted;
#else
    (void)pCommandPool;
    return 1;
#endif
}
