#pragma once

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <inttypes.h>

#include "rlgame.base/cvulkan/cvulkan_common.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

struct R_CVulkan_Device;

/**
 * @brief Safe wrapper for VkPipelineLayout
 */
struct R_CVulkan_PipelineLayout
{
                VkPipelineLayout handle; /**< Raw Vulkan pipeline layout handle */
                VkDevice         device; /**< Associated device */
                R_CVULKAN_DEBUG_FIELD
};

/**
 * @brief Safe wrapper for VkPipeline
 */
struct R_CVulkan_Pipeline
{
                VkPipeline handle; /**< Raw Vulkan pipeline handle */
                VkDevice   device; /**< Associated device */
                R_CVULKAN_DEBUG_FIELD
};

/**
 * @brief Initialize a pipeline layout
 * @param pLayout Pointer to layout to initialize
 * @param pDevice R_CVulkan device wrapper
 * @param pSetLayouts Array of descriptor set layouts
 * @param setLayoutCount Number of descriptor set layouts
 * @param pPushConstantRanges Array of push constant ranges (can be NULL)
 * @param pushConstantRangeCount Number of push constant ranges (can be 0)
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_NewPipelineLayout (
    struct R_CVulkan_PipelineLayout* pLayout,
    const struct R_CVulkan_Device*   pDevice,
    const VkDescriptorSetLayout*     pSetLayouts,
    uint32_t                         setLayoutCount,
    const VkPushConstantRange*       pPushConstantRanges,
    uint32_t                         pushConstantRangeCount);

/**
 * @brief Deletes a pipeline layout and destroy the Vulkan object
 * @param pLayout Pointer to layout to delete
 */
R_CVULKAN_API void R_CVulkan_DeletePipelineLayout (struct R_CVulkan_PipelineLayout* pLayout);

/**
 * @brief Initialize a graphics pipeline
 * @param pPipeline Pointer to pipeline to initialize
 * @param pDevice R_CVulkan device wrapper
 * @param pipelineLayout Pipeline layout
 * @param pRenderPass Render pass
 * @param pStages Array of shader stages
 * @param stageCount Number of shader stages
 * @param pVertexInputInfo Vertex input state
 * @param pInputAssemblyInfo Input assembly state
 * @param pViewportInfo Viewport state
 * @param pRasterizationInfo Rasterization state
 * @param pMultisampleInfo Multisample state
 * @param pDepthStencilInfo Depth stencil state (can be NULL)
 * @param pColorBlendInfo Color blend state
 * @param pDynamicStateInfo Dynamic state (can be NULL)
 * @param subpass Subpass index
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_NewGraphicsPipeline (
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
    uint32_t                                      subpass);

/**
 * @brief Initialize a compute pipeline
 * @param pPipeline Pointer to pipeline to initialize
 * @param pDevice R_CVulkan device wrapper
 * @param pipelineLayout Pipeline layout
 * @param pStage Compute shader stage
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_NewComputePipeline (
    struct R_CVulkan_Pipeline*             pPipeline,
    const struct R_CVulkan_Device*         pDevice,
    VkPipelineLayout                       pipelineLayout,
    const VkPipelineShaderStageCreateInfo* pStage);

/**
 * @brief Deletes a pipeline and destroy the Vulkan object
 * @param pPipeline Pointer to pipeline to delete
 */
R_CVULKAN_API void R_CVulkan_DeletePipeline (struct R_CVulkan_Pipeline* pPipeline);

/**
 * @brief Get the raw Vulkan pipeline layout handle
 * @param pLayout Pointer to layout
 * @return Vulkan pipeline layout handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkPipelineLayout R_CVulkan_PipelineLayoutGetHandle (const struct R_CVulkan_PipelineLayout* pLayout);

/**
 * @brief Get the associated device for layout
 * @param pLayout Pointer to layout
 * @return Vulkan device handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkDevice R_CVulkan_PipelineLayoutGetDevice (const struct R_CVulkan_PipelineLayout* pLayout);

/**
 * @brief Check if the layout is initialized
 * @param pLayout Pointer to layout
 * @return 1 if initialized, 0 otherwise
 */
R_CVULKAN_API int R_CVulkan_PipelineLayoutIsInitialized (const struct R_CVulkan_PipelineLayout* pLayout);

/**
 * @brief Get the raw Vulkan pipeline handle
 * @param pPipeline Pointer to pipeline
 * @return Vulkan pipeline handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkPipeline R_CVulkan_PipelineGetHandle (const struct R_CVulkan_Pipeline* pPipeline);

/**
 * @brief Get the associated device for pipeline
 * @param pPipeline Pointer to pipeline
 * @return Vulkan device handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkDevice R_CVulkan_PipelineGetDevice (const struct R_CVulkan_Pipeline* pPipeline);

/**
 * @brief Check if the pipeline is initialized
 * @param pPipeline Pointer to pipeline
 * @return 1 if initialized, 0 otherwise
 */
R_CVULKAN_API int R_CVulkan_PipelineIsInitialized (const struct R_CVulkan_Pipeline* pPipeline);
