#pragma once

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <inttypes.h>

#include "rlgame.base/cvulkan/cvulkan_common.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

struct R_CVulkan_Device;

/**
 * @brief Safe wrapper for VkSampler
 */
struct R_CVulkan_Sampler
{
                VkSampler handle; /**< Raw Vulkan sampler handle */
                VkDevice  device; /**< Associated device */
                R_CVULKAN_DEBUG_FIELD
};

/**
 * @brief Initialize a sampler
 * @param pSampler Pointer to sampler to initialize
 * @param pDevice R_CVulkan device wrapper
 * @param magFilter Magnification filter
 * @param minFilter Minification filter
 * @param mipmapMode Mipmap mode
 * @param addressModeU Address mode for U coordinate
 * @param addressModeV Address mode for V coordinate
 * @param addressModeW Address mode for W coordinate
 * @param mipLodBias Mipmap LOD bias
 * @param anisotropyEnable Whether to enable anisotropic filtering
 * @param maxAnisotropy Maximum anisotropy
 * @param minLod Minimum LOD
 * @param maxLod Maximum LOD
 * @param borderColor Border color (for clamp to border)
 * @param unnormalizedCoordinates Whether to use unnormalized coordinates
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_NewSampler (
    struct R_CVulkan_Sampler*      pSampler,
    const struct R_CVulkan_Device* pDevice,
    VkFilter                       magFilter,
    VkFilter                       minFilter,
    VkSamplerMipmapMode            mipmapMode,
    VkSamplerAddressMode           addressModeU,
    VkSamplerAddressMode           addressModeV,
    VkSamplerAddressMode           addressModeW,
    float                          mipLodBias,
    int                            anisotropyEnable,
    float                          maxAnisotropy,
    float                          minLod,
    float                          maxLod,
    VkBorderColor                  borderColor,
    int                            unnormalizedCoordinates);

/**
 * @brief Deletes a sampler and destroy the Vulkan object
 * @param pSampler Pointer to sampler to delete
 */
R_CVULKAN_API void R_CVulkan_DeleteSampler (struct R_CVulkan_Sampler* pSampler);

/**
 * @brief Get the raw Vulkan sampler handle
 * @param pSampler Pointer to sampler
 * @return Vulkan sampler handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkSampler R_CVulkan_SamplerGetHandle (const struct R_CVulkan_Sampler* pSampler);

/**
 * @brief Get the associated device
 * @param pSampler Pointer to sampler
 * @return Vulkan device handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkDevice R_CVulkan_SamplerGetDevice (const struct R_CVulkan_Sampler* pSampler);

/**
 * @brief Check if the sampler is initialized
 * @param pSampler Pointer to sampler
 * @return 1 if initialized, 0 otherwise
 */
R_CVULKAN_API int R_CVulkan_SamplerIsInitialized (const struct R_CVulkan_Sampler* pSampler);
