#include "rlgame.base/cvulkan/cvulkan_fence.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <stdlib.h>
#include <inttypes.h>
#include <vulkan/vulkan.h>

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_NewFence (struct R_CVulkan_Fence* pFence, const struct R_CVulkan_Device* pDevice, bool signaled)
{
        R_CVULKAN_ASSERT (pFence );
        R_CVULKAN_ASSERT (pDevice );

#if defined(R_CVULKAN_DEBUG)
        if (!R_CVulkan_DeviceIsInitialized (pDevice))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        pFence->device = R_CVulkan_DeviceGetLogicalDevice (pDevice);
        pFence->handle = VK_NULL_HANDLE;
#if defined(R_CVULKAN_DEBUG)
        pFence->booted = false;
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
        pFence->booted = true;
#endif
        return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_DeleteFence (struct R_CVulkan_Fence* pFence)
{
        R_CVULKAN_ASSERT (pFence );

        vkDestroyFence (pFence->device, pFence->handle, NULL);
#if defined(R_CVULKAN_DEBUG)
        pFence->device = VK_NULL_HANDLE;
        pFence->booted = false;
#endif
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_FenceWait (
    const struct R_CVulkan_Device* pDevice,
    const struct R_CVulkan_Fence*  pFences,
    uint32_t                       fenceCount,
    int                            waitAll,
    uint64_t                       timeout)
{
        R_CVULKAN_ASSERT (pDevice );
        R_CVULKAN_ASSERT (pFences );
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
        size_t   nativeFencesSize = fenceCount * sizeof (VkFence);
        VkFence* nativeFences = (VkFence*)R_CSTL_HeapAlloc (nativeFencesSize);
        if (!nativeFences)
        {
                return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }

        for (uint32_t i = 0; i < fenceCount; ++i)
        {
#if defined(R_CVULKAN_DEBUG)
                R_CVULKAN_ASSERT (pFences[i].booted);
                if (!pFences[i].booted || pFences[i].handle == VK_NULL_HANDLE)
                {
                        R_CSTL_HeapFree (nativeFences);
                        return R_CVULKAN_ERROR_NOT_INITIALIZED;
                }
#endif
                nativeFences[i] = pFences[i].handle;
        }

        VkResult result = vkWaitForFences (
            R_CVulkan_DeviceGetLogicalDevice (pDevice),
            fenceCount,
            nativeFences,
            waitAll ? VK_TRUE : VK_FALSE,
            timeout);
        R_CSTL_HeapFree(nativeFences);

        if (result == VK_SUCCESS)
        {
                return R_CVULKAN_OK;
        }
        else if (result == VK_TIMEOUT)
        {
                return R_CVULKAN_ERROR_FAILED;
        }
        else
        {
                return R_CVulkan_ResultToError (result);
        }
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_FenceReset (
    const struct R_CVulkan_Device* pDevice,
    const struct R_CVulkan_Fence*  pFences,
    uint32_t                       fenceCount)
{
        R_CVULKAN_ASSERT (pDevice );
        R_CVULKAN_ASSERT (pFences );
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

        VkFence* nativeFences = (VkFence*)R_CSTL_HeapAlloc (fenceCount * sizeof (VkFence));
        if (!nativeFences)
        {
                return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }

        for (uint32_t i = 0; i < fenceCount; ++i)
        {
#if defined(R_CVULKAN_DEBUG)
                R_CVULKAN_ASSERT (pFences[i].booted);
                if (!pFences[i].booted || pFences[i].handle == VK_NULL_HANDLE)
                {
                        R_CSTL_HeapFree (nativeFences);
                        return R_CVULKAN_ERROR_NOT_INITIALIZED;
                }
#endif
                nativeFences[i] = pFences[i].handle;
        }

        VkResult result
            = vkResetFences (R_CVulkan_DeviceGetLogicalDevice (pDevice), fenceCount, nativeFences);

        R_CSTL_HeapFree (nativeFences);

        if (result == VK_SUCCESS)
        {
                return R_CVULKAN_OK;
        }
        else
        {
                return R_CVulkan_ResultToError (result);
        }
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_FenceGetStatus (
    const struct R_CVulkan_Device* pDevice,
    const struct R_CVulkan_Fence*  pFence,
    bool*                          pOutSignaled)
{
        R_CVULKAN_ASSERT (pDevice );
        R_CVULKAN_ASSERT (pFence );
        R_CVULKAN_ASSERT (pOutSignaled );

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

        R_CVULKAN_ASSERT (pFence->booted);
        if (!pFence->booted || pFence->handle == VK_NULL_HANDLE)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        VkResult result = vkGetFenceStatus (R_CVulkan_DeviceGetLogicalDevice (pDevice), pFence->handle);

        if (result == VK_SUCCESS)
        {
                *pOutSignaled = true;
                return R_CVULKAN_OK;
        }
        else if (result == VK_NOT_READY)
        {
                *pOutSignaled = false;
                return R_CVULKAN_OK;
        }
        else
        {
                return R_CVulkan_ResultToError (result);
        }
}

R_CVULKAN_API VkFence
R_CVulkan_FenceGetHandle (const struct R_CVulkan_Fence* pFence)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pFence );
#endif
        return pFence->handle;
}

R_CVULKAN_API VkDevice
R_CVulkan_FenceGetDevice (const struct R_CVulkan_Fence* pFence)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pFence );
#endif
        return pFence->device;
}

R_CVULKAN_API int
R_CVulkan_FenceIsInitialized (const struct R_CVulkan_Fence* pFence)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pFence );
        return pFence->booted;
#else
        (void)pFence;
        return 1;
#endif
}
