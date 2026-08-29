#pragma once

#include <vulkan/vulkan.h>
#include <stdint.h>

#include "rlgame.base/cvulkan/cvulkan_platform.h"

struct R_CVulkan_Device;

/**
 * @brief Dynamic rendering fallback render pass wrapper
 */
struct R_CVulkan_DYRRenderPass
{
        VkRenderPass handle; /**< Raw Vulkan render pass handle */
        VkDevice     device; /**< Associated device */
};

/**
 * @brief Configuration parameters for creating a fallback render pass from dynamic rendering info
 */
struct R_CVulkan_DYRRenderPassCreateInfo
{
        const struct R_CVulkan_Device* pDevice; /**< R_CVulkan device wrapper */
        uint32_t                       colorAttachmentCount; /**< Number of color attachments */
        const VkFormat*                pColorAttachmentFormats; /**< Color attachment formats */
        VkFormat                       depthAttachmentFormat; /**< Depth attachment format */
        VkFormat                       stencilAttachmentFormat; /**< Stencil attachment format */
};

/**
 * @brief Create a render pass from dynamic rendering attachment info
 * @param pDYRRenderPass Pointer to DYR render pass wrapper to initialize
 * @param pCreateInfo DYR render pass creation parameters
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_DYRCreateRenderPass (
    struct R_CVulkan_DYRRenderPass*                 pDYRRenderPass,
    const struct R_CVulkan_DYRRenderPassCreateInfo* pCreateInfo);

/**
 * @brief Delete a DYR render pass and destroy the Vulkan object
 * @param pDYRRenderPass Pointer to DYR render pass to delete
 */
R_CVULKAN_API void R_CVulkan_DYRDeleteRenderPass (struct R_CVulkan_DYRRenderPass* pDYRRenderPass);

/**
 * @brief Get the raw Vulkan render pass handle
 * @param pDYRRenderPass Pointer to DYR render pass
 * @return Vulkan render pass handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkRenderPass
R_CVulkan_DYRRenderPassGetHandle (const struct R_CVulkan_DYRRenderPass* pDYRRenderPass);

/**
 * @brief Check if the DYR render pass is initialized
 * @param pDYRRenderPass Pointer to DYR render pass
 * @return 1 if initialized, 0 otherwise
 */
R_CVULKAN_API int R_CVulkan_DYRRenderPassIsInitialized (const struct R_CVulkan_DYRRenderPass* pDYRRenderPass);
