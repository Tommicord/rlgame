#include "rlgame.base/cvulkan/cvulkan_render_pass.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_NewRenderPass (
    struct R_CVulkan_RenderPass*                 pRenderPass,
    const struct R_CVulkan_RenderPassCreateInfo* pCreateInfo)
{
    R_CVULKAN_ASSERT (pRenderPass);
    R_CVULKAN_ASSERT (pCreateInfo);
    R_CVULKAN_ASSERT (pCreateInfo->pDevice);
    R_CVULKAN_ASSERT (pCreateInfo->pAttachments);
    R_CVULKAN_ASSERT (pCreateInfo->attachmentCount > 0);
    R_CVULKAN_ASSERT (pCreateInfo->pSubpasses);
    R_CVULKAN_ASSERT (pCreateInfo->subpassCount > 0);

    pRenderPass->device = R_CVulkan_DeviceGetLogicalDevice (pCreateInfo->pDevice);
    pRenderPass->handle = VK_NULL_HANDLE;
    VkRenderPassCreateInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = pCreateInfo->attachmentCount;
    renderPassInfo.pAttachments = pCreateInfo->pAttachments;
    renderPassInfo.subpassCount = pCreateInfo->subpassCount;
    renderPassInfo.pSubpasses = pCreateInfo->pSubpasses;
    renderPassInfo.dependencyCount = pCreateInfo->dependencyCount;
    renderPassInfo.pDependencies = pCreateInfo->pDependencies;

    VkResult result = vkCreateRenderPass (pRenderPass->device, &renderPassInfo, NULL, &pRenderPass->handle);
    if (result != VK_SUCCESS)
    {
        return R_CVULKAN_ERROR_RENDER_PASS_CREATE_FAILED;
    }

#if defined(R_CVULKAN_DEBUG)

#endif
    return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_DeleteRenderPass (struct R_CVulkan_RenderPass* pRenderPass)
{
    R_CVULKAN_ASSERT (pRenderPass);
    vkDestroyRenderPass (pRenderPass->device, pRenderPass->handle, NULL);
#if defined(R_CVULKAN_DEBUG)
    pRenderPass->handle = VK_NULL_HANDLE;
    pRenderPass->device = VK_NULL_HANDLE;
#endif
}

R_CVULKAN_API VkRenderPass
R_CVulkan_RenderPassGetHandle (const struct R_CVulkan_RenderPass* pRenderPass)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pRenderPass);
#endif
    return pRenderPass->handle;
}

R_CVULKAN_API VkDevice
R_CVulkan_RenderPassGetDevice (const struct R_CVulkan_RenderPass* pRenderPass)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pRenderPass);
#endif
    return pRenderPass->device;
}

R_CVULKAN_API int
R_CVulkan_RenderPassIsInitialized (const struct R_CVulkan_RenderPass* pRenderPass)
{
#if defined(R_CVULKAN_DEBUG)
    return 1;
#else
    (void)pRenderPass;
    return 1;
#endif
}
