#pragma once

#include <inttypes.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

#include "rlgame.base/cvulkan/cvulkan_platform.h"

struct R_CVulkan_Device;

/**
 * @brief Settingsuration parameters for image creation
 */
struct r_cvulkan_image_create_info
{
        const struct R_CVulkan_Device* device; /**< R_CVulkan device wrapper */
        VkPhysicalDevice               physicalDevice; /**< Physical device */
        VkImageType                    imageType; /**< Image type (1D, 2D, 3D) */
        VkExtent3D                     extent; /**< Image extent (width, height, depth) */
        uint32_t                       mipLevels; /**< Number of mip levels */
        uint32_t                       arrayLayers; /**< Number of array layers */
        VkFormat                       format; /**< Image format */
        VkImageTiling                  tiling; /**< Image tiling mode */
        VkImageUsageFlags              usage; /**< Image usage flags */
        VkMemoryPropertyFlags          properties; /**< Memory property flags */
        VkSampleCountFlagBits          samples; /**< Number of samples */
};

/**
 * @brief Safe wrapper for VkImage
 */
struct R_CVulkan_Image
{
        VkImage               handle; /**< Raw Vulkan image handle */
        VkDeviceMemory        memory; /**< Device memory handle */
        VkDevice              device; /**< Associated device */
        VkDeviceSize          size; /**< Image size in bytes */
        uint32_t              width; /**< Image width */
        uint32_t              height; /**< Image height */
        uint32_t              mipLevels; /**< Number of mip levels */
        uint32_t              arrayLayers; /**< Number of array layers */
        VkFormat              format; /**< Image format */
        VkImageUsageFlags     usage; /**< Image usage flags */
        VkMemoryPropertyFlags properties; /**< Memory property flags */
        VkImageLayout         currentLayout; /**< Current image layout */
        VkImageType           imageType; /**< Image type (1D, 2D, 3D) */
        VkSampleCountFlagBits samples; /**< Number of samples */
        VkImageTiling         tiling; /**< Image tiling mode */
};

/**
 * @brief Initialize an image
 * @param image Pointer to image to initialize
 * @param pCreateInfo Image creation parameters
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_NewImage (struct R_CVulkan_Image* pImage, const struct r_cvulkan_image_create_info* pCreateInfo);

/**
 * @brief Deletes an image and destroy the Vulkan objects
 * @param image Pointer to image to delete
 */
R_CVULKAN_API void R_CVulkan_DeleteImage (struct R_CVulkan_Image* pImage);

/**
 * @brief Transition image layout
 * @param image Pointer to image
 * @param commandBuffer Command buffer to record the transition on (raw handle)
 * @param oldLayout Current layout
 * @param newLayout Desired layout
 * @param srcStageMask Source pipeline stage mask
 * @param dstStageMask Destination pipeline stage mask
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error r_cvulkan_image_transition_layout (
    struct R_CVulkan_Image* pImage,
    VkCommandBuffer         commandBuffer,
    VkImageLayout           oldLayout,
    VkImageLayout           newLayout,
    VkPipelineStageFlags    srcStageMask,
    VkPipelineStageFlags    dstStageMask);

/**
 * @brief Copy data to image memory
 * @param image Pointer to image
 * @param data Pointer to source data
 * @param dataSize Size of data in bytes
 * @param buffer Staging buffer to use for the copy (raw handle)
 * @param commandBuffer Command buffer to record the copy on (raw handle)
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error r_cvulkan_image_copy_data (
    struct R_CVulkan_Image* pImage,
    const void*             data,
    VkDeviceSize            dataSize,
    VkBuffer                buffer,
    VkCommandBuffer         commandBuffer);

/**
 * @brief Get the raw Vulkan image handle
 * @param image Pointer to image
 * @return Vulkan image handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkImage r_cvulkan_image_get_handle (const struct R_CVulkan_Image* pImage);

/**
 * @brief Get the device memory handle
 * @param image Pointer to image
 * @return Device memory handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkDeviceMemory r_cvulkan_image_get_memory (const struct R_CVulkan_Image* pImage);

/**
 * @brief Get the associated device
 * @param image Pointer to image
 * @return Vulkan device handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkDevice r_cvulkan_image_get_device (const struct R_CVulkan_Image* pImage);

/**
 * @brief Get the image width
 * @param image Pointer to image
 * @return Image width, or 0 if not initialized
 */
R_CVULKAN_API uint32_t r_cvulkan_image_get_width (const struct R_CVulkan_Image* pImage);

/**
 * @brief Get the image height
 * @param image Pointer to image
 * @return Image height, or 0 if not initialized
 */
R_CVULKAN_API uint32_t r_cvulkan_image_get_height (const struct R_CVulkan_Image* pImage);

/**
 * @brief Get the image format
 * @param image Pointer to image
 * @return Image format, or VK_FORMAT_UNDEFINED if not initialized
 */
R_CVULKAN_API VkFormat r_cvulkan_image_get_format (const struct R_CVulkan_Image* pImage);

/**
 * @brief Get the current image layout
 * @param image Pointer to image
 * @return Current image layout, or VK_IMAGE_LAYOUT_UNDEFINED if not initialized
 */
R_CVULKAN_API VkImageLayout r_cvulkan_image_get_layout (const struct R_CVulkan_Image* pImage);

/**
 * @brief Check if the image is initialized
 * @param image Pointer to image
 * @return 1 if initialized, 0 otherwise
 */
R_CVULKAN_API int r_cvulkan_image_is_initialized (const struct R_CVulkan_Image* pImage);
