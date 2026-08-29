#include "rlgame.base/cvulkan/cvulkan_semaphore.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_NewSemaphore (
    struct R_CVulkan_Semaphore*    pSemaphore,
    const struct R_CVulkan_Device* pDevice,
    int                            timelineSemaphore,
    uint64_t                       initialValue)
{
    R_CVULKAN_ASSERT (pSemaphore);
    R_CVULKAN_ASSERT (pDevice);

    pSemaphore->device = R_CVulkan_DeviceGetLogicalDevice (pDevice);
    pSemaphore->handle = VK_NULL_HANDLE;
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
    return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_DeleteSemaphore (struct R_CVulkan_Semaphore* pSemaphore)
{
    R_CVULKAN_ASSERT (pSemaphore);
    vkDestroySemaphore (pSemaphore->device, pSemaphore->handle, NULL);
#if defined(R_CVULKAN_DEBUG)
    pSemaphore->handle = VK_NULL_HANDLE;
    pSemaphore->device = VK_NULL_HANDLE;
#endif
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_SemaphoreSignal (struct R_CVulkan_Semaphore* pSemaphore, uint64_t value)
{
    R_CVULKAN_ASSERT (pSemaphore);
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
    R_CVULKAN_ASSERT (pSemaphore);
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
    R_CVULKAN_ASSERT (pSemaphore);
    R_CVULKAN_ASSERT (pOutValue);

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
    R_CVULKAN_ASSERT (pSemaphore);
    return pSemaphore->handle;
}

R_CVULKAN_API VkDevice
R_CVulkan_SemaphoreGetDevice (const struct R_CVulkan_Semaphore* pSemaphore)
{
    R_CVULKAN_ASSERT (pSemaphore);
    return pSemaphore->device;
}