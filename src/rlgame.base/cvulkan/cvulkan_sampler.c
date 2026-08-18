#include "rlgame.base/cvulkan/cvulkan_sampler.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_NewSampler (
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
    int                            unnormalizedCoordinates)
{
        R_CVULKAN_ASSERT (pSampler);
        R_CVULKAN_ASSERT (pDevice);
#if defined(R_CVULKAN_DEBUG)
        if (!pSampler || !pDevice)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }
        if (!R_CVulkan_DeviceIsInitialized (pDevice))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        pSampler->device = R_CVulkan_DeviceGetHandle (pDevice);
#if defined(R_CVULKAN_DEBUG)
        pSampler->handle = VK_NULL_HANDLE;
        pSampler->isInitialized = false;
#endif

        VkSamplerCreateInfo samplerInfo = {0};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = magFilter;
        samplerInfo.minFilter = minFilter;
        samplerInfo.mipmapMode = mipmapMode;
        samplerInfo.addressModeU = addressModeU;
        samplerInfo.addressModeV = addressModeV;
        samplerInfo.addressModeW = addressModeW;
        samplerInfo.mipLodBias = mipLodBias;
        samplerInfo.anisotropyEnable = anisotropyEnable ? VK_TRUE : VK_FALSE;
        samplerInfo.maxAnisotropy = maxAnisotropy;
        samplerInfo.minLod = minLod;
        samplerInfo.maxLod = maxLod;
        samplerInfo.borderColor = borderColor;
        samplerInfo.unnormalizedCoordinates = unnormalizedCoordinates ? VK_TRUE : VK_FALSE;

        VkResult result = vkCreateSampler (pSampler->device, &samplerInfo, NULL, &pSampler->handle);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_SAMPLER_CREATE_FAILED;
        }

#if defined(R_CVULKAN_DEBUG)
        pSampler->isInitialized = true;
#endif
        return R_CVULKAN_ERROR_OK;
}

R_CVULKAN_API void
R_CVulkan_DeleteSampler (struct R_CVulkan_Sampler* pSampler)
{
        R_CVULKAN_ASSERT (pSampler);

#if defined(R_CVULKAN_DEBUG)
        if (!pSampler)
        {
                return;
        }
#endif

        if (pSampler->handle != VK_NULL_HANDLE)
        {
                vkDestroySampler (pSampler->device, pSampler->handle, NULL);
                pSampler->handle = VK_NULL_HANDLE;
        }
#if defined(R_CVULKAN_DEBUG)
        pSampler->device = VK_NULL_HANDLE;
        pSampler->isInitialized = false;
#endif
}

R_CVULKAN_API VkSampler
R_CVulkan_SamplerGetHandle (const struct R_CVulkan_Sampler* pSampler)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pSampler != NULL);
#endif
        return pSampler->handle;
}

R_CVULKAN_API VkDevice
R_CVulkan_SamplerGetDevice (const struct R_CVulkan_Sampler* pSampler)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pSampler != NULL);
#endif
        return pSampler->device;
}

R_CVULKAN_API int
R_CVulkan_SamplerIsInitialized (const struct R_CVulkan_Sampler* pSampler)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pSampler != NULL);
        return pSampler->isInitialized;
#else
        (void)pSampler;
        return 1;
#endif
}
