#include "rlgame.base/cvulkan/cvulkan_semaphore.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_Semaphore_Init (
    struct R_CVulkan_Semaphore* pSemaphore,
    const struct R_CVulkan_Device* pDevice,
    int                         timelineSemaphore,
    uint64_t                    initialValue)
{
        R_CVULKAN_ASSERT (pSemaphore != NULL);
        R_CVULKAN_ASSERT (pDevice != NULL);

#if defined(R_CVULKAN_DEBUG)
        if (!pSemaphore || !pDevice)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }
        if (!R_CVulkan_DeviceIsInitialized (pDevice))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        pSemaphore->device = R_CVulkan_DeviceGetLogicalDevice (pDevice);
        pSemaphore->handle = VK_NULL_HANDLE;
#if defined(R_CVULKAN_DEBUG)
        pSemaphore->isInitialized = false;
#endif

        VkSemaphoreCreateInfo semaphoreInfo = {0};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        if (timelineSemaphore)
        {
                VkSemaphoreTypeCreateInfo timelineCreateInfo = {0};
                timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
                timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
                timelineCreateInfo.initialValue = initialValue;
                semaphoreInfo.pNext = &timelineCreateInfo;
        }

        VkResult result = vkCreateSemaphore (pSemaphore->device, &semaphoreInfo, NULL, &pSemaphore->handle);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_FAILED;
        }

#if defined(R_CVULKAN_DEBUG)
        pSemaphore->isInitialized = true;
#endif
        return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_SemaphoreShutdown (struct R_CVulkan_Semaphore* pSemaphore)
{
        R_CVULKAN_ASSERT (pSemaphore != NULL);

#if defined(R_CVULKAN_DEBUG)
        if (!pSemaphore)
        {
                return;
        }
#endif

        if (pSemaphore->handle != VK_NULL_HANDLE)
        {
                vkDestroySemaphore (pSemaphore->device, pSemaphore->handle, NULL);
                pSemaphore->handle = VK_NULL_HANDLE;
        }

#if defined(R_CVULKAN_DEBUG)
        pSemaphore->isInitialized = false;
#endif
        pSemaphore->device = VK_NULL_HANDLE;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_SemaphoreSignal (struct R_CVulkan_Semaphore* pSemaphore, uint64_t value)
{
        R_CVULKAN_ASSERT (pSemaphore != NULL);
        R_CVULKAN_ASSERT (pSemaphore->isInitialized);

        if (!pSemaphore)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pSemaphore->isInitialized);
        if (!pSemaphore->isInitialized)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        VkSemaphoreSignalInfo signalInfo = {0};
        signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
        signalInfo.semaphore = pSemaphore->handle;
        signalInfo.value = value;

        VkResult result = vkSignalSemaphore (pSemaphore->device, &signalInfo);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_FAILED;
        }
        return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_SemaphoreWait (struct R_CVulkan_Semaphore* pSemaphore, uint64_t value, uint64_t timeout)
{
        R_CVULKAN_ASSERT (pSemaphore != NULL);
        R_CVULKAN_ASSERT (pSemaphore->isInitialized);

        if (!pSemaphore)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pSemaphore->isInitialized);
        if (!pSemaphore->isInitialized)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        VkSemaphoreWaitInfo waitInfo = {0};
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &pSemaphore->handle;
        waitInfo.pValues = &value;
        waitInfo.flags = 0;

        VkResult result = vkWaitSemaphores (pSemaphore->device, &waitInfo, timeout);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_FAILED;
        }
        return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_SemaphoreGetValue (struct R_CVulkan_Semaphore* pSemaphore, uint64_t* pOutValue)
{
        R_CVULKAN_ASSERT (pSemaphore != NULL);
        R_CVULKAN_ASSERT (pOutValue != NULL);
        R_CVULKAN_ASSERT (pSemaphore->isInitialized);

        if (!pSemaphore || !pOutValue)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pSemaphore->isInitialized);
        if (!pSemaphore->isInitialized)
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        VkResult result = vkGetSemaphoreCounterValue (pSemaphore->device, pSemaphore->handle, pOutValue);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_FAILED;
        }
        return R_CVULKAN_OK;
}

R_CVULKAN_API VkSemaphore
R_CVulkan_SemaphoreGetHandle (const struct R_CVulkan_Semaphore* pSemaphore)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pSemaphore != NULL);
#endif
        return pSemaphore->handle;
}

R_CVULKAN_API VkDevice
R_CVulkan_SemaphoreGetDevice (const struct R_CVulkan_Semaphore* pSemaphore)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pSemaphore != NULL);
#endif
        return pSemaphore->device;
}

R_CVULKAN_API int
R_CVulkan_SemaphoreIsInitialized (const struct R_CVulkan_Semaphore* pSemaphore)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pSemaphore != NULL);
        return pSemaphore->isInitialized;
#else
        (void)pSemaphore;
        return 1;
#endif
}
