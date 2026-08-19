#pragma once

#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

#include <vulkan/vulkan.h>
#include <inttypes.h>
#include <stdint.h>

/**
 * @file cvulkan_common.h
 * @brief Common definitions and error codes for R_CVulkan wrapper
 *
 * This module provides error codes and common utilities for the Vulkan wrapper layer.
 * These are separate from CSTL error codes to maintain clear separation.
 */

/**
 * @brief R_CVulkan wrapper error codes
 */
enum R_CVulkan_Error
{
        R_CVULKAN_OK = 0, /**< Success */
        R_CVULKAN_ERROR_FAILED = -1, /**< General failure */
        R_CVULKAN_ERROR_OUT_OF_MEMORY = -2, /**< Memory allocation failed */
        R_CVULKAN_ERROR_INVALID_ARGUMENT = -3, /**< Invalid function argument */
        R_CVULKAN_ERROR_NULL_POINTER = -4, /**< Null pointer passed */
        R_CVULKAN_ERROR_NOT_INITIALIZED = -5, /**< Vulkan not initialized */
        R_CVULKAN_ERROR_ALREADY_INITIALIZED = -6, /**< Already initialized */
        R_CVULKAN_ERROR_INSTANCE_CREATE_FAILED = -7, /**< Failed to create Vulkan instance */
        R_CVULKAN_ERROR_DEVICE_CREATE_FAILED = -8, /**< Failed to create Vulkan device */
        R_CVULKAN_ERROR_EXTENSION_NOT_FOUND = -9, /**< Required extension not found */
        R_CVULKAN_ERROR_LAYER_NOT_FOUND = -10, /**< Required layer not found */
        R_CVULKAN_ERROR_PHYSICAL_DEVICE_NOT_FOUND = -11, /**< No suitable physical device */
        R_CVULKAN_ERROR_QUEUE_FAMILY_NOT_FOUND = -12, /**< Required queue family not found */
        R_CVULKAN_ERROR_SURFACE_CREATE_FAILED = -13, /**< Failed to create surface */
        R_CVULKAN_ERROR_SWAPCHAIN_CREATE_FAILED = -14, /**< Failed to create swapchain */
        R_CVULKAN_ERROR_SWAPCHAIN_OUT_OF_DATE = -15, /**< Swapchain is out of date, needs recreation */
        R_CVULKAN_ERROR_SWAPCHAIN_SUBOPTIMAL = -16, /**< Swapchain is suboptimal, can recreate */
        R_CVULKAN_ERROR_MAP_MEMORY_FAILED = -17, /**< Failed to map memory */
        R_CVULKAN_ERROR_UNMAP_MEMORY_FAILED = -18, /**< Failed to unmap memory */
        R_CVULKAN_ERROR_FENCE_WAIT_FAILED = -19, /**< Failed to wait for fence */
        R_CVULKAN_ERROR_FENCE_RESET_FAILED = -20, /**< Failed to reset fence */
        R_CVULKAN_ERROR_BUFFER_CREATE_FAILED = -21, /**< Failed to create buffer */
        R_CVULKAN_ERROR_IMAGE_CREATE_FAILED = -22, /**< Failed to create image */
        R_CVULKAN_ERROR_MEMORY_ALLOCATE_FAILED = -23, /**< Failed to allocate memory */
        R_CVULKAN_ERROR_COMMAND_POOL_CREATE_FAILED = -24, /**< Failed to create command pool */
        R_CVULKAN_ERROR_COMMAND_BUFFER_BEGIN_FAILED = -25, /**< Failed to begin command buffer */
        R_CVULKAN_ERROR_COMMAND_BUFFER_END_FAILED = -26, /**< Failed to end command buffer */
        R_CVULKAN_ERROR_RENDER_PASS_CREATE_FAILED = -27, /**< Failed to create render pass */
        R_CVULKAN_ERROR_FRAMEBUFFER_CREATE_FAILED = -28, /**< Failed to create framebuffer */
        R_CVULKAN_ERROR_SHADER_MODULE_CREATE_FAILED = -29, /**< Failed to create shader module */
        R_CVULKAN_ERROR_PIPELINE_CREATE_FAILED = -30, /**< Failed to create pipeline */
        R_CVULKAN_ERROR_DESCRIPTOR_POOL_CREATE_FAILED = -31, /**< Failed to create descriptor pool */
        R_CVULKAN_ERROR_DESCRIPTOR_SET_LAYOUT_CREATE_FAILED
        = -32, /**< Failed to create descriptor set layout */
        R_CVULKAN_ERROR_SAMPLER_CREATE_FAILED = -33, /**< Failed to create sampler */
        R_CVULKAN_ERROR_DEVICE_LOST = -34, /**< Device lost, must exit immediately */
        R_CVULKAN_ERROR_SURFACE_LOST = -35, /**< Surface lost, needs recreation */
        R_CVULKAN_ERROR_OUT_OF_DATE_KHR = -36, /**< Out of date (window resize) */
        R_CVULKAN_ERROR_FEATURE_NOT_PRESENT = -37, /**< Required feature not present */
        R_CVULKAN_ERROR_UNKNOWN = -99 /**< Unknown error */
};

typedef VkDeviceSize                 R_CVulkanDeviceSize;
typedef VkFlags                      R_CVulkanFlags;
typedef VkBool32                     R_CVulkanBool32;
typedef VkSampleMask                 R_CVulkanSampleMask;
typedef VkDeviceAddress              R_CVulkanDeviceAddress;
typedef VkMemoryPropertyFlags        R_CVulkanMemoryPropertyFlags;
typedef VkMemoryPropertyFlagBits     R_CVulkanMemoryPropertyFlagBits;
typedef VkBufferUsageFlags           R_CVulkanBufferUsageFlags;
typedef VkBufferUsageFlagBits        R_CVulkanBufferUsageFlagBits;
typedef VkImageUsageFlags            R_CVulkanImageUsageFlags;
typedef VkImageUsageFlagBits         R_CVulkanImageUsageFlagBits;
typedef VkPipelineStageFlags         R_CVulkanPipelineStageFlags;
typedef VkPipelineStageFlagBits      R_CVulkanPipelineStageFlagBits;
typedef VkAccessFlags                R_CVulkanAccessFlags;
typedef VkAccessFlagBits             R_CVulkanAccessFlagBits;
typedef VkDependencyFlags            R_CVulkanDependencyFlags;
typedef VkDependencyFlagBits         R_CVulkanDependencyFlagBits;
typedef VkCommandPoolCreateFlags     R_CVulkanCommandPoolCreateFlags;
typedef VkCommandPoolCreateFlagBits  R_CVulkanCommandPoolCreateFlagBits;
typedef VkCommandPoolResetFlags      R_CVulkanCommandPoolResetFlags;
typedef VkCommandPoolResetFlagBits   R_CVulkanCommandPoolResetFlagBits;
typedef VkCommandBufferUsageFlags    R_CVulkanCommandBufferUsageFlags;
typedef VkCommandBufferUsageFlagBits R_CVulkanCommandBufferUsageFlagBits;
typedef VkCommandBufferResetFlags    R_CVulkanCommandBufferResetFlags;
typedef VkCommandBufferResetFlagBits R_CVulkanCommandBufferResetFlagBits;
typedef VkPipelineBindPoint          R_CVulkanPipelineBindPoint;
typedef VkPipelineLayout             R_CVulkanPipelineLayout;
typedef VkShaderStageFlags           R_CVulkanShaderStageFlags;
typedef VkShaderStageFlagBits        R_CVulkanShaderStageFlagBits;
typedef VkIndexType                  R_CVulkanIndexType;
typedef VkImageLayout                R_CVulkanImageLayout;
typedef VkSubpassContents            R_CVulkanSubpassContents;
typedef VkStencilFaceFlags           R_CVulkanStencilFaceFlags;
typedef VkStencilFaceFlagBits        R_CVulkanStencilFaceFlagBits;
typedef VkFenceCreateFlags           R_CVulkanFenceCreateFlags;
typedef VkFenceCreateFlagBits        R_CVulkanFenceCreateFlagBits;
typedef VkSemaphoreCreateFlags       R_CVulkanSemaphoreCreateFlags;

/**
 * @brief Get human-readable error message for an R_CVulkan error code
 * @param error The R_CVulkan error code
 * @return Static string describing the error, or "Unknown error" if not recognized
 */
const char* R_CVulkan_ErrorToString (enum R_CVulkan_Error error);

/**
 * @brief Convert Vulkan result to R_CVulkan errorcode
 * @param result Vulkan result code
 * @return Corresponding R_CVulkan error code
 */
enum R_CVulkan_Error R_CVulkan_ResultToError (const VkResult result);

/**
 * @brief Check if error requires swapchain recreation
 * @param error The R_CVulkan error code
 * @return 1 if error requires swapchain recreation, 0 otherwise
 */
int R_CVulkan_ShouldRecreateSwapchain (enum R_CVulkan_Error error);
