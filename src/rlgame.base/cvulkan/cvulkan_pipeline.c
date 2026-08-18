#include "rlgame.base/cvulkan/cvulkan_pipeline.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

#include <string.h>
#include <stdlib.h>

enum R_CVulkan_Error
R_CVulkan_NewPipelineLayout (
    struct R_CVulkan_PipelineLayout*    pLayout,
    const struct R_CVulkan_Device*      pDevice,
    const VkDescriptorSetLayout*        pSetLayouts,
    uint32_t                            setLayoutCount,
    const VkPushConstantRange*          pPushConstantRanges,
    uint32_t                            pushConstantRangeCount)
{
        R_CVULKAN_ASSERT (pLayout);
        R_CVULKAN_ASSERT (pDevice);

#if defined(R_CVULKAN_DEBUG)
        if (!pLayout || !pDevice)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!R_CVulkan_DeviceIsInitialized (pDevice))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        pLayout->device = R_CVulkan_DeviceGetLogicalDevice (pDevice);
#if defined(R_CVULKAN_DEBUG)
        pLayout->handle = VK_NULL_HANDLE;
        pLayout->isInitialized = false;
#endif

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = setLayoutCount;
        pipelineLayoutInfo.pSetLayouts = pSetLayouts;
        pipelineLayoutInfo.pushConstantRangeCount = pushConstantRangeCount;
        pipelineLayoutInfo.pPushConstantRanges = pPushConstantRanges;

        VkResult result = vkCreatePipelineLayout (pLayout->device, &pipelineLayoutInfo, NULL, &pLayout->handle);
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
R_CVulkan_GraphicsPipeline_Init (
    struct R_CVulkan_Pipeline*                    pPipeline,
    const struct R_CVulkan_Device*                pDevice,
    VkPipelineLayout                              pipelineLayout,
    VkRenderPass                                  pRenderPass,
    const VkPipelineShaderStageCreateInfo*        pStages,
    uint32_t                                      stageCount,
    const VkPipelineVertexInputStateCreateInfo*   pVertexInputInfo,
    const VkPipelineInputAssemblyStateCreateInfo* pInputAssemblyInfo,
    const VkPipelineViewportStateCreateInfo*      pViewportInfo,
    const VkPipelineRasterizationStateCreateInfo* pRasterizationInfo,
    const VkPipelineMultisampleStateCreateInfo*   pMultisampleInfo,
    const VkPipelineDepthStencilStateCreateInfo*  pDepthStencilInfo,
    const VkPipelineColorBlendStateCreateInfo*    pColorBlendInfo,
    const VkPipelineDynamicStateCreateInfo*       pDynamicStateInfo,
    uint32_t                                      subpass)
{
        R_CVULKAN_ASSERT (pPipeline);
        R_CVULKAN_ASSERT (pDevice);
        R_CVULKAN_ASSERT (pipelineLayout != VK_NULL_HANDLE);
        R_CVULKAN_ASSERT (pRenderPass != VK_NULL_HANDLE);
        R_CVULKAN_ASSERT (pStages);
        R_CVULKAN_ASSERT (stageCount > 0);
        R_CVULKAN_ASSERT (pVertexInputInfo);
        R_CVULKAN_ASSERT (pInputAssemblyInfo);
        R_CVULKAN_ASSERT (pViewportInfo);
        R_CVULKAN_ASSERT (pRasterizationInfo);
        R_CVULKAN_ASSERT (pMultisampleInfo);
        R_CVULKAN_ASSERT (pColorBlendInfo);

#if defined(R_CVULKAN_DEBUG)
        if (!pPipeline || !pDevice || pipelineLayout == VK_NULL_HANDLE)
                return R_CVULKAN_ERROR_NULL_POINTER;
        if (pRenderPass == VK_NULL_HANDLE || !pStages || stageCount == 0)
                return R_CVULKAN_ERROR_NULL_POINTER;
        if (!pVertexInputInfo || !pInputAssemblyInfo || !pViewportInfo)
                return R_CVULKAN_ERROR_NULL_POINTER;
        if (!pRasterizationInfo || !pMultisampleInfo || !pColorBlendInfo)
                return R_CVULKAN_ERROR_NULL_POINTER;
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

        VkGraphicsPipelineCreateInfo pipelineInfo = {0};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = stageCount;
        pipelineInfo.pStages = pStages;
        pipelineInfo.pVertexInputState = pVertexInputInfo;
        pipelineInfo.pInputAssemblyState = pInputAssemblyInfo;
        pipelineInfo.pViewportState = pViewportInfo;
        pipelineInfo.pRasterizationState = pRasterizationInfo;
        pipelineInfo.pMultisampleState = pMultisampleInfo;
        pipelineInfo.pDepthStencilState = pDepthStencilInfo;
        pipelineInfo.pColorBlendState = pColorBlendInfo;
        pipelineInfo.pDynamicState = pDynamicStateInfo;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = pRenderPass;
        pipelineInfo.subpass = subpass;
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
R_CVulkan_ComputePipeline_Init (
    struct R_CVulkan_Pipeline*             pPipeline,
    const struct R_CVulkan_Device*         pDevice,
    VkPipelineLayout                       pipelineLayout,
    const VkPipelineShaderStageCreateInfo* stage)
{
        R_CVULKAN_ASSERT (pPipeline);
        R_CVULKAN_ASSERT (pDevice);
        R_CVULKAN_ASSERT (pipelineLayout != VK_NULL_HANDLE);
        R_CVULKAN_ASSERT (stage);

#if defined(R_CVULKAN_DEBUG)
        if (!pPipeline || !pDevice || pipelineLayout == VK_NULL_HANDLE || !stage)
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
        pipelineInfo.stage = *stage;
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
