#include "rlgame.base/cvulkan/cvulkan_queue.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_command_buffer.h"
#include "rlgame.base/cvulkan/cvulkan_semaphore.h"
#include "rlgame.base/cvulkan/cvulkan_fence.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <string.h>

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_NewQueue (
    struct R_CVulkan_Queue*        pQueue,
    const struct R_CVulkan_Device* pDevice,
    uint32_t                       queueFamilyIndex,
    uint32_t                       queueIndex)
{
    R_CVULKAN_ASSERT (pQueue);
    R_CVULKAN_ASSERT (pDevice);

#if defined(R_CVULKAN_DEBUG)
    if (!pQueue || !pDevice)
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }
    R_CVULKAN_ASSERT (R_CVulkan_DeviceIsInitialized (pDevice));
    if (!R_CVulkan_DeviceIsInitialized (pDevice))
    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }
#endif

    pQueue->device = R_CVulkan_DeviceGetLogicalDevice (pDevice);
    pQueue->handle = VK_NULL_HANDLE;
    pQueue->queueFamilyIndex = queueFamilyIndex;
    pQueue->queueIndex = queueIndex;
#if defined(R_CVULKAN_DEBUG)
    
#endif
    vkGetDeviceQueue (pQueue->device, queueFamilyIndex, queueIndex, &pQueue->handle);

    if (pQueue->handle == VK_NULL_HANDLE)
    {
        return R_CVULKAN_ERROR_FAILED;
    }

#if defined(R_CVULKAN_DEBUG)
    
#endif
    return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_DeleteQueue (struct R_CVulkan_Queue* pQueue)
{
    R_CVULKAN_ASSERT (pQueue);
#if defined(R_CVULKAN_DEBUG)
    if (!pQueue)
    {
        return;
    }
    // Queues are destroyed when the device is destroyed
    // so we just clear the state
    pQueue->handle = VK_NULL_HANDLE;
    pQueue->device = VK_NULL_HANDLE;
    pQueue->queueFamilyIndex = 0;
    pQueue->queueIndex = 0;
    
#else
    (void)pQueue;
#endif
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_QueueSubmit (
    struct R_CVulkan_Queue*               pQueue,
    const struct R_CVulkan_CommandBuffer* pCommandBuffers,
    uint32_t                              commandBufferCount,
    const struct R_CVulkan_Semaphore*     pWaitSemaphores,
    uint32_t                              waitSemaphoreCount,
    const VkPipelineStageFlags*           pWaitDstStageMask,
    const struct R_CVulkan_Semaphore*     pSignalSemaphores,
    uint32_t                              signalSemaphoreCount,
    const struct R_CVulkan_Fence*         pFence)
{
    R_CVULKAN_ASSERT (pQueue);
    R_CVULKAN_ASSERT (commandBufferCount > 0);
    R_CVULKAN_ASSERT (pCommandBuffers);
#if defined(R_CVULKAN_DEBUG)
    if (!R_CVulkan_QueueIsInitialized (pQueue))
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }
#endif
    enum R_CVulkanError error;

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkCommandBuffer* pNativeCommandBuffers = NULL;

    if (commandBufferCount > 0)
    {
        pNativeCommandBuffers
            = (VkCommandBuffer*)R_CSTL_HeapAlloc (commandBufferCount * sizeof (VkCommandBuffer));
        if (!pNativeCommandBuffers)
        {
            return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }

        for (uint32_t i = 0; i < commandBufferCount; ++i)
        {
            if (!R_CVulkan_CommandBufferIsInitialized (&pCommandBuffers[i]))
            {
                R_CSTL_HeapFree (pNativeCommandBuffers);
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
            }
            pNativeCommandBuffers[i] = R_CVulkan_CommandBufferGetHandle (&pCommandBuffers[i]);
        }

        submitInfo.commandBufferCount = commandBufferCount;
        submitInfo.pCommandBuffers = pNativeCommandBuffers;
    }
    VkSemaphore*          nativeWaitSemaphores = NULL;
    VkPipelineStageFlags* nativeWaitStageMask = NULL;
    if (waitSemaphoreCount > 0)
    {
        nativeWaitSemaphores = (VkSemaphore*)R_CSTL_HeapAlloc (waitSemaphoreCount * sizeof (VkSemaphore));
        if (!nativeWaitSemaphores)
        {
            R_CSTL_HeapFree (pNativeCommandBuffers);
            return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }

        nativeWaitStageMask
            = (VkPipelineStageFlags*)R_CSTL_HeapAlloc (waitSemaphoreCount * sizeof (VkPipelineStageFlags));
        if (!nativeWaitStageMask)
        {
            R_CSTL_HeapFree (pNativeCommandBuffers);
            R_CSTL_HeapFree (nativeWaitSemaphores);
            return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }

        for (uint32_t i = 0; i < waitSemaphoreCount; ++i)
        {
            if (!R_CVulkan_SemaphoreIsInitialized (&pWaitSemaphores[i]))
            {
                R_CSTL_HeapFree (pNativeCommandBuffers);
                R_CSTL_HeapFree (nativeWaitSemaphores);
                R_CSTL_HeapFree (nativeWaitStageMask);
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
            }
            nativeWaitSemaphores[i] = R_CVulkan_SemaphoreGetHandle (&pWaitSemaphores[i]);
            nativeWaitStageMask[i] = pWaitDstStageMask[i];
        }

        submitInfo.waitSemaphoreCount = waitSemaphoreCount;
        submitInfo.pWaitSemaphores = nativeWaitSemaphores;
        submitInfo.pWaitDstStageMask = nativeWaitStageMask;
    }
    VkSemaphore* pNativeSignalSemaphores = NULL;
    if (signalSemaphoreCount > 0)
    {
        pNativeSignalSemaphores
            = (VkSemaphore*)R_CSTL_HeapAlloc (signalSemaphoreCount * sizeof (VkSemaphore));
        if (!pNativeSignalSemaphores)
        {
            R_CSTL_HeapFree (pNativeCommandBuffers);
            R_CSTL_HeapFree (nativeWaitSemaphores);
            R_CSTL_HeapFree (nativeWaitStageMask);
            return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }

        for (uint32_t i = 0; i < signalSemaphoreCount; ++i)
        {
            if (!R_CVulkan_SemaphoreIsInitialized (&pSignalSemaphores[i]))
            {
                R_CSTL_HeapFree (pNativeCommandBuffers);
                R_CSTL_HeapFree (nativeWaitSemaphores);
                R_CSTL_HeapFree (nativeWaitStageMask);
                R_CSTL_HeapFree (pNativeSignalSemaphores);
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
            }
            pNativeSignalSemaphores[i] = R_CVulkan_SemaphoreGetHandle (&pSignalSemaphores[i]);
        }

        submitInfo.signalSemaphoreCount = signalSemaphoreCount;
        submitInfo.pSignalSemaphores = pNativeSignalSemaphores;
    }
    VkFence nativeFence = VK_NULL_HANDLE;
    if (pFence && R_CVulkan_FenceIsInitialized (pFence))
    {
        nativeFence = R_CVulkan_FenceGetHandle (pFence);
        VkResult resetResult = vkResetFences (pQueue->device, 1, &nativeFence);
        if (resetResult != VK_SUCCESS)
        {
            R_CSTL_LOG_ERROR ("R_CVulkan_QueueSubmit: Failed to reset fence: %d", resetResult);
            error = R_CVulkan_ResultToError (resetResult);
            goto r_cleanup;
        }
    }
    VkResult submitResult = vkQueueSubmit (pQueue->handle, 1, &submitInfo, nativeFence);
r_cleanup:
    R_CSTL_HeapFree (pNativeCommandBuffers);
    R_CSTL_HeapFree (nativeWaitSemaphores);
    R_CSTL_HeapFree (nativeWaitStageMask);
    R_CSTL_HeapFree (pNativeSignalSemaphores);
    error = R_CVulkan_ResultToError (submitResult);
    return error;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_QueuePresent (
    struct R_CVulkan_Queue*           pQueue,
    const VkSwapchainKHR*             pSwapchains,
    uint32_t                          swapchainCount,
    const uint32_t*                   pImageIndices,
    const struct R_CVulkan_Semaphore* pWaitSemaphores,
    uint32_t                          waitSemaphoreCount)
{
    R_CVULKAN_ASSERT (pQueue);
#if defined(R_CVULKAN_DEBUG)
    if (!pQueue || !R_CVulkan_QueueIsInitialized (pQueue))
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }

    if (swapchainCount > 0 && (!pSwapchains || !pImageIndices))
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }
#endif
    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    VkSemaphore* pNativeWaitSemaphores = NULL;
    if (waitSemaphoreCount > 0)
    {
        pNativeWaitSemaphores = (VkSemaphore*)R_CSTL_HeapAlloc (waitSemaphoreCount * sizeof (VkSemaphore));
        if (!pNativeWaitSemaphores)
        {
            return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }

        for (uint32_t i = 0; i < waitSemaphoreCount; ++i)
        {
            if (!R_CVulkan_SemaphoreIsInitialized (&pWaitSemaphores[i]))
            {
                R_CSTL_HeapFree (pNativeWaitSemaphores);
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
            }
            pNativeWaitSemaphores[i] = R_CVulkan_SemaphoreGetHandle (&pWaitSemaphores[i]);
        }

        presentInfo.waitSemaphoreCount = waitSemaphoreCount;
        presentInfo.pWaitSemaphores = pNativeWaitSemaphores;
    }

    presentInfo.swapchainCount = swapchainCount;
    presentInfo.pSwapchains = pSwapchains;
    presentInfo.pImageIndices = pImageIndices;

    VkResult result = vkQueuePresentKHR (pQueue->handle, &presentInfo);
    R_CSTL_HeapFree (pNativeWaitSemaphores);

    if (result == VK_SUCCESS)
    {
        return R_CVULKAN_OK;
    }
    else if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        return R_CVULKAN_ERROR_SWAPCHAIN_OUT_OF_DATE;
    }
    else
    {
        return R_CVulkan_ResultToError (result);
    }
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_QueueWaitIdle (struct R_CVulkan_Queue* pQueue)
{
    R_CVULKAN_ASSERT (pQueue);

#if defined(R_CVULKAN_DEBUG)
    if (!pQueue || !R_CVulkan_QueueIsInitialized (pQueue))
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }
#endif
    VkResult result = vkQueueWaitIdle (pQueue->handle);

    if (result == VK_SUCCESS)
    {
        return R_CVULKAN_OK;
    }
    else
    {
        return R_CVulkan_ResultToError (result);
    }
}

R_CVULKAN_API VkQueue
R_CVulkan_QueueGetHandle (const struct R_CVulkan_Queue* pQueue)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pQueue);
#endif
    return pQueue->handle;
}

R_CVULKAN_API VkDevice
R_CVulkan_QueueGetDevice (const struct R_CVulkan_Queue* pQueue)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pQueue);
#endif
    return pQueue->device;
}

R_CVULKAN_API uint32_t
R_CVulkan_QueueGetFamilyIndex (const struct R_CVulkan_Queue* pQueue)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pQueue);
#endif
    return pQueue->queueFamilyIndex;
}

R_CVULKAN_API uint32_t
R_CVulkan_QueueGetIndex (const struct R_CVulkan_Queue* pQueue)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pQueue);
#endif
    return pQueue->queueIndex;
}

R_CVULKAN_API int
R_CVulkan_QueueIsInitialized (const struct R_CVulkan_Queue* pQueue)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pQueue);
    return 1;
#else
    (void)pQueue;
    return 1;
#endif
}
