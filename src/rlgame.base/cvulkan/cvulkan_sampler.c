#include "rlgame.base/cvulkan/cvulkan_sampler.h"
#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_NewSampler (
    struct R_CVulkan_Sampler*                 pSampler,
    const struct R_CVulkan_SamplerCreateInfo* pCreateInfo)
{
        R_CVULKAN_ASSERT (pSampler);
        R_CVULKAN_ASSERT (pCreateInfo);
        R_CVULKAN_ASSERT (pCreateInfo->pDevice);
#if defined(R_CVULKAN_DEBUG)
        if (!pSampler || !pCreateInfo || !pCreateInfo->pDevice)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }
        if (!R_CVulkan_DeviceIsInitialized (pCreateInfo->pDevice))
        {
                return R_CVULKAN_ERROR_NOT_INITIALIZED;
        }
#endif

        pSampler->device = R_CVulkan_DeviceGetLogicalDevice (pCreateInfo->pDevice);
#if defined(R_CVULKAN_DEBUG)
        pSampler->handle = VK_NULL_HANDLE;
        pSampler->booted = false;
#endif

        VkSamplerCreateInfo samplerInfo = {0};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = pCreateInfo->magFilter;
        samplerInfo.minFilter = pCreateInfo->minFilter;
        samplerInfo.mipmapMode = pCreateInfo->mipmapMode;
        samplerInfo.addressModeU = pCreateInfo->addressModeU;
        samplerInfo.addressModeV = pCreateInfo->addressModeV;
        samplerInfo.addressModeW = pCreateInfo->addressModeW;
        samplerInfo.mipLodBias = pCreateInfo->mipLodBias;
        samplerInfo.anisotropyEnable = pCreateInfo->anisotropyEnable ? VK_TRUE : VK_FALSE;
        samplerInfo.maxAnisotropy = pCreateInfo->maxAnisotropy;
        samplerInfo.minLod = pCreateInfo->minLod;
        samplerInfo.maxLod = pCreateInfo->maxLod;
        samplerInfo.borderColor = pCreateInfo->borderColor;
        samplerInfo.unnormalizedCoordinates = pCreateInfo->unnormalizedCoordinates ? VK_TRUE : VK_FALSE;

        VkResult result = vkCreateSampler (pSampler->device, &samplerInfo, NULL, &pSampler->handle);
        if (result != VK_SUCCESS)
        {
                return R_CVULKAN_ERROR_SAMPLER_CREATE_FAILED;
        }

#if defined(R_CVULKAN_DEBUG)
        pSampler->booted = true;
#endif
        return R_CVULKAN_OK;
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
        pSampler->booted = false;
#endif
}

R_CVULKAN_API VkSampler
R_CVulkan_SamplerGetHandle (const struct R_CVulkan_Sampler* pSampler)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pSampler);
#endif
        return pSampler->handle;
}

R_CVULKAN_API VkDevice
R_CVulkan_SamplerGetDevice (const struct R_CVulkan_Sampler* pSampler)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pSampler);
#endif
        return pSampler->device;
}

R_CVULKAN_API int
R_CVulkan_SamplerIsInitialized (const struct R_CVulkan_Sampler* pSampler)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pSampler);
        return pSampler->booted;
#else
        (void)pSampler;
        return 1;
#endif
}
