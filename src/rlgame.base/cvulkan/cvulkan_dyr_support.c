#include "rlgame.base/cvulkan/cvulkan_dyr_support.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <string.h>
#include <stdlib.h>

enum R_CVulkanError
R_CVulkan_DYRCreateRenderPass (
    struct R_CVulkan_DYRRenderPass*                pDYRRenderPass,
    const struct R_CVulkan_DYRRenderPassCreateInfo* pCreateInfo)
{
        R_CVULKAN_ASSERT (pDYRRenderPass);
        R_CVULKAN_ASSERT (pCreateInfo);
        R_CVULKAN_ASSERT (pCreateInfo->pDevice);
        R_CVULKAN_ASSERT (pCreateInfo->colorAttachmentCount > 0);
        R_CVULKAN_ASSERT (pCreateInfo->pColorAttachmentFormats);
#if defined(R_CVULKAN_DEBUG)
        if (!R_CVulkan_DeviceIsInitialized (pCreateInfo->pDevice))
        {
                R_CSTL_LOG_ERROR ("R_CVulkan_DYRCreateRenderPass: Device not initialized");
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif
        VkDevice logicalDevice = R_CVulkan_DeviceGetLogicalDevice (pCreateInfo->pDevice);
        R_CVULKAN_ASSERT (logicalDevice != VK_NULL_HANDLE);

#if defined(R_CVULKAN_DEBUG)
        if (logicalDevice == VK_NULL_HANDLE)
        {
                R_CSTL_LOG_ERROR ("R_CVulkan_DYRCreateRenderPass: Failed to get logical device");
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif
        enum R_CVulkanError error;
        pDYRRenderPass->device = logicalDevice;
#if defined(R_CVULKAN_DEBUG)
        pDYRRenderPass->handle = VK_NULL_HANDLE;
        pDYRRenderPass->booted = false;
#endif
        uint32_t totalAttachmentCount = pCreateInfo->colorAttachmentCount;
        uint32_t depthAttachmentIndex = VK_ATTACHMENT_UNUSED;
        uint32_t stencilAttachmentIndex = VK_ATTACHMENT_UNUSED;

        if (pCreateInfo->depthAttachmentFormat != VK_FORMAT_UNDEFINED)
        {
                totalAttachmentCount++;
                depthAttachmentIndex = pCreateInfo->colorAttachmentCount;
        }

        if (pCreateInfo->stencilAttachmentFormat != VK_FORMAT_UNDEFINED 
            && pCreateInfo->stencilAttachmentFormat != pCreateInfo->depthAttachmentFormat)
        {
                totalAttachmentCount++;
                stencilAttachmentIndex = totalAttachmentCount - 1;
        }

        size_t attachmentsSize = sizeof (VkAttachmentDescription) * totalAttachmentCount;
        VkAttachmentDescription* pAttachments = (VkAttachmentDescription*)R_CSTL_HeapAlloc (attachmentsSize);
        if (!pAttachments)
        {
                R_CSTL_LOG_ERROR ("R_CVulkan_DYRCreateRenderPass: Failed to allocate attachment descriptions");
                return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }
#if defined(R_CVULKAN_DEBUG)
        memset (pAttachments, 0, sizeof (VkAttachmentDescription) * totalAttachmentCount);
#endif
        size_t colorAttachmentRefsSize = 
            sizeof (VkAttachmentReference) * pCreateInfo->colorAttachmentCount;
        VkAttachmentReference* pColorAttachmentRefs = (VkAttachmentReference*)R_CSTL_HeapAlloc (colorAttachmentRefsSize);
        if (!pColorAttachmentRefs)
        {
                R_CSTL_LOG_ERROR ("R_CVulkan_DYRCreateRenderPass: Failed to allocate color attachment references");
                error = R_CVULKAN_ERROR_OUT_OF_MEMORY;
                goto r_cleanup_attachments;
        }
#if defined(R_CVULKAN_DEBUG)
        memset (pColorAttachmentRefs, 0, sizeof (VkAttachmentReference) * pCreateInfo->colorAttachmentCount);
#endif
        for (uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; i++)
        {
                pAttachments[i].format = pCreateInfo->pColorAttachmentFormats[i];
                pAttachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
                pAttachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                pAttachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                pAttachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                pAttachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                pAttachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                pAttachments[i].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                pColorAttachmentRefs[i].attachment = i;
                pColorAttachmentRefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        VkAttachmentReference depthAttachmentRef = {0};
        if (pCreateInfo->depthAttachmentFormat != VK_FORMAT_UNDEFINED)
        {
                pAttachments[depthAttachmentIndex].format = pCreateInfo->depthAttachmentFormat;
                pAttachments[depthAttachmentIndex].samples = VK_SAMPLE_COUNT_1_BIT;
                pAttachments[depthAttachmentIndex].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                pAttachments[depthAttachmentIndex].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                pAttachments[depthAttachmentIndex].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                pAttachments[depthAttachmentIndex].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                pAttachments[depthAttachmentIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                pAttachments[depthAttachmentIndex].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                depthAttachmentRef.attachment = depthAttachmentIndex;
                depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        VkAttachmentReference stencilAttachmentRef = {0};
        if (pCreateInfo->stencilAttachmentFormat != VK_FORMAT_UNDEFINED 
            && pCreateInfo->stencilAttachmentFormat != pCreateInfo->depthAttachmentFormat)
        {
                pAttachments[stencilAttachmentIndex].format = pCreateInfo->stencilAttachmentFormat;
                pAttachments[stencilAttachmentIndex].samples = VK_SAMPLE_COUNT_1_BIT;
                pAttachments[stencilAttachmentIndex].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                pAttachments[stencilAttachmentIndex].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                pAttachments[stencilAttachmentIndex].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                pAttachments[stencilAttachmentIndex].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                pAttachments[stencilAttachmentIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                pAttachments[stencilAttachmentIndex].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                stencilAttachmentRef.attachment = stencilAttachmentIndex;
                stencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        VkSubpassDescription subpass = {0};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = pCreateInfo->colorAttachmentCount;
        subpass.pColorAttachments = pColorAttachmentRefs;
        subpass.pDepthStencilAttachment = (pCreateInfo->depthAttachmentFormat != VK_FORMAT_UNDEFINED) 
                ? &depthAttachmentRef 
                : NULL;

        VkRenderPassCreateInfo renderPassInfo = {0};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = totalAttachmentCount;
        renderPassInfo.pAttachments = pAttachments;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        VkResult result = vkCreateRenderPass (logicalDevice, &renderPassInfo, NULL, &pDYRRenderPass->handle);
        if (result != VK_SUCCESS)
        {
                R_CSTL_LOG_ERROR ("R_CVulkan_DYRCreateRenderPass: Failed to create render pass: %d", result);
                error = R_CVULKAN_ERROR_FAILED;
                goto r_cleanup;
        }

#if defined(R_CVULKAN_DEBUG)
        pDYRRenderPass->booted = true;
        R_CSTL_LOG_INFO ("R_CVulkan_DYRCreateRenderPass: Render pass created");
        R_CSTL_LOG_INFO ("  - Color attachments: %u", pCreateInfo->colorAttachmentCount);
        R_CSTL_LOG_INFO ("  - Depth attachment: %s", 
                R_CVulkan_FormatToString(pCreateInfo->depthAttachmentFormat));
        R_CSTL_LOG_INFO ("  - Stencil attachment: %s", 
                R_CVulkan_FormatToString(pCreateInfo->stencilAttachmentFormat));
#endif
        error = R_CVULKAN_OK;
r_cleanup:
        R_CSTL_HeapFree (pColorAttachmentRefs);
r_cleanup_attachments:
        R_CSTL_HeapFree (pAttachments);
        return error;
}

void
R_CVulkan_DYRDeleteRenderPass (struct R_CVulkan_DYRRenderPass* pDYRRenderPass)
{
        R_CVULKAN_ASSERT (pDYRRenderPass);

#if defined(R_CVULKAN_DEBUG)
        if (!pDYRRenderPass)
        {
                return;
        }
#endif

        if (pDYRRenderPass->handle != VK_NULL_HANDLE)
        {
                vkDestroyRenderPass (pDYRRenderPass->device, pDYRRenderPass->handle, NULL);
                pDYRRenderPass->handle = VK_NULL_HANDLE;
        }
#if defined(R_CVULKAN_DEBUG)
        pDYRRenderPass->device = VK_NULL_HANDLE;
        pDYRRenderPass->booted = false;
#endif
}

VkRenderPass
R_CVulkan_DYRRenderPassGetHandle (const struct R_CVulkan_DYRRenderPass* pDYRRenderPass)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pDYRRenderPass);
#endif
        return pDYRRenderPass->handle;
}

int
R_CVulkan_DYRRenderPassIsInitialized (const struct R_CVulkan_DYRRenderPass* pDYRRenderPass)
{
#if defined(R_CVULKAN_DEBUG)
        return pDYRRenderPass->booted;
#else
        (void)pDYRRenderPass;
        return 1;
#endif
}

