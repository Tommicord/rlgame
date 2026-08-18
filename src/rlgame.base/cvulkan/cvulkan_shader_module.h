#pragma once

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <inttypes.h>

#include "rlgame.base/cvulkan/cvulkan_common.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

struct R_CVulkan_Device;

/**
 * @brief Safe wrapper for VkShaderModule
 */
struct R_CVulkan_ShaderModule
{
                VkShaderModule handle; /**< Raw Vulkan shader module handle */
                VkDevice       device; /**< Associated device */
                size_t         codeSize; /**< Size of SPIR-V code in bytes */
                R_CVULKAN_DEBUG_FIELD
};

/**
 * @brief Initialize a shader module from SPIR-V code
 * @param pShaderModule Pointer to shader module to initialize
 * @param device R_CVulkan device wrapper
 * @param pCode Pointer to SPIR-V code
 * @param codeSize Size of SPIR-V code in bytes
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_NewShaderModule (
    struct R_CVulkan_ShaderModule* pShaderModule,
    const struct R_CVulkan_Device* pDevice,
    const uint32_t*                pCode,
    size_t                         codeSize);

/**
 * @brief Deletes a shader module and destroy the Vulkan object
 * @param pShaderModule Pointer to shader module to delete
 */
R_CVULKAN_API void R_CVulkan_DeleteShaderModule (struct R_CVulkan_ShaderModule* pShaderModule);

/**
 * @brief Get the raw Vulkan shader module handle
 * @param pShaderModule Pointer to shader module
 * @return Vulkan shader module handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkShaderModule R_CVulkan_ShaderModuleGetHandle (const struct R_CVulkan_ShaderModule* pShaderModule);

/**
 * @brief Get the associated device
 * @param pShaderModule Pointer to shader module
 * @return Vulkan device handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkDevice R_CVulkan_ShaderModuleGetDevice (const struct R_CVulkan_ShaderModule* pShaderModule);

/**
 * @brief Get the SPIR-V code size
 * @param pShaderModule Pointer to shader module
 * @return Code size in bytes, or 0 if not initialized
 */
R_CVULKAN_API size_t R_CVulkan_ShaderModuleGetCodeSize (const struct R_CVulkan_ShaderModule* pShaderModule);

/**
 * @brief Check if the shader module is initialized
 * @param pShaderModule Pointer to shader module
 * @return 1 if initialized, 0 otherwise
 */
R_CVULKAN_API int R_CVulkan_ShaderModuleIsInitialized (const struct R_CVulkan_ShaderModule* pShaderModule);
