#include "rlgame.base/cvulkan/cvulkan_framebuffer.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

#include <string.h>
#include <stdlib.h>

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_NewFramebuffer (
    struct R_CVulkan_Framebuffer*                 pFramebuffer,
    const struct R_CVulkan_FramebufferCreateInfo* pCreateInfo)
{
    R_CVULKAN_ASSERT (pFramebuffer);
    R_CVULKAN_ASSERT (pCreateInfo);
    R_CVULKAN_ASSERT (pCreateInfo->pDevice);
    R_CVULKAN_ASSERT (pCreateInfo->pRenderPass);
    R_CVULKAN_ASSERT (pCreateInfo->pAttachments);
    R_CVULKAN_ASSERT (pCreateInfo->attachmentCount > 0);

#if defined(R_CVULKAN_DEBUG)
    if (pCreateInfo->width == 0 || pCreateInfo->height == 0 || pCreateInfo->layers == 0)
    {
        return R_CVULKAN_ERROR_INVALID_ARGUMENT;
    }
#endif
    pFramebuffer->device = R_CVulkan_DeviceGetLogicalDevice (pCreateInfo->pDevice);
    pFramebuffer->handle = VK_NULL_HANDLE;
    pFramebuffer->renderPass = pCreateInfo->pRenderPass;
    pFramebuffer->width = pCreateInfo->width;
    pFramebuffer->height = pCreateInfo->height;
    pFramebuffer->attachmentCount = pCreateInfo->attachmentCount;
#if defined(R_CVULKAN_DEBUG)
    
#endif
    VkFramebufferCreateInfo framebufferInfo = {0};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = pCreateInfo->pRenderPass;
    framebufferInfo.attachmentCount = pCreateInfo->attachmentCount;
    framebufferInfo.pAttachments = pCreateInfo->pAttachments;
    framebufferInfo.width = pCreateInfo->width;
    framebufferInfo.height = pCreateInfo->height;
    framebufferInfo.layers = pCreateInfo->layers;

    VkResult result
        = vkCreateFramebuffer (pFramebuffer->device, &framebufferInfo, NULL, &pFramebuffer->handle);
    if (result != VK_SUCCESS)
    {
        return R_CVULKAN_ERROR_FRAMEBUFFER_CREATE_FAILED;
    }

#if defined(R_CVULKAN_DEBUG)
    
#endif
    return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_DeleteFramebuffer (struct R_CVulkan_Framebuffer* pFramebuffer)
{
    R_CVULKAN_ASSERT (pFramebuffer);

    vkDestroyFramebuffer (pFramebuffer->device, pFramebuffer->handle, NULL);
#if defined(R_CVULKAN_DEBUG)
    pFramebuffer->device = VK_NULL_HANDLE;
    pFramebuffer->renderPass = VK_NULL_HANDLE;
    pFramebuffer->width = 0;
    pFramebuffer->height = 0;
    pFramebuffer->attachmentCount = 0;
    
#endif
}

R_CVULKAN_API VkFramebuffer
R_CVulkan_FramebufferGetHandle (const struct R_CVulkan_Framebuffer* pFramebuffer)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pFramebuffer);
#endif
    return pFramebuffer->handle;
}

R_CVULKAN_API VkDevice
R_CVulkan_FramebufferGetDevice (const struct R_CVulkan_Framebuffer* pFramebuffer)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pFramebuffer);
#endif
    return pFramebuffer->device;
}

R_CVULKAN_API VkRenderPass
R_CVulkan_FramebufferGetRenderPass (const struct R_CVulkan_Framebuffer* pFramebuffer)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pFramebuffer);
#endif
    return pFramebuffer->renderPass;
}

R_CVULKAN_API uint32_t
R_CVulkan_FramebufferGetWidth (const struct R_CVulkan_Framebuffer* pFramebuffer)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pFramebuffer);
#endif
    return pFramebuffer->width;
}

R_CVULKAN_API uint32_t
R_CVulkan_FramebufferGetHeight (const struct R_CVulkan_Framebuffer* pFramebuffer)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pFramebuffer);
#endif
    return pFramebuffer->height;
}

R_CVULKAN_API uint32_t
R_CVulkan_FramebufferGetAttachmentCount (const struct R_CVulkan_Framebuffer* pFramebuffer)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pFramebuffer);
#endif
    return pFramebuffer->attachmentCount;
}

R_CVULKAN_API int
R_CVulkan_FramebufferIsInitialized (const struct R_CVulkan_Framebuffer* pFramebuffer)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pFramebuffer);
    return 1;
#else
    (void)pFramebuffer;
    return 1;
#endif
}
