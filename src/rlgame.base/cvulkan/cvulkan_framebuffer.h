#pragma once

#include <stdint.h>
#include <inttypes.h>
#include <vulkan/vulkan.h>

#include "rlgame.base/cvulkan/cvulkan_common.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

struct R_CVulkan_Device;

/**
 * @brief Configuration parameters for framebuffer creation
 */
struct R_CVulkan_FramebufferCreateInfo
{
                const struct R_CVulkan_Device* pDevice; /**< R_CVulkan device wrapper */
                VkRenderPass                   pRenderPass; /**< Render pass to use with this framebuffer */
                const VkImageView*             pAttachments; /**< Array of image view attachments */
                uint32_t                       attachmentCount; /**< Number of attachments */
                uint32_t                       width; /**< Framebuffer width */
                uint32_t                       height; /**< Framebuffer height */
                uint32_t                       layers; /**< Framebuffer layers */
};

/**
 * @brief Safe wrapper for VkFramebuffer
 */
struct R_CVulkan_Framebuffer
{
                VkFramebuffer handle; /**< Raw Vulkan framebuffer handle */
                VkDevice      device; /**< Associated device */
                VkRenderPass  renderPass; /**< Associated render pass */
                uint32_t      width; /**< Framebuffer width */
                uint32_t      height; /**< Framebuffer height */
                uint32_t      attachmentCount; /**< Number of attachments */
                R_CVULKAN_DEBUG_FIELD
};

/**
 * @brief Initialize a framebuffer
 * @param pFramebuffer Pointer to framebuffer to initialize
 * @param pCreateInfo Framebuffer creation parameters
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_NewFramebuffer (
    struct R_CVulkan_Framebuffer*                 pFramebuffer,
    const struct R_CVulkan_FramebufferCreateInfo* pCreateInfo);

/**
 * @brief Deletes a framebuffer and destroy the Vulkan object
 * @param framebuffer Pointer to framebuffer to delete
 */
R_CVULKAN_API void R_CVulkan_DeleteFramebuffer (struct R_CVulkan_Framebuffer* pFramebuffer);

/**
 * @brief Get the raw Vulkan framebuffer handle
 * @param framebuffer Pointer to framebuffer
 * @return Vulkan framebuffer handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkFramebuffer R_CVulkan_FramebufferGetHandle (const struct R_CVulkan_Framebuffer* pFramebuffer);

/**
 * @brief Get the associated device
 * @param framebuffer Pointer to framebuffer
 * @return Vulkan device handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkDevice R_CVulkan_FramebufferGetDevice (const struct R_CVulkan_Framebuffer* pFramebuffer);

/**
 * @brief Get the associated render pass
 * @param framebuffer Pointer to framebuffer
 * @return Render pass handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkRenderPass
R_CVulkan_FramebufferGetRenderPass (const struct R_CVulkan_Framebuffer* pFramebuffer);

/**
 * @brief Get the framebuffer width
 * @param framebuffer Pointer to framebuffer
 * @return Framebuffer width, or 0 if not initialized
 */
R_CVULKAN_API uint32_t R_CVulkan_FramebufferGetWidth (const struct R_CVulkan_Framebuffer* pFramebuffer);

/**
 * @brief Get the framebuffer height
 * @param framebuffer Pointer to framebuffer
 * @return Framebuffer height, or 0 if not initialized
 */
R_CVULKAN_API uint32_t R_CVulkan_FramebufferGetHeight (const struct R_CVulkan_Framebuffer* pFramebuffer);

/**
 * @brief Get the number of attachments
 * @param framebuffer Pointer to framebuffer
 * @return Number of attachments, or 0 if not initialized
 */
R_CVULKAN_API uint32_t
R_CVulkan_FramebufferGetAttachmentCount (const struct R_CVulkan_Framebuffer* pFramebuffer);

/**
 * @brief Check if the framebuffer is initialized
 * @param framebuffer Pointer to framebuffer
 * @return 1 if initialized, 0 otherwise
 */
R_CVULKAN_API int R_CVulkan_FramebufferIsInitialized (const struct R_CVulkan_Framebuffer* pFramebuffer);
