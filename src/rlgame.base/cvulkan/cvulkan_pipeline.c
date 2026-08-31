#include "rlgame.base/cvulkan/cvulkan_pipeline.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_dyr_support.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <string.h>
#include <stdlib.h>

enum R_CVulkan_Error
r_cvulkan_new_pipeline_layout (
    struct R_CVulkan_PipelineLayout**                ppLayout,
    const struct r_cvulkan_pipeline_layout_create_info* pCreateInfo)
{
    R_CVULKAN_ASSERT (ppLayout);
    R_CVULKAN_ASSERT (pCreateInfo);
    R_CVULKAN_ASSERT (pCreateInfo->pDevice);

    VkDevice logicalDevice = r_cvulkan_device_get_logical_device (pCreateInfo->pDevice);
    R_CVULKAN_ASSERT (logicalDevice != VK_NULL_HANDLE);
#if defined(R_CVULKAN_DEBUG)
    if (logicalDevice == VK_NULL_HANDLE)
    {
        R_CSTL_LOG_ERROR ("r_cvulkan_new_pipeline_layout: Failed to get logical device from device wrapper");
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }
#endif
    struct R_CVulkan_PipelineLayout* pLayout
        = (struct R_CVulkan_PipelineLayout*)r_cstl_heap_alloc (sizeof (struct R_CVulkan_PipelineLayout));
    if (!pLayout)
    {
        return R_CVULKAN_ERROR_OUT_OF_MEMORY;
    }

    pLayout->device = logicalDevice;
#if defined(R_CVULKAN_DEBUG)
    pLayout->handle = VK_NULL_HANDLE;

#endif

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = pCreateInfo->setLayoutCount;
    pipelineLayoutInfo.pSetLayouts = pCreateInfo->pSetLayouts;
    pipelineLayoutInfo.pushConstantRangeCount = pCreateInfo->pushConstantRangeCount;
    pipelineLayoutInfo.pPushConstantRanges = pCreateInfo->pPushConstantRanges;

    VkResult result = vkCreatePipelineLayout (pLayout->device, &pipelineLayoutInfo, NULL, &pLayout->handle);
    if (result != VK_SUCCESS)
    {
        r_cstl_heap_free (pLayout);
        return R_CVULKAN_ERROR_FAILED;
    }

#if defined(R_CVULKAN_DEBUG)

#endif
    *ppLayout = pLayout;
    return R_CVULKAN_OK;
}

void
r_cvulkan_delete_pipeline_layout (struct R_CVulkan_PipelineLayout* pLayout)
{
    R_CVULKAN_ASSERT (pLayout);

    vkDestroyPipelineLayout (pLayout->device, pLayout->handle, NULL);
#if defined(R_CVULKAN_DEBUG)
    pLayout->device = VK_NULL_HANDLE;

#endif
    r_cstl_heap_free (pLayout);
}

enum R_CVulkan_Error
r_cvulkan_new_graphics_pipeline (
    struct R_CVulkan_Pipeline*                         pPipeline,
    const struct r_cvulkan_graphics_pipeline_create_info* pCreateInfo)
{
    R_CVULKAN_ASSERT (pPipeline);
    R_CVULKAN_ASSERT (pCreateInfo);
    R_CVULKAN_ASSERT (pCreateInfo->pDevice);
    R_CVULKAN_ASSERT (pCreateInfo->pipelineLayout != VK_NULL_HANDLE);
    R_CVULKAN_ASSERT (pCreateInfo->pRenderPass != VK_NULL_HANDLE);
    R_CVULKAN_ASSERT (pCreateInfo->pStages);
    R_CVULKAN_ASSERT (pCreateInfo->stageCount > 0);
    R_CVULKAN_ASSERT (pCreateInfo->pVertexInputInfo);
    R_CVULKAN_ASSERT (pCreateInfo->pInputAssemblyInfo);
    R_CVULKAN_ASSERT (pCreateInfo->pViewportInfo);
    R_CVULKAN_ASSERT (pCreateInfo->pRasterizationInfo);
    R_CVULKAN_ASSERT (pCreateInfo->pMultisampleInfo);
    R_CVULKAN_ASSERT (pCreateInfo->pColorBlendInfo);

    VkDevice logicalDevice = r_cvulkan_device_get_logical_device (pCreateInfo->pDevice);
    R_CVULKAN_ASSERT (logicalDevice != VK_NULL_HANDLE);
#if defined(R_CVULKAN_DEBUG)
    if (logicalDevice == VK_NULL_HANDLE)
    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }
#endif

    pPipeline->device = logicalDevice;
#if defined(R_CVULKAN_DEBUG)
    pPipeline->handle = VK_NULL_HANDLE;

#endif

    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = pCreateInfo->stageCount;
    pipelineInfo.pStages = pCreateInfo->pStages;
    pipelineInfo.pVertexInputState = pCreateInfo->pVertexInputInfo;
    pipelineInfo.pInputAssemblyState = pCreateInfo->pInputAssemblyInfo;
    pipelineInfo.pViewportState = pCreateInfo->pViewportInfo;
    pipelineInfo.pRasterizationState = pCreateInfo->pRasterizationInfo;
    pipelineInfo.pMultisampleState = pCreateInfo->pMultisampleInfo;
    pipelineInfo.pDepthStencilState = pCreateInfo->pDepthStencilInfo;
    pipelineInfo.pColorBlendState = pCreateInfo->pColorBlendInfo;
    pipelineInfo.pDynamicState = pCreateInfo->pDynamicStateInfo;
    pipelineInfo.layout = pCreateInfo->pipelineLayout;
    pipelineInfo.renderPass = pCreateInfo->pRenderPass;
    pipelineInfo.subpass = pCreateInfo->subpass;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkResult result = vkCreateGraphicsPipelines (
        pPipeline->device,
        VK_NULL_HANDLE,
        1,
        &pipelineInfo,
        NULL,
        &pPipeline->handle);
    if (result != VK_SUCCESS)
    {
        return R_CVULKAN_ERROR_PIPELINE_CREATE_FAILED;
    }

#if defined(R_CVULKAN_DEBUG)

#endif
    return R_CVULKAN_OK;
}

enum R_CVulkan_Error
r_cvulkan_new_dynamic_graphics_pipeline (
    struct R_CVulkan_Pipeline*                         pPipeline,
    const struct r_cvulkan_graphics_pipeline_create_info* pCreateInfo)
{
    R_CVULKAN_ASSERT (pPipeline);
    R_CVULKAN_ASSERT (pCreateInfo);
    R_CVULKAN_ASSERT (pCreateInfo->pDevice);
    R_CVULKAN_ASSERT (pCreateInfo->pipelineLayout != VK_NULL_HANDLE);
    R_CVULKAN_ASSERT (pCreateInfo->pStages);
    R_CVULKAN_ASSERT (pCreateInfo->stageCount > 0);
    R_CVULKAN_ASSERT (pCreateInfo->pVertexInputInfo);
    R_CVULKAN_ASSERT (pCreateInfo->pInputAssemblyInfo);
    R_CVULKAN_ASSERT (pCreateInfo->pViewportInfo);
    R_CVULKAN_ASSERT (pCreateInfo->pRasterizationInfo);
    R_CVULKAN_ASSERT (pCreateInfo->pMultisampleInfo);
    R_CVULKAN_ASSERT (pCreateInfo->pColorBlendInfo);
    R_CVULKAN_ASSERT (pCreateInfo->pColorAttachmentFormats);

#if defined(R_CVULKAN_DEBUG)
    if (!pPipeline || !pCreateInfo || !pCreateInfo->pDevice)
    {
        R_CSTL_LOG_ERROR ("r_cvulkan_new_dynamic_graphics_pipeline: NULL pointer detected");
        R_CSTL_LOG_ERROR ("  - pPipeline: %p", (void*)pPipeline);
        R_CSTL_LOG_ERROR ("  - pCreateInfo: %p", (void*)pCreateInfo);
        R_CSTL_LOG_ERROR ("  - pCreateInfo->pDevice: %p", (void*)(pCreateInfo ? pCreateInfo->pDevice : NULL));
        return R_CVULKAN_ERROR_NULL_POINTER;
    }
    if (pCreateInfo->pipelineLayout == VK_NULL_HANDLE)
    {
        R_CSTL_LOG_ERROR ("r_cvulkan_new_dynamic_graphics_pipeline: Pipeline layout is VK_NULL_HANDLE");
        return R_CVULKAN_ERROR_NULL_POINTER;
    }
    if (!pCreateInfo->pStages || pCreateInfo->stageCount == 0)
    {
        R_CSTL_LOG_ERROR ("r_cvulkan_new_dynamic_graphics_pipeline: Invalid shader stages");
        R_CSTL_LOG_ERROR ("  pStages: %p", (void*)pCreateInfo->pStages);
        R_CSTL_LOG_ERROR ("  stageCount: %u", pCreateInfo->stageCount);
        return R_CVULKAN_ERROR_NULL_POINTER;
    }
    if (!r_cvulkan_device_is_dynamic_rendering_supported (pCreateInfo->pDevice))
    {
        R_CSTL_LOG_WARN ("r_cvulkan_new_dynamic_graphics_pipeline: Dynamic rendering not supported by device");
        R_CSTL_LOG_WARN ("  Falling back to traditional render pass approach using DYR support");

        struct R_CVulkan_DYRRenderPass           dyrRenderPass = {0};
        struct R_CVulkan_DYRRenderPassCreateInfo dyrCreateInfo = {0};
        dyrCreateInfo.pDevice = pCreateInfo->pDevice;
        dyrCreateInfo.colorAttachmentCount = pCreateInfo->colorAttachmentCount;
        dyrCreateInfo.pColorAttachmentFormats = pCreateInfo->pColorAttachmentFormats;
        dyrCreateInfo.depthAttachmentFormat = pCreateInfo->depthAttachmentFormat;
        dyrCreateInfo.stencilAttachmentFormat = pCreateInfo->stencilAttachmentFormat;

        enum R_CVulkan_Error dyrResult = R_CVulkan_DYRCreateRenderPass (&dyrRenderPass, &dyrCreateInfo);
        if (dyrResult != R_CVULKAN_OK)
        {
            R_CSTL_LOG_ERROR (
                "r_cvulkan_new_dynamic_graphics_pipeline: Failed to create fallback render pass: %d",
                dyrResult);
            return dyrResult;
        }

        struct r_cvulkan_graphics_pipeline_create_info fallbackCreateInfo = *pCreateInfo;
        fallbackCreateInfo.pRenderPass = R_CVulkan_DYRRenderPassGetHandle (&dyrRenderPass);

        enum R_CVulkan_Error pipelineResult = r_cvulkan_new_graphics_pipeline (pPipeline, &fallbackCreateInfo);

        R_CVulkan_DYRDeleteRenderPass (&dyrRenderPass);

        return pipelineResult;
    }
#endif
    VkDevice logicalDevice = r_cvulkan_device_get_logical_device (pCreateInfo->pDevice);
    R_CVULKAN_ASSERT (logicalDevice != VK_NULL_HANDLE);

#if defined(R_CVULKAN_DEBUG)
    if (logicalDevice == VK_NULL_HANDLE)
    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }
#endif

    pPipeline->device = logicalDevice;
#if defined(R_CVULKAN_DEBUG)
    pPipeline->handle = VK_NULL_HANDLE;

#endif

    VkPipelineRenderingCreateInfoKHR renderingCreateInfo = {0};
    renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    renderingCreateInfo.viewMask = 0;
    renderingCreateInfo.colorAttachmentCount = pCreateInfo->colorAttachmentCount;
    renderingCreateInfo.pColorAttachmentFormats = pCreateInfo->pColorAttachmentFormats;
    renderingCreateInfo.depthAttachmentFormat = pCreateInfo->depthAttachmentFormat;
    renderingCreateInfo.stencilAttachmentFormat = pCreateInfo->stencilAttachmentFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingCreateInfo;
    pipelineInfo.stageCount = pCreateInfo->stageCount;
    pipelineInfo.pStages = pCreateInfo->pStages;
    pipelineInfo.pVertexInputState = pCreateInfo->pVertexInputInfo;
    pipelineInfo.pInputAssemblyState = pCreateInfo->pInputAssemblyInfo;
    pipelineInfo.pViewportState = pCreateInfo->pViewportInfo;
    pipelineInfo.pRasterizationState = pCreateInfo->pRasterizationInfo;
    pipelineInfo.pMultisampleState = pCreateInfo->pMultisampleInfo;
    pipelineInfo.pDepthStencilState = pCreateInfo->pDepthStencilInfo;
    pipelineInfo.pColorBlendState = pCreateInfo->pColorBlendInfo;
    pipelineInfo.pDynamicState = pCreateInfo->pDynamicStateInfo;
    pipelineInfo.layout = pCreateInfo->pipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkResult result = vkCreateGraphicsPipelines (
        pPipeline->device,
        VK_NULL_HANDLE,
        1,
        &pipelineInfo,
        NULL,
        &pPipeline->handle);
    if (result != VK_SUCCESS)
    {
        R_CSTL_LOG_ERROR ("r_cvulkan_new_dynamic_graphics_pipeline: vkCreateGraphicsPipelines failed");
        R_CSTL_LOG_ERROR ("  Vulkan result code: %d", result);
        R_CSTL_LOG_ERROR ("  Shader stage count: %u", pCreateInfo->stageCount);
        R_CSTL_LOG_ERROR ("  Pipeline layout handle: %p", (void*)pCreateInfo->pipelineLayout);
        R_CSTL_LOG_ERROR ("  Color attachment count: %u", pCreateInfo->colorAttachmentCount);
        return R_CVULKAN_ERROR_PIPELINE_CREATE_FAILED;
    }

#if defined(R_CVULKAN_DEBUG)

    R_CSTL_LOG_INFO ("r_cvulkan_new_dynamic_graphics_pipeline: Graphics pipeline created");
    R_CSTL_LOG_INFO ("  Handle: %p", (void*)pPipeline->handle);
    R_CSTL_LOG_INFO ("  Shader stage count: %u", pCreateInfo->stageCount);
#endif
    return R_CVULKAN_OK;
}

enum R_CVulkan_Error
r_cvulkan_new_compute_pipeline (
    struct R_CVulkan_Pipeline*             pPipeline,
    const struct R_CVulkan_Device*         pDevice,
    VkPipelineLayout                       pipelineLayout,
    const VkPipelineShaderStageCreateInfo* pStage)
{
    R_CVULKAN_ASSERT (pPipeline);
    R_CVULKAN_ASSERT (pDevice);
    R_CVULKAN_ASSERT (pipelineLayout != VK_NULL_HANDLE);
    R_CVULKAN_ASSERT (pStage);

    VkDevice logicalDevice = r_cvulkan_device_get_logical_device (pDevice);
    R_CVULKAN_ASSERT (logicalDevice != VK_NULL_HANDLE);
#if defined(R_CVULKAN_DEBUG)
    if (logicalDevice == VK_NULL_HANDLE)
    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }
#endif

    pPipeline->device = logicalDevice;
#if defined(R_CVULKAN_DEBUG)
    pPipeline->handle = VK_NULL_HANDLE;

#endif

    VkComputePipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.stage = *pStage;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkResult result = vkCreateComputePipelines (
        pPipeline->device,
        VK_NULL_HANDLE,
        1,
        &pipelineInfo,
        NULL,
        &pPipeline->handle);
    if (result != VK_SUCCESS)
    {
        return R_CVULKAN_ERROR_PIPELINE_CREATE_FAILED;
    }

#if defined(R_CVULKAN_DEBUG)

#endif
    return R_CVULKAN_OK;
}

void
R_CVulkan_DeletePipeline (struct R_CVulkan_Pipeline* pPipeline)
{
    R_CVULKAN_ASSERT (pPipeline);

#if defined(R_CVULKAN_DEBUG)
    if (!pPipeline)
    {
        return;
    }
#endif

    if (pPipeline->handle != VK_NULL_HANDLE)
    {
        vkDestroyPipeline (pPipeline->device, pPipeline->handle, NULL);
        pPipeline->handle = VK_NULL_HANDLE;
    }
#if defined(R_CVULKAN_DEBUG)
    pPipeline->device = VK_NULL_HANDLE;

#endif
}

VkPipelineLayout
r_cvulkan_pipeline_layout_get_handle (const struct R_CVulkan_PipelineLayout* pLayout)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pLayout);
#endif
    return pLayout->handle;
}

VkDevice
r_cvulkan_pipeline_layout_get_device (const struct R_CVulkan_PipelineLayout* pLayout)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pLayout);
#endif
    return pLayout->device;
}

int
r_cvulkan_pipeline_layout_is_initialized (const struct R_CVulkan_PipelineLayout* pLayout)
{
#if defined(R_CVULKAN_DEBUG)
    return 1;
#else
    (void)pLayout;
    return 1;
#endif
}

VkPipeline
r_cvulkan_pipeline_get_handle (const struct R_CVulkan_Pipeline* pPipeline)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pPipeline);
#endif
    return pPipeline->handle;
}

VkDevice
r_cvulkan_pipeline_get_device (const struct R_CVulkan_Pipeline* pPipeline)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pPipeline);
#endif
    return pPipeline->device;
}

int
r_cvulkan_pipeline_is_initialized (const struct R_CVulkan_Pipeline* pPipeline)
{
#if defined(R_CVULKAN_DEBUG)
    return 1;
#else
    (void)pPipeline;
    return 1;
#endif
}
