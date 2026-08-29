#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

#include "rlgame.base/cvulkan/cvulkan_platform.h"

struct R_CVulkan_Device;

/**
 * @brief Configuration parameters for render pass creation
 */
struct R_CVulkan_RenderPassCreateInfo
{
        const struct R_CVulkan_Device* pDevice; /**< R_CVulkan device wrapper */
        const VkAttachmentDescription* pAttachments; /**< Array of attachment descriptions */
        uint32_t                       attachmentCount; /**< Number of attachments */
        const VkSubpassDescription*    pSubpasses; /**< Array of subpass descriptions */
        uint32_t                       subpassCount; /**< Number of subpasses */
        const VkSubpassDependency*     pDependencies; /**< Array of subpass dependencies */
        uint32_t                       dependencyCount; /**< Number of dependencies */
};

/**
 * @brief Safe wrapper for VkRenderPass
 */
struct R_CVulkan_RenderPass
{
        VkRenderPass handle; /**< Raw Vulkan render pass handle */
        VkDevice     device; /**< Associated device */
};

/**
 * @brief Initialize a render pass
 * @param pRenderPass Pointer to render pass to initialize
 * @param pCreateInfo Render pass creation parameters
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_NewRenderPass (
    struct R_CVulkan_RenderPass*                 pRenderPass,
    const struct R_CVulkan_RenderPassCreateInfo* pCreateInfo);

/**
 * @brief Deletes a render pass and destroy the Vulkan object
 * @param pRenderPass Pointer to render pass to delete
 */
R_CVULKAN_API void R_CVulkan_DeleteRenderPass (struct R_CVulkan_RenderPass* pRenderPass);

/**
 * @brief Get the raw Vulkan render pass handle
 * @param pRenderPass Pointer to render pass
 * @return Vulkan render pass handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkRenderPass R_CVulkan_RenderPassGetHandle (const struct R_CVulkan_RenderPass* pRenderPass);

/**
 * @brief Get the associated device
 * @param pRenderPass Pointer to render pass
 * @return Vulkan device handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkDevice R_CVulkan_RenderPassGetDevice (const struct R_CVulkan_RenderPass* pRenderPass);

/**
 * @brief Check if the render pass is initialized
 * @param pRenderPass Pointer to render pass
 * @return 1 if initialized, 0 otherwise
 */
R_CVULKAN_API int R_CVulkan_RenderPassIsInitialized (const struct R_CVulkan_RenderPass* pRenderPass);
