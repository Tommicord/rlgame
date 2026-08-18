#pragma once

#include <inttypes.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

#include "rlgame.base/cvulkan/cvulkan_common.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

struct R_CVulkan_Device;

/**
 * @brief Safe wrapper for VkImage
 */
struct R_CVulkan_Image
{
                VkImage                      handle; /**< Raw Vulkan image handle */
                VkDeviceMemory               memory; /**< Device memory handle */
                VkDevice                     device; /**< Associated device */
                R_CVulkanDeviceSize          size; /**< Image size in bytes */
                uint32_t                     width; /**< Image width */
                uint32_t                     height; /**< Image height */
                uint32_t                     mipLevels; /**< Number of mip levels */
                uint32_t                     arrayLayers; /**< Number of array layers */
                VkFormat                     format; /**< Image format */
                R_CVulkanImageUsageFlags     usage; /**< Image usage flags */
                R_CVulkanMemoryPropertyFlags properties; /**< Memory property flags */
                VkImageLayout                currentLayout; /**< Current image layout */
                VkImageType                  imageType; /**< Image type (1D, 2D, 3D) */
                VkSampleCountFlagBits        samples; /**< Number of samples */
                VkImageTiling                tiling; /**< Image tiling mode */
                R_CVULKAN_DEBUG_FIELD
} R_CVulkan_Image;

/**
 * @brief Initialize an image
 * @param image Pointer to image to initialize
 * @param device R_CVulkan device wrapper
 * @param physicalDevice Physical device (raw Vulkan handle, no wrapper yet)
 * @param imageType Image type
 * @param extent Image extent (width, height, depth)
 * @param mipLevels Number of mip levels
 * @param arrayLayers Number of array layers
 * @param format Image format
 * @param tiling Image tiling mode
 * @param usage Image usage flags
 * @param properties Memory property flags
 * @param samples Number of samples
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_NewImage (
    struct R_CVulkan_Image*        image,
    const struct R_CVulkan_Device* device,
    VkPhysicalDevice               physicalDevice,
    VkImageType                    imageType,
    VkExtent3D                     extent,
    uint32_t                       mipLevels,
    uint32_t                       arrayLayers,
    VkFormat                       format,
    VkImageTiling                  tiling,
    R_CVulkanImageUsageFlags       usage,
    R_CVulkanMemoryPropertyFlags   properties,
    VkSampleCountFlagBits          samples);

/**
 * @brief Deletes an image and destroy the Vulkan objects
 * @param image Pointer to image to delete
 */
void R_CVulkan_DeleteImage (R_CVulkan_Image* image);

/**
 * @brief Transition image layout
 * @param image Pointer to image
 * @param commandBuffer Command buffer to record the transition on (raw handle)
 * @param oldLayout Current layout
 * @param newLayout Desired layout
 * @param srcStageMask Source pipeline stage mask
 * @param dstStageMask Destination pipeline stage mask
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_ImageTransitionLayout (
    struct R_CVulkan_Image*     image,
    VkCommandBuffer             commandBuffer,
    VkImageLayout               oldLayout,
    VkImageLayout               newLayout,
    R_CVulkanPipelineStageFlags srcStageMask,
    R_CVulkanPipelineStageFlags dstStageMask);

/**
 * @brief Copy data to image memory
 * @param image Pointer to image
 * @param data Pointer to source data
 * @param dataSize Size of data in bytes
 * @param buffer Staging buffer to use for the copy (raw handle)
 * @param commandBuffer Command buffer to record the copy on (raw handle)
 * @return R_CVULKAN_ERROR_OK on success, error code otherwise
 */
enum R_CVulkan_Error R_CVulkan_ImageCopyData (
    struct R_CVulkan_Image* image,
    const void*             data,
    R_CVulkanDeviceSize     dataSize,
    VkBuffer                buffer,
    VkCommandBuffer         commandBuffer);

/**
 * @brief Get the raw Vulkan image handle
 * @param image Pointer to image
 * @return Vulkan image handle, or VK_NULL_HANDLE if not initialized
 */
VkImage R_CVulkan_ImageGetHandle (const struct R_CVulkan_Image* image);

/**
 * @brief Get the device memory handle
 * @param image Pointer to image
 * @return Device memory handle, or VK_NULL_HANDLE if not initialized
 */
VkDeviceMemory R_CVulkan_ImageGetMemory (const struct R_CVulkan_Image* image);

/**
 * @brief Get the associated device
 * @param image Pointer to image
 * @return Vulkan device handle, or VK_NULL_HANDLE if not initialized
 */
VkDevice R_CVulkan_ImageGetDevice (const struct R_CVulkan_Image* image);

/**
 * @brief Get the image width
 * @param image Pointer to image
 * @return Image width, or 0 if not initialized
 */
uint32_t R_CVulkan_ImageGetWidth (const struct R_CVulkan_Image* image);

/**
 * @brief Get the image height
 * @param image Pointer to image
 * @return Image height, or 0 if not initialized
 */
uint32_t R_CVulkan_ImageGetHeight (const struct R_CVulkan_Image* image);

/**
 * @brief Get the image format
 * @param image Pointer to image
 * @return Image format, or VK_FORMAT_UNDEFINED if not initialized
 */
VkFormat R_CVulkan_Image_GetFormat (const struct R_CVulkan_Image* image);

/**
 * @brief Get the current image layout
 * @param image Pointer to image
 * @return Current image layout, or VK_IMAGE_LAYOUT_UNDEFINED if not initialized
 */
VkImageLayout R_CVulkan_ImageGetLayout (const struct R_CVulkan_Image* image);

/**
 * @brief Check if the image is initialized
 * @param image Pointer to image
 * @return 1 if initialized, 0 otherwise
 */
int R_CVulkan_ImageIsInitialized (const struct R_CVulkan_Image* image);
