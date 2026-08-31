#include "rlgame.base/cvulkan/cvulkan_fence.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <stdlib.h>
#include <inttypes.h>
#include <vulkan/vulkan.h>

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_NewFence (struct R_CVulkan_Fence* pFence, const struct R_CVulkan_Device* pDevice, bool signaled)
{
    R_CVULKAN_ASSERT (pFence);
    R_CVULKAN_ASSERT (pDevice);

    pFence->device = r_cvulkan_device_get_logical_device (pDevice);
    VkFenceCreateInfo fenceInfo = {0};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    VkResult result = vkCreateFence (pFence->device, &fenceInfo, NULL, &pFence->handle);
    if (result != VK_SUCCESS)
    {
        return R_CVULKAN_ERROR_FAILED;
    }
    return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_DeleteFence (struct R_CVulkan_Fence* pFence)
{
    R_CVULKAN_ASSERT (pFence);
    vkDestroyFence (pFence->device, pFence->handle, NULL);
#if defined(R_CVULKAN_DEBUG)
    pFence->device = VK_NULL_HANDLE;
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
    R_CVULKAN_ASSERT (pDevice);
    R_CVULKAN_ASSERT (pFences);
    R_CVULKAN_ASSERT (fenceCount > 0);
    size_t   nativeFencesSize = fenceCount * sizeof (VkFence);
    VkFence* nativeFences = (VkFence*)r_cstl_heap_alloc (nativeFencesSize);
    if (!nativeFences)
    {
        return R_CVULKAN_ERROR_OUT_OF_MEMORY;
    }

    for (uint32_t i = 0; i < fenceCount; ++i)
    {
        if (pFences[i].handle == VK_NULL_HANDLE)
        {
            r_cstl_heap_free (nativeFences);
            return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
        nativeFences[i] = pFences[i].handle;
    }

    VkResult result = vkWaitForFences (
        r_cvulkan_device_get_logical_device (pDevice),
        fenceCount,
        nativeFences,
        waitAll ? VK_TRUE : VK_FALSE,
        timeout);
    r_cstl_heap_free (nativeFences);

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
        return r_cvulkan_result_to_error (result);
    }
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_FenceReset (
    const struct R_CVulkan_Device* pDevice,
    const struct R_CVulkan_Fence*  pFences,
    uint32_t                       fenceCount)
{
    R_CVULKAN_ASSERT (pDevice);
    R_CVULKAN_ASSERT (pFences);
    R_CVULKAN_ASSERT (fenceCount > 0);
    VkFence* nativeFences = (VkFence*)r_cstl_heap_alloc (fenceCount * sizeof (VkFence));
    if (!nativeFences)
    {
        return R_CVULKAN_ERROR_OUT_OF_MEMORY;
    }

    for (uint32_t i = 0; i < fenceCount; ++i)
    {
        if (pFences[i].handle == VK_NULL_HANDLE)
        {
            r_cstl_heap_free (nativeFences);
            return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
        nativeFences[i] = pFences[i].handle;
    }

    VkResult result = vkResetFences (r_cvulkan_device_get_logical_device (pDevice), fenceCount, nativeFences);

    r_cstl_heap_free (nativeFences);

    if (result == VK_SUCCESS)
    {
        return R_CVULKAN_OK;
    }
    else
    {
        return r_cvulkan_result_to_error (result);
    }
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_fence_get_status (
    const struct R_CVulkan_Device* pDevice,
    const struct R_CVulkan_Fence*  pFence,
    bool*                          pOutSignaled)
{
    R_CVULKAN_ASSERT (pDevice);
    R_CVULKAN_ASSERT (pFence);
    R_CVULKAN_ASSERT (pFence->handle);
    R_CVULKAN_ASSERT (pOutSignaled);
    VkResult result = vkGetFenceStatus (r_cvulkan_device_get_logical_device (pDevice), pFence->handle);

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
        return r_cvulkan_result_to_error (result);
    }
}

R_CVULKAN_API VkFence
r_cvulkan_fence_get_handle (const struct R_CVulkan_Fence* pFence)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pFence);
#endif
    return pFence->handle;
}

R_CVULKAN_API VkDevice
r_cvulkan_fence_get_device (const struct R_CVulkan_Fence* pFence)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pFence);
#endif
    return pFence->device;
}