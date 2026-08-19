#include "rlgame.base/cvulkan/cvulkan_pipeline.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

#include <string.h>
#include <stdlib.h>

enum R_CVulkan_Error
R_CVulkan_NewPipelineLayout (
    struct R_CVulkan_PipelineLayout*             pLayout,
    const struct R_CVulkan_PipelineLayoutCreateInfo* pCreateInfo)
{
        R_CVULKAN_ASSERT (pLayout);
        R_CVULKAN_ASSERT (pCreateInfo);
        R_CVULKAN_ASSERT (pCreateInfo->pDevice);

#if defined(R_CVULKAN_DEBUG)
        if (!pLayout || !pCreateInfo || !pCreateInfo->pDevice)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!R_CVulkan_DeviceIsInitialized (pCreateInfo->pDevice))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        pLayout->device = R_CVulkan_DeviceGetLogicalDevice (pCreateInfo->pDevice);
#if defined(R_CVULKAN_DEBUG)
        pLayout->handle = VK_NULL_HANDLE;
        pLayout->isInitialized = false;
#endif

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = pCreateInfo->setLayoutCount;
        pipelineLayoutInfo.pSetLayouts = pCreateInfo->pSetLayouts;
        pipelineLayoutInfo.pushConstantRangeCount = pCreateInfo->pushConstantRangeCount;
        pipelineLayoutInfo.pPushConstantRanges = pCreateInfo->pPushConstantRanges;

        VkResult result
            = vkCreatePipelineLayout (pLayout->device, &pipelineLayoutInfo, NULL, &pLayout->handle);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_FAILED;
        }

#if defined(R_CVULKAN_DEBUG)
        pLayout->isInitialized = true;
#endif
        return R_CVULKAN_OK;
}

void
R_CVulkan_DeletePipelineLayout (struct R_CVulkan_PipelineLayout* pLayout)
{
        R_CVULKAN_ASSERT (pLayout);

#if defined(R_CVULKAN_DEBUG)
        if (!pLayout)
        {
                return;
        }
#endif
        if (pLayout->handle != VK_NULL_HANDLE)
        {
                vkDestroyPipelineLayout (pLayout->device, pLayout->handle, NULL);
                pLayout->handle = VK_NULL_HANDLE;
        }
#if defined(R_CVULKAN_DEBUG)
        pLayout->device = VK_NULL_HANDLE;
        pLayout->isInitialized = false;
#endif
}

enum R_CVulkan_Error
R_CVulkan_NewGraphicsPipeline (
    struct R_CVulkan_Pipeline*                       pPipeline,
    const struct R_CVulkan_GraphicsPipelineCreateInfo* pCreateInfo)
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

#if defined(R_CVULKAN_DEBUG)
        if (!pPipeline || !pCreateInfo || !pCreateInfo->pDevice)
                return R_CVULKAN_ERROR_NULL_POINTER;
        if (pCreateInfo->pipelineLayout == VK_NULL_HANDLE)
                return R_CVULKAN_ERROR_NULL_POINTER;
        if (pCreateInfo->pRenderPass == VK_NULL_HANDLE)
                return R_CVULKAN_ERROR_NULL_POINTER;
        if (!pCreateInfo->pStages || pCreateInfo->stageCount == 0)
                return R_CVULKAN_ERROR_NULL_POINTER;
        if (!pCreateInfo->pVertexInputInfo || !pCreateInfo->pInputAssemblyInfo || !pCreateInfo->pViewportInfo)
                return R_CVULKAN_ERROR_NULL_POINTER;
        if (!pCreateInfo->pRasterizationInfo || !pCreateInfo->pMultisampleInfo || !pCreateInfo->pColorBlendInfo)
                return R_CVULKAN_ERROR_NULL_POINTER;
        if (!R_CVulkan_DeviceIsInitialized (pCreateInfo->pDevice))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        pPipeline->device = R_CVulkan_DeviceGetLogicalDevice (pCreateInfo->pDevice);
#if defined(R_CVULKAN_DEBUG)
        pPipeline->handle = VK_NULL_HANDLE;
        pPipeline->isInitialized = false;
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
        pPipeline->isInitialized = true;
#endif
        return R_CVULKAN_OK;
}

enum R_CVulkan_Error
R_CVulkan_NewDynamicGraphicsPipeline (
    struct R_CVulkan_Pipeline*                       pPipeline,
    const struct R_CVulkan_GraphicsPipelineCreateInfo* pCreateInfo)
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
                return R_CVULKAN_ERROR_NULL_POINTER;
        if (pCreateInfo->pipelineLayout == VK_NULL_HANDLE)
                return R_CVULKAN_ERROR_NULL_POINTER;
        if (!pCreateInfo->pStages || pCreateInfo->stageCount == 0)
                return R_CVULKAN_ERROR_NULL_POINTER;
        if (!pCreateInfo->pVertexInputInfo || !pCreateInfo->pInputAssemblyInfo || !pCreateInfo->pViewportInfo)
                return R_CVULKAN_ERROR_NULL_POINTER;
        if (!pCreateInfo->pRasterizationInfo || !pCreateInfo->pMultisampleInfo || !pCreateInfo->pColorBlendInfo)
                return R_CVULKAN_ERROR_NULL_POINTER;
        if (!pCreateInfo->pColorAttachmentFormats)
                return R_CVULKAN_ERROR_NULL_POINTER;
        if (!R_CVulkan_DeviceIsInitialized (pCreateInfo->pDevice))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
        if (!R_CVulkan_DeviceIsDynamicRenderingSupported (pCreateInfo->pDevice))
        {
                return R_CVULKAN_ERROR_FEATURE_NOT_PRESENT;
        }
#endif

        pPipeline->device = R_CVulkan_DeviceGetLogicalDevice (pCreateInfo->pDevice);
#if defined(R_CVULKAN_DEBUG)
        pPipeline->handle = VK_NULL_HANDLE;
        pPipeline->isInitialized = false;
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
                return R_CVULKAN_ERROR_PIPELINE_CREATE_FAILED;
        }

#if defined(R_CVULKAN_DEBUG)
        pPipeline->isInitialized = true;
#endif
        return R_CVULKAN_OK;
}

enum R_CVulkan_Error
R_CVulkan_NewComputePipeline (
    struct R_CVulkan_Pipeline*             pPipeline,
    const struct R_CVulkan_Device*         pDevice,
    VkPipelineLayout                       pipelineLayout,
    const VkPipelineShaderStageCreateInfo* pStage)
{
        R_CVULKAN_ASSERT (pPipeline);
        R_CVULKAN_ASSERT (pDevice);
        R_CVULKAN_ASSERT (pipelineLayout != VK_NULL_HANDLE);
        R_CVULKAN_ASSERT (pStage);

#if defined(R_CVULKAN_DEBUG)
        if (!pPipeline || !pDevice || pipelineLayout == VK_NULL_HANDLE || !pStage)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!R_CVulkan_DeviceIsInitialized (pDevice))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        pPipeline->device = R_CVulkan_DeviceGetLogicalDevice (pDevice);
#if defined(R_CVULKAN_DEBUG)
        pPipeline->handle = VK_NULL_HANDLE;
        pPipeline->isInitialized = false;
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
        pPipeline->isInitialized = true;
#endif
        return R_CVULKAN_OK;
}

void
R_CVulkan_Pipeline_Shutdown (struct R_CVulkan_Pipeline* pPipeline)
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
        pPipeline->isInitialized = false;
#endif
}

VkPipelineLayout
R_CVulkan_PipelineLayout_GetHandle (const struct R_CVulkan_PipelineLayout* pLayout)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pLayout != NULL);
#endif
        return pLayout->handle;
}

VkDevice
R_CVulkan_PipelineLayout_GetDevice (const struct R_CVulkan_PipelineLayout* pLayout)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pLayout != NULL);
#endif
        return pLayout->device;
}

int
R_CVulkan_PipelineLayout_IsInitialized (const struct R_CVulkan_PipelineLayout* pLayout)
{
#if defined(R_CVULKAN_DEBUG)
        return pLayout->isInitialized;
#else
        (void)pLayout;
        return 1;
#endif
}

VkPipeline
R_CVulkan_Pipeline_GetHandle (const struct R_CVulkan_Pipeline* pPipeline)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pPipeline != NULL);
#endif
        return pPipeline->handle;
}

VkDevice
R_CVulkan_Pipeline_GetDevice (const struct R_CVulkan_Pipeline* pPipeline)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pPipeline != NULL);
#endif
        return pPipeline->device;
}

int
R_CVulkan_Pipeline_IsInitialized (const struct R_CVulkan_Pipeline* pPipeline)
{
#if defined(R_CVULKAN_DEBUG)
        return pPipeline->isInitialized;
#else
        (void)pPipeline;
        return 1;
#endif
}
