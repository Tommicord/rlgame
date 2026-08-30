#pragma once

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <inttypes.h>

#include "rlgame.base/cvulkan/cvulkan_platform.h"

struct R_CVulkan_Device;

/**
 * @brief Settingsuration parameters for sampler creation
 */
struct R_CVulkan_SamplerCreateInfo
{
        const struct R_CVulkan_Device* pDevice; /**< R_CVulkan device wrapper */
        VkFilter                       magFilter; /**< Magnification filter */
        VkFilter                       minFilter; /**< Minification filter */
        VkSamplerMipmapMode            mipmapMode; /**< Mipmap mode */
        VkSamplerAddressMode           addressModeU; /**< Address mode for U coordinate */
        VkSamplerAddressMode           addressModeV; /**< Address mode for V coordinate */
        VkSamplerAddressMode           addressModeW; /**< Address mode for W coordinate */
        float                          mipLodBias; /**< Mipmap LOD bias */
        int                            anisotropyEnable; /**< Whether to enable anisotropic filtering */
        float                          maxAnisotropy; /**< Maximum anisotropy */
        float                          minLod; /**< Minimum LOD */
        float                          maxLod; /**< Maximum LOD */
        VkBorderColor                  borderColor; /**< Border color (for clamp to border) */
        int unnormalizedCoordinates; /**< Whether to use unnormalized coordinates */
};

/**
 * @brief Safe wrapper for VkSampler
 */
struct R_CVulkan_Sampler
{
        VkSampler handle; /**< Raw Vulkan sampler handle */
        VkDevice  device; /**< Associated device */
};

/**
 * @brief Initialize a sampler
 * @param pSampler Pointer to sampler to initialize
 * @param pCreateInfo Sampler creation parameters
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_NewSampler (
    struct R_CVulkan_Sampler*                 pSampler,
    const struct R_CVulkan_SamplerCreateInfo* pCreateInfo);

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
