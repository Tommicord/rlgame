#include "rlgame.base/cvulkan/cvulkan_fence.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

#include <stdlib.h>
#include <inttypes.h>
#include <vulkan/vulkan.h>

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_NewFence (struct R_CVulkan_Fence* pFence, const struct R_CVulkan_Device* pDevice, int signaled)
{
        R_CVULKAN_ASSERT (pFence != NULL);
        R_CVULKAN_ASSERT (pDevice != NULL);

#if defined(R_CVULKAN_DEBUG)
        if (!pFence || !pDevice)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }
        R_CVULKAN_ASSERT (R_CVulkan_DeviceIsInitialized (pDevice));
        if (!R_CVulkan_DeviceIsInitialized (pDevice))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        pFence->device = R_CVulkan_DeviceGetHandle (pDevice);
        pFence->handle = VK_NULL_HANDLE;
#if defined(R_CVULKAN_DEBUG)
        pFence->isInitialized = false;
#endif

        VkFenceCreateInfo fenceInfo = {0};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

        VkResult result = vkCreateFence (pFence->device, &fenceInfo, NULL, &pFence->handle);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_FAILED;
        }

#if defined(R_CVULKAN_DEBUG)
        pFence->isInitialized = true;
#endif
        return R_CVULKAN_ERROR_OK;
}

R_CVULKAN_API void
R_CVulkan_DeleteFence (struct R_CVulkan_Fence* pFence)
{
        R_CVULKAN_ASSERT (pFence != NULL);

#if defined(R_CVULKAN_DEBUG)
        if (!pFence)
        {
                return;
        }
#endif

        if (pFence->handle != VK_NULL_HANDLE)
        {
                vkDestroyFence (pFence->device, pFence->handle, NULL);
                pFence->handle = VK_NULL_HANDLE;
        }

#if defined(R_CVULKAN_DEBUG)
        pFence->device = VK_NULL_HANDLE;
        pFence->isInitialized = false;
#endif
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_FenceWait (
    const struct R_CVulkan_Device* pDevice,
    const struct R_CVulkan_Fence*  pFences,
    uint32_t                       fenceCount,
    int                            waitAll,
    uint64_t                       timeout)
{
        R_CVULKAN_ASSERT (pDevice != NULL);
        R_CVULKAN_ASSERT (pFences != NULL);
        R_CVULKAN_ASSERT (fenceCount > 0);

#if defined(R_CVULKAN_DEBUG)
        if (!pDevice || !pFences || fenceCount == 0)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        R_CVULKAN_ASSERT (R_CVulkan_DeviceIsInitialized (pDevice));
        if (!R_CVulkan_DeviceIsInitialized (pDevice))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        VkFence* nativeFences = (VkFence*)malloc (fenceCount * sizeof (VkFence));
        if (!nativeFences)
        {
                return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }

        for (uint32_t i = 0; i < fenceCount; ++i)
        {
#if defined(R_CVULKAN_DEBUG)
                R_CVULKAN_ASSERT (pFences[i].isInitialized);
                if (!pFences[i].isInitialized || pFences[i].handle == VK_NULL_HANDLE)
                {
                        free (nativeFences);
                        return R_CVULKAN_ERROR_NOT_INITIALIZED;
                }
#endif
                nativeFences[i] = pFences[i].handle;
        }

        VkResult result = vkWaitForFences (
            R_CVulkan_DeviceGetHandle (pDevice),
            fenceCount,
            nativeFences,
            waitAll ? VK_TRUE : VK_FALSE,
            timeout);

        free (nativeFences);

        if (result == VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_OK;
        }
        else if (result == VK_TIMEOUT)
        {
                return R_CVULKAN_ERROR_FAILED;
        }
        else
        {
                return R_CVULKAN_ResultToError (result);
        }
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_FenceReset (
    const struct R_CVulkan_Device* pDevice,
    const struct R_CVulkan_Fence*  pFences,
    uint32_t                       fenceCount)
{
        R_CVULKAN_ASSERT (pDevice != NULL);
        R_CVULKAN_ASSERT (pFences != NULL);
        R_CVULKAN_ASSERT (fenceCount > 0);

#if defined(R_CVULKAN_DEBUG)
        if (!pDevice || !pFences || fenceCount == 0)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        R_CVULKAN_ASSERT (R_CVulkan_DeviceIsInitialized (pDevice));
        if (!R_CVulkan_DeviceIsInitialized (pDevice))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        VkFence* nativeFences = (VkFence*)malloc (fenceCount * sizeof (VkFence));
        if (!nativeFences)
        {
                return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }

        for (uint32_t i = 0; i < fenceCount; ++i)
        {
#if defined(R_CVULKAN_DEBUG)
                R_CVULKAN_ASSERT (pFences[i].isInitialized);
                if (!pFences[i].isInitialized || pFences[i].handle == VK_NULL_HANDLE)
                {
                        free (nativeFences);
                        return R_CVULKAN_ERROR_NOT_INITIALIZED;
                }
#endif
                nativeFences[i] = pFences[i].handle;
        }

        VkResult result = vkResetFences (R_CVulkan_DeviceGetHandle (pDevice), fenceCount, nativeFences);

        free (nativeFences);

        if (result == VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_OK;
        }
        else
        {
                return R_CVULKAN_ResultToError (result);
        }
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_FenceGetStatus (
    const struct R_CVulkan_Device* pDevice,
    const struct R_CVulkan_Fence*  pFence,
    int*                           pOutSignaled)
{
        R_CVULKAN_ASSERT (pDevice != NULL);
        R_CVULKAN_ASSERT (pFence != NULL);
        R_CVULKAN_ASSERT (pOutSignaled != NULL);

#if defined(R_CVULKAN_DEBUG)
        if (!pDevice || !pFence || !pOutSignaled)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }
        R_CVULKAN_ASSERT (R_CVulkan_DeviceIsInitialized (pDevice));
        if (!R_CVulkan_DeviceIsInitialized (pDevice))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }

        R_CVULKAN_ASSERT (pFence->isInitialized);
        if (!pFence->isInitialized || pFence->handle == VK_NULL_HANDLE)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        VkResult result = vkGetFenceStatus (R_CVulkan_DeviceGetHandle (pDevice), pFence->handle);

        if (result == VK_SUCCESS)
        {
                *pOutSignaled = 1;
                return R_CVULKAN_ERROR_OK;
        }
        else if (result == VK_NOT_READY)
        {
                *pOutSignaled = 0;
                return R_CVULKAN_ERROR_OK;
        }
        else
        {
                return R_CVULKAN_ResultToError (result);
        }
}

R_CVULKAN_API VkFence
R_CVulkan_FenceGetHandle (const struct R_CVulkan_Fence* pFence)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pFence != NULL);
#endif
        return pFence->handle;
}

R_CVULKAN_API VkDevice
R_CVulkan_FenceGetDevice (const struct R_CVulkan_Fence* pFence)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pFence != NULL);
#endif
        return pFence->device;
}

R_CVULKAN_API int
R_CVulkan_FenceIsInitialized (const struct R_CVulkan_Fence* pFence)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pFence != NULL);
        return pFence->isInitialized;
#else
        (void)pFence;
        return 1;
#endif
}
