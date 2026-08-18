#include "rlgame.base/cvulkan/cvulkan_command_pool.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_NewCommandPool (
    struct R_CVulkan_CommandPool*   pCommandPool,
    const struct R_CVulkan_Device*  pDevice,
    uint32_t                        queueFamilyIndex,
    R_CVulkanCommandPoolCreateFlags flags)
{
        R_CVULKAN_ASSERT (pCommandPool != NULL);
        R_CVULKAN_ASSERT (pDevice != NULL);

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

        pCommandPool->device = R_CVulkan_DeviceGetHandle (pDevice);
        pCommandPool->handle = VK_NULL_HANDLE;
        pCommandPool->queueFamilyIndex = queueFamilyIndex;
#if defined(R_CVULKAN_DEBUG)
        pCommandPool->isInitialized = false;
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
        pCommandPool->isInitialized = true;
#endif
        return R_CVULKAN_ERROR_OK;
}

R_CVULKAN_API void
R_CVulkan_DeleteCommandPool (struct R_CVulkan_CommandPool* pCommandPool)
{
        R_CVULKAN_ASSERT (pCommandPool != NULL);

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
        pCommandPool->isInitialized = false;
#endif
        pCommandPool->device = VK_NULL_HANDLE;
        pCommandPool->queueFamilyIndex = 0;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_CommandPoolReset (struct R_CVulkan_CommandPool* pCommandPool, R_CVulkanCommandPoolResetFlags flags)
{
        R_CVULKAN_ASSERT (pCommandPool != NULL);

        if (!pCommandPool)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pCommandPool->isInitialized);
        if (!pCommandPool->isInitialized)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        VkResult result = vkResetCommandPool (pCommandPool->device, pCommandPool->handle, flags);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_FAILED;
        }

        return R_CVULKAN_ERROR_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_CommandPoolTrim (struct R_CVulkan_CommandPool* pCommandPool)
{
        R_CVULKAN_ASSERT (pCommandPool != NULL);

        if (!pCommandPool)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pCommandPool->isInitialized);
        if (!pCommandPool->isInitialized)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif
        (void)pCommandPool;
        return R_CVULKAN_ERROR_OK;
}

R_CVULKAN_API VkCommandPool
R_CVulkan_CommandPoolGetHandle (const struct R_CVulkan_CommandPool* pCommandPool)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pCommandPool != NULL);
#endif
        return pCommandPool->handle;
}

R_CVULKAN_API VkDevice
R_CVulkan_CommandPoolGetDevice (const struct R_CVulkan_CommandPool* pCommandPool)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pCommandPool != NULL);
#endif
        return pCommandPool->device;
}

R_CVULKAN_API uint32_t
R_CVulkan_CommandPoolGetQueueFamilyIndex (const struct R_CVulkan_CommandPool* pCommandPool)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pCommandPool != NULL);
#endif
        return pCommandPool->queueFamilyIndex;
}

R_CVULKAN_API int
R_CVulkan_CommandPoolIsInitialized (const struct R_CVulkan_CommandPool* pCommandPool)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pCommandPool != NULL);
        return pCommandPool->isInitialized;
#else
        (void)pCommandPool;
        return 1;
#endif
}
