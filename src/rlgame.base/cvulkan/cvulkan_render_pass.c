#include "rlgame.base/cvulkan/cvulkan_render_pass.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_NewRenderPass (
    struct R_CVulkan_RenderPass*   pRenderPass,
    const struct R_CVulkan_Device* pDevice,
    const VkAttachmentDescription* pAttachments,
    uint32_t                       attachmentCount,
    const VkSubpassDescription*    pSubpasses,
    uint32_t                       subpassCount,
    const VkSubpassDependency*     pDependencies,
    uint32_t                       dependencyCount)
{
        R_CVULKAN_ASSERT (pRenderPass);
        R_CVULKAN_ASSERT (pDevice);
        R_CVULKAN_ASSERT (pAttachments);
        R_CVULKAN_ASSERT (attachmentCount > 0);
        R_CVULKAN_ASSERT (pSubpasses);
        R_CVULKAN_ASSERT (subpassCount > 0);

#if defined(R_CVULKAN_DEBUG)
        if (!pRenderPass || !pDevice || !pAttachments)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }
        if (attachmentCount == 0 || !pSubpasses || subpassCount == 0)
        {
                return R_CVULKAN_ERROR_INVALID_ARGUMENT;
        }
        if (!R_CVulkan_DeviceIsInitialized (pDevice))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        pRenderPass->device = R_CVulkan_DeviceGetLogicalDevice (pDevice);
        pRenderPass->handle = VK_NULL_HANDLE;
#if defined(R_CVULKAN_DEBUG)
        pRenderPass->isInitialized = false;
#endif

        VkRenderPassCreateInfo renderPassInfo = {0};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = attachmentCount;
        renderPassInfo.pAttachments = pAttachments;
        renderPassInfo.subpassCount = subpassCount;
        renderPassInfo.pSubpasses = pSubpasses;
        renderPassInfo.dependencyCount = dependencyCount;
        renderPassInfo.pDependencies = pDependencies;

        VkResult result
            = vkCreateRenderPass (pRenderPass->device, &renderPassInfo, NULL, &pRenderPass->handle);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_RENDER_PASS_CREATE_FAILED;
        }

#if defined(R_CVULKAN_DEBUG)
        pRenderPass->isInitialized = true;
#endif
        return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_DeleteRenderPass (struct R_CVulkan_RenderPass* pRenderPass)
{
        R_CVULKAN_ASSERT (pRenderPass);

#if defined(R_CVULKAN_DEBUG)
        if (!pRenderPass)
        {
                return;
        }
#endif

        if (pRenderPass->handle != VK_NULL_HANDLE)
        {
                vkDestroyRenderPass (pRenderPass->device, pRenderPass->handle, NULL);
                pRenderPass->handle = VK_NULL_HANDLE;
        }

        pRenderPass->device = VK_NULL_HANDLE;
#if defined(R_CVULKAN_DEBUG)
        pRenderPass->isInitialized = false;
#endif
}

R_CVULKAN_API VkRenderPass
R_CVulkan_RenderPass_GetHandle (const struct R_CVulkan_RenderPass* pRenderPass)
{
        return pRenderPass->handle;
}

R_CVULKAN_API VkDevice
R_CVulkan_RenderPass_GetDevice (const struct R_CVulkan_RenderPass* pRenderPass)
{
        return pRenderPass->device;
}

R_CVULKAN_API int
R_CVulkan_RenderPass_IsInitialized (const struct R_CVulkan_RenderPass* pRenderPass)
{
#if defined(R_CVULKAN_DEBUG)
        return pRenderPass->isInitialized;
#else
        (void)pRenderPass;
        return 1;
#endif
}
