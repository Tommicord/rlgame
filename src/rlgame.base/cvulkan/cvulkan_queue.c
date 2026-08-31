#include "rlgame.base/cvulkan/cvulkan_queue.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_command_buffer.h"
#include "rlgame.base/cvulkan/cvulkan_semaphore.h"
#include "rlgame.base/cvulkan/cvulkan_fence.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <string.h>

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_NewQueue (
    struct R_CVulkan_Queue*        pQueue,
    const struct R_CVulkan_Device* pDevice,
    uint32_t                       queueFamilyIndex,
    uint32_t                       queueIndex)
{
    R_CVULKAN_ASSERT (pQueue);
    R_CVULKAN_ASSERT (pDevice);

    pQueue->device = r_cvulkan_device_get_logical_device (pDevice);
    pQueue->handle = VK_NULL_HANDLE;
    pQueue->queueFamilyIndex = queueFamilyIndex;
    pQueue->queueIndex = queueIndex;
    vkGetDeviceQueue (pQueue->device, queueFamilyIndex, queueIndex, &pQueue->handle);
    if (pQueue->handle == VK_NULL_HANDLE)
    {
        return R_CVULKAN_ERROR_FAILED;
    }
    return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_DeleteQueue (struct R_CVulkan_Queue* pQueue)
{
    R_CVULKAN_ASSERT (pQueue);
#if defined(R_CVULKAN_DEBUG)
    pQueue->handle = VK_NULL_HANDLE;
    pQueue->device = VK_NULL_HANDLE;
    pQueue->queueFamilyIndex = 0;
    pQueue->queueIndex = 0;
#else
    (void)pQueue;
#endif
}

R_CVULKAN_API enum R_CVulkan_Error
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

    enum R_CVulkan_Error error;
    VkSubmitInfo         submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkCommandBuffer* pNativeCommandBuffers = NULL;

    if (commandBufferCount > 0)
    {
        pNativeCommandBuffers
            = (VkCommandBuffer*)r_cstl_heap_alloc (commandBufferCount * sizeof (VkCommandBuffer));
        if (!pNativeCommandBuffers)
        {
            return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }

        for (uint32_t i = 0; i < commandBufferCount; ++i)
        {
            pNativeCommandBuffers[i] = r_cvulkan_command_buffer_get_handle (&pCommandBuffers[i]);
        }

        submitInfo.commandBufferCount = commandBufferCount;
        submitInfo.pCommandBuffers = pNativeCommandBuffers;
    }
    VkSemaphore*          nativeWaitSemaphores = NULL;
    VkPipelineStageFlags* nativeWaitStageMask = NULL;
    if (waitSemaphoreCount > 0)
    {
        nativeWaitSemaphores = (VkSemaphore*)r_cstl_heap_alloc (waitSemaphoreCount * sizeof (VkSemaphore));
        if (!nativeWaitSemaphores)
        {
            r_cstl_heap_free (pNativeCommandBuffers);
            return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }

        nativeWaitStageMask
            = (VkPipelineStageFlags*)r_cstl_heap_alloc (waitSemaphoreCount * sizeof (VkPipelineStageFlags));
        if (!nativeWaitStageMask)
        {
            r_cstl_heap_free (pNativeCommandBuffers);
            r_cstl_heap_free (nativeWaitSemaphores);
            return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }

        for (uint32_t i = 0; i < waitSemaphoreCount; ++i)
        {
            nativeWaitSemaphores[i] = r_cvulkan_semaphore_get_handle (&pWaitSemaphores[i]);
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
            = (VkSemaphore*)r_cstl_heap_alloc (signalSemaphoreCount * sizeof (VkSemaphore));
        if (!pNativeSignalSemaphores)
        {
            r_cstl_heap_free (pNativeCommandBuffers);
            r_cstl_heap_free (nativeWaitSemaphores);
            r_cstl_heap_free (nativeWaitStageMask);
            return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }
        for (uint32_t i = 0; i < signalSemaphoreCount; ++i)
        {
            pNativeSignalSemaphores[i] = r_cvulkan_semaphore_get_handle (&pSignalSemaphores[i]);
        }
        submitInfo.signalSemaphoreCount = signalSemaphoreCount;
        submitInfo.pSignalSemaphores = pNativeSignalSemaphores;
    }
    VkFence nativeFence = VK_NULL_HANDLE;
    if (pFence)
    {
        nativeFence = r_cvulkan_fence_get_handle (pFence);
        VkResult resetResult = vkResetFences (pQueue->device, 1, &nativeFence);
        if (resetResult != VK_SUCCESS)
        {
            R_CSTL_LOG_ERROR ("R_CVulkan_QueueSubmit: Failed to reset fence: %d", resetResult);
            error = r_cvulkan_result_to_error (resetResult);
            goto r_cleanup;
        }
    }
    VkResult submitResult = vkQueueSubmit (pQueue->handle, 1, &submitInfo, nativeFence);
r_cleanup:
    r_cstl_heap_free (pNativeCommandBuffers);
    r_cstl_heap_free (nativeWaitSemaphores);
    r_cstl_heap_free (nativeWaitStageMask);
    r_cstl_heap_free (pNativeSignalSemaphores);
    error = r_cvulkan_result_to_error (submitResult);
    return error;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_QueuePresent (
    struct R_CVulkan_Queue*           pQueue,
    const VkSwapchainKHR*             pSwapchains,
    uint32_t                          swapchainCount,
    const uint32_t*                   pImageIndices,
    const struct R_CVulkan_Semaphore* pWaitSemaphores,
    uint32_t                          waitSemaphoreCount)
{
    R_CVULKAN_ASSERT (pQueue);
    R_CVULKAN_ASSERT (swapchainCount > 0);
    R_CVULKAN_ASSERT (pSwapchains);
    R_CVULKAN_ASSERT (pImageIndices);

    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    VkSemaphore* pNativeWaitSemaphores = NULL;
    if (waitSemaphoreCount > 0)
    {
        pNativeWaitSemaphores = (VkSemaphore*)r_cstl_heap_alloc (waitSemaphoreCount * sizeof (VkSemaphore));
        if (!pNativeWaitSemaphores)
        {
            return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }
        for (uint32_t i = 0; i < waitSemaphoreCount; ++i)
        {
            pNativeWaitSemaphores[i] = r_cvulkan_semaphore_get_handle (&pWaitSemaphores[i]);
        }
        presentInfo.waitSemaphoreCount = waitSemaphoreCount;
        presentInfo.pWaitSemaphores = pNativeWaitSemaphores;
    }
    presentInfo.swapchainCount = swapchainCount;
    presentInfo.pSwapchains = pSwapchains;
    presentInfo.pImageIndices = pImageIndices;

    VkResult result = vkQueuePresentKHR (pQueue->handle, &presentInfo);
    r_cstl_heap_free (pNativeWaitSemaphores);

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
        return r_cvulkan_result_to_error (result);
    }
}

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_queue_wait_idle (struct R_CVulkan_Queue* pQueue)
{
    R_CVULKAN_ASSERT (pQueue);
    VkResult result = vkQueueWaitIdle (pQueue->handle);

    if (result == VK_SUCCESS)
    {
        return R_CVULKAN_OK;
    }
    else
    {
        return r_cvulkan_result_to_error (result);
    }
}

R_CVULKAN_API VkQueue
r_cvulkan_queue_get_handle (const struct R_CVulkan_Queue* pQueue)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pQueue);
#endif
    return pQueue->handle;
}

R_CVULKAN_API VkDevice
r_cvulkan_queue_get_device (const struct R_CVulkan_Queue* pQueue)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pQueue);
#endif
    return pQueue->device;
}

R_CVULKAN_API uint32_t
r_cvulkan_queue_get_family_index (const struct R_CVulkan_Queue* pQueue)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pQueue);
#endif
    return pQueue->queueFamilyIndex;
}

R_CVULKAN_API uint32_t
r_cvulkan_queue_get_index (const struct R_CVulkan_Queue* pQueue)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pQueue);
#endif
    return pQueue->queueIndex;
}