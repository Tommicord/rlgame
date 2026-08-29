#include "rlgame.base/cvulkan/cvulkan_shader_module.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cstl/cstl_log.h"

#include <string.h>
#include <stdlib.h>

/**
 * @brief SPIR-V magic number (0x07230203 in little-endian)
 * The first word of a valid SPIR-V binary
 */
#define R_CVULKAN_SPIRV_MAGIC_NUMBER 0x07230203

/**
 * @brief Minimum valid SPIR-V code size (at least the header)
 */
#define R_CVULKAN_SPIRV_MIN_CODE_SIZE (sizeof (uint32_t) * 5)

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_NewShaderModule (
    struct R_CVulkan_ShaderModule* pShaderModule,
    const struct R_CVulkan_Device* pDevice,
    const uint32_t*                pCode,
    size_t                         codeSize)
{
    R_CVULKAN_ASSERT (pShaderModule);
    R_CVULKAN_ASSERT (pDevice);
    R_CVULKAN_ASSERT (pCode);

#if defined(R_CVULKAN_DEBUG)
    if (!pShaderModule || !pDevice || !pCode)
    {
        R_CSTL_LOG_ERROR ("R_CVulkan_NewShaderModule: NULL pointer detected");
        R_CSTL_LOG_ERROR ("  pShaderModule: %p", (void*)pShaderModule);
        R_CSTL_LOG_ERROR ("  pDevice: %p", (void*)pDevice);
        R_CSTL_LOG_ERROR ("  pCode: %p", (void*)pCode);
        return R_CVULKAN_ERROR_NULL_POINTER;
    }

    if (!R_CVulkan_DeviceIsInitialized (pDevice))
    {
        R_CSTL_LOG_ERROR ("R_CVulkan_NewShaderModule: Device not initialized");
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }
    if (codeSize < R_CVULKAN_SPIRV_MIN_CODE_SIZE)
    {
        R_CSTL_LOG_ERROR ("R_CVulkan_NewShaderModule: Invalid SPIR-V code size");
        R_CSTL_LOG_ERROR ("  Provided codeSize: %zu bytes", codeSize);
        R_CSTL_LOG_ERROR ("  Minimum required: %zu bytes (5 uint32_t words)", R_CVULKAN_SPIRV_MIN_CODE_SIZE);
        R_CSTL_LOG_ERROR ("  This suggests the shader data may be empty or corrupted");
        return R_CVULKAN_ERROR_INVALID_ARGUMENT;
    }
    uint32_t magicNumber = *(const uint32_t*)pCode;
    if (magicNumber != R_CVULKAN_SPIRV_MAGIC_NUMBER)
    {
        R_CSTL_LOG_ERROR ("R_CVulkan_NewShaderModule: Invalid SPIR-V magic number");
        R_CSTL_LOG_ERROR ("  Expected magic number: 0x%08X", R_CVULKAN_SPIRV_MAGIC_NUMBER);
        R_CSTL_LOG_ERROR ("  Found magic number: 0x%08X", magicNumber);
        R_CSTL_LOG_ERROR ("  The data is not valid SPIR-V bytecode");
        return R_CVULKAN_ERROR_INVALID_ARGUMENT;
    }
    if ((codeSize & 3) != 0)
    {
        R_CSTL_LOG_ERROR ("R_CVulkan_NewShaderModule: Invalid SPIR-V code size alignment");
        R_CSTL_LOG_ERROR ("  Provided codeSize: %zu bytes", codeSize);
        R_CSTL_LOG_ERROR ("  SPIR-V code size must be a multiple of 4 bytes");
        R_CSTL_LOG_ERROR ("  This suggests the shader data may be corrupted or improperly embedded");
        return R_CVULKAN_ERROR_INVALID_ARGUMENT;
    }
    pShaderModule->handle = VK_NULL_HANDLE;
    
#endif
    pShaderModule->device = R_CVulkan_DeviceGetLogicalDevice (pDevice);
    pShaderModule->codeSize = codeSize;

    VkShaderModuleCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = codeSize;
    createInfo.pCode = pCode;

    VkResult result = vkCreateShaderModule (pShaderModule->device, &createInfo, NULL, &pShaderModule->handle);
    if (result != VK_SUCCESS)
    {
        R_CSTL_LOG_ERROR ("R_CVulkan_NewShaderModule: vkCreateShaderModule failed");
        R_CSTL_LOG_ERROR ("  Vulkan result code: %d", result);
        R_CSTL_LOG_ERROR ("  Shader code size: %zu bytes", codeSize);
        return R_CVULKAN_ERROR_SHADER_MODULE_CREATE_FAILED;
    }
#if defined(R_CVULKAN_DEBUG)
    
    R_CSTL_LOG_INFO ("R_CVulkan_NewShaderModule: Shader module created");
    R_CSTL_LOG_INFO ("  Code size: %zu bytes", codeSize);
    R_CSTL_LOG_INFO ("  Handle: %p", (void*)pShaderModule->handle);
#endif
    return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_DeleteShaderModule (struct R_CVulkan_ShaderModule* pShaderModule)
{
    R_CVULKAN_ASSERT (pShaderModule);
#if defined(R_CVULKAN_DEBUG)
    if (!pShaderModule)
    {
        R_CSTL_LOG_ERROR ("R_CVulkan_DeleteShaderModule: NULL pointer detected");
        return;
    }
    if (pShaderModule->handle == VK_NULL_HANDLE)
    {
        R_CSTL_LOG_WARN (
            "R_CVulkan_DeleteShaderModule: Shader module already destroyed or never initialized");
        return;
    }
#endif
    if (pShaderModule->handle != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule (pShaderModule->device, pShaderModule->handle, NULL);
    }
#if defined(R_CVULKAN_DEBUG)
    pShaderModule->handle = VK_NULL_HANDLE;
    pShaderModule->device = VK_NULL_HANDLE;
    pShaderModule->codeSize = 0;
    
#endif
}

R_CVULKAN_API VkShaderModule
R_CVulkan_ShaderModuleGetHandle (const struct R_CVulkan_ShaderModule* pShaderModule)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pShaderModule);
#endif
    return pShaderModule->handle;
}

R_CVULKAN_API VkDevice
R_CVulkan_ShaderModuleGetDevice (const struct R_CVulkan_ShaderModule* pShaderModule)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pShaderModule);
#endif
    return pShaderModule->device;
}

R_CVULKAN_API size_t
R_CVulkan_ShaderModuleGetCodeSize (const struct R_CVulkan_ShaderModule* pShaderModule)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pShaderModule);
#endif
    return pShaderModule->codeSize;
}

R_CVULKAN_API int
R_CVulkan_ShaderModuleIsInitialized (const struct R_CVulkan_ShaderModule* pShaderModule)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pShaderModule);
    return 1;
#else
    return true;
#endif
}
