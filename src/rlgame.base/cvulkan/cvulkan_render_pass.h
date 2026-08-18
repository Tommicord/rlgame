#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

#include "rlgame.base/cvulkan/cvulkan_common.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

struct R_CVulkan_Device;

/**
 * @brief Safe wrapper for VkRenderPass
 */
struct R_CVulkan_RenderPass
{
                VkRenderPass handle; /**< Raw Vulkan render pass handle */
                VkDevice     device; /**< Associated device */
                R_CVULKAN_DEBUG_FIELD
};

/**
 * @brief Initialize a render pass
 * @param pRenderPass Pointer to render pass to initialize
 * @param pDevice R_CVulkan device wrapper
 * @param pAttachments Array of attachment descriptions
 * @param attachmentCount Number of attachments
 * @param pSubpasses Array of subpass descriptions
 * @param subpassCount Number of subpasses
 * @param pDependencies Array of subpass dependencies (can be NULL)
 * @param dependencyCount Number of dependencies (can be 0)
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_NewRenderPass (
    struct R_CVulkan_RenderPass*   pRenderPass,
    const struct R_CVulkan_Device* pDevice,
    const VkAttachmentDescription* pAttachments,
    uint32_t                       attachmentCount,
    const VkSubpassDescription*    pSubpasses,
    uint32_t                       subpassCount,
    const VkSubpassDependency*     pDependencies,
    uint32_t                       dependencyCount);

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
