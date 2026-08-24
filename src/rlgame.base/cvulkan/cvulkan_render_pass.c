#include "rlgame.base/cvulkan/cvulkan_render_pass.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

R_CVULKAN_API enum R_CVulkanError
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

#if defined(R_CVULKAN_DEBUG)
        if (!pRenderPass || !pCreateInfo || !pCreateInfo->pDevice || !pCreateInfo->pAttachments)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }
        if (pCreateInfo->attachmentCount == 0 || !pCreateInfo->pSubpasses || pCreateInfo->subpassCount == 0)
        {
                return R_CVULKAN_ERROR_INVALID_ARGUMENT;
        }
        if (!R_CVulkan_DeviceIsInitialized (pCreateInfo->pDevice))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        pRenderPass->device = R_CVulkan_DeviceGetLogicalDevice (pCreateInfo->pDevice);
        pRenderPass->handle = VK_NULL_HANDLE;
#if defined(R_CVULKAN_DEBUG)
        pRenderPass->booted = false;
#endif

        VkRenderPassCreateInfo renderPassInfo = {0};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = pCreateInfo->attachmentCount;
        renderPassInfo.pAttachments = pCreateInfo->pAttachments;
        renderPassInfo.subpassCount = pCreateInfo->subpassCount;
        renderPassInfo.pSubpasses = pCreateInfo->pSubpasses;
        renderPassInfo.dependencyCount = pCreateInfo->dependencyCount;
        renderPassInfo.pDependencies = pCreateInfo->pDependencies;

        VkResult result
            = vkCreateRenderPass (pRenderPass->device, &renderPassInfo, NULL, &pRenderPass->handle);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_RENDER_PASS_CREATE_FAILED;
        }

#if defined(R_CVULKAN_DEBUG)
        pRenderPass->booted = true;
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
        pRenderPass->booted = false;
#endif
}

R_CVULKAN_API VkRenderPass
R_CVulkan_RenderPassGetHandle (const struct R_CVulkan_RenderPass* pRenderPass)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pRenderPass );
#endif
        return pRenderPass->handle;
}

R_CVULKAN_API VkDevice
R_CVulkan_RenderPassGetDevice (const struct R_CVulkan_RenderPass* pRenderPass)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pRenderPass );
#endif
        return pRenderPass->device;
}

R_CVULKAN_API int
R_CVulkan_RenderPassIsInitialized (const struct R_CVulkan_RenderPass* pRenderPass)
{
#if defined(R_CVULKAN_DEBUG)
        return pRenderPass->booted;
#else
        (void)pRenderPass;
        return 1;
#endif
}
