#include "rlgame.base/cvulkan/cvulkan_shader_module.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

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
                return R_CVULKAN_ERROR_NULL_POINTER;
        }

        if (!R_CVulkan_DeviceIsInitialized (pDevice))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
        if (codeSize < R_CVULKAN_SPIRV_MIN_CODE_SIZE)
        {
                return R_CVULKAN_ERROR_INVALID_ARGUMENT;
        }
        if (*(const uint32_t*)pCode != R_CVULKAN_SPIRV_MAGIC_NUMBER)
        {
                return R_CVULKAN_ERROR_INVALID_ARGUMENT;
        }
        if ((codeSize & 3) != 0)
        {
                return R_CVULKAN_ERROR_INVALID_ARGUMENT;
        }
        pShaderModule->handle = VK_NULL_HANDLE;
        pShaderModule->booted = false;
#endif
        pShaderModule->device = R_CVulkan_DeviceGetLogicalDevice (pDevice);
        pShaderModule->codeSize = codeSize;

        VkShaderModuleCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = codeSize;
        createInfo.pCode = pCode;

        VkResult result
            = vkCreateShaderModule (pShaderModule->device, &createInfo, NULL, &pShaderModule->handle);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_SHADER_MODULE_CREATE_FAILED;
        }
#if defined(R_CVULKAN_DEBUG)
        pShaderModule->booted = true;
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
        pShaderModule->booted = false;
#endif
}

R_CVULKAN_API VkShaderModule
R_CVulkan_ShaderModuleGetHandle (const struct R_CVulkan_ShaderModule* pShaderModule)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pShaderModule != NULL);
#endif
        return pShaderModule->handle;
}

R_CVULKAN_API VkDevice
R_CVulkan_ShaderModuleGetDevice (const struct R_CVulkan_ShaderModule* pShaderModule)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pShaderModule != NULL);
#endif
        return pShaderModule->device;
}

R_CVULKAN_API size_t
R_CVulkan_ShaderModuleGetCodeSize (const struct R_CVulkan_ShaderModule* pShaderModule)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pShaderModule != NULL);
#endif
        return pShaderModule->codeSize;
}

R_CVULKAN_API int
R_CVulkan_ShaderModuleIsInitialized (const struct R_CVulkan_ShaderModule* pShaderModule)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pShaderModule != NULL);
        return pShaderModule->booted;
#else
        return true;
#endif
}
