#include "rlgame.base/cvulkan/cvulkan_render_pass.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

R_CVULKAN_API enum R_CVulkan_Error
r_cvulkan_new_render_pass (
    struct R_CVulkan_RenderPass*                 pRenderPass,
    const struct r_cvulkan_render_pass_create_info* pCreateInfo)
{
    R_CVULKAN_ASSERT (pRenderPass);
    R_CVULKAN_ASSERT (pCreateInfo);
    R_CVULKAN_ASSERT (pCreateInfo->pDevice);
    R_CVULKAN_ASSERT (pCreateInfo->pAttachments);
    R_CVULKAN_ASSERT (pCreateInfo->attachmentCount > 0);
    R_CVULKAN_ASSERT (pCreateInfo->pSubpasses);
    R_CVULKAN_ASSERT (pCreateInfo->subpassCount > 0);

    pRenderPass->device = r_cvulkan_device_get_logical_device (pCreateInfo->pDevice);
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
r_cvulkan_delete_render_pass (struct R_CVulkan_RenderPass* pRenderPass)
{
    R_CVULKAN_ASSERT (pRenderPass);
    vkDestroyRenderPass (pRenderPass->device, pRenderPass->handle, NULL);
#if defined(R_CVULKAN_DEBUG)
    pRenderPass->handle = VK_NULL_HANDLE;
    pRenderPass->device = VK_NULL_HANDLE;
#endif
}

R_CVULKAN_API VkRenderPass
r_cvulkan_render_pass_get_handle (const struct R_CVulkan_RenderPass* pRenderPass)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pRenderPass);
#endif
    return pRenderPass->handle;
}

R_CVULKAN_API VkDevice
r_cvulkan_render_pass_get_device (const struct R_CVulkan_RenderPass* pRenderPass)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pRenderPass);
#endif
    return pRenderPass->device;
}

R_CVULKAN_API int
r_cvulkan_render_pass_is_initialized (const struct R_CVulkan_RenderPass* pRenderPass)
{
#if defined(R_CVULKAN_DEBUG)
    return 1;
#else
    (void)pRenderPass;
    return 1;
#endif
}
