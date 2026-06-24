#include "rl/Client/Render/Unit/UnitRendererSampler.h"

namespace Rl::Client::Render
{

void UnitCreateGlobalTextureSampler(VkDevice device, VkSampler& sampler)
{
  VkSamplerCreateInfo globalSamplerInfo{};
  globalSamplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  globalSamplerInfo.magFilter               = VK_FILTER_LINEAR;
  globalSamplerInfo.minFilter               = VK_FILTER_LINEAR;
  globalSamplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  globalSamplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  globalSamplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  globalSamplerInfo.anisotropyEnable        = VK_FALSE;
  globalSamplerInfo.maxAnisotropy           = 1.0f;
  globalSamplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  globalSamplerInfo.unnormalizedCoordinates = VK_FALSE;
  globalSamplerInfo.compareEnable           = VK_FALSE;
  globalSamplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
  globalSamplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  globalSamplerInfo.mipLodBias              = 0.0f;
  globalSamplerInfo.minLod                  = 0.0f;
  globalSamplerInfo.maxLod                  = 16.0f;

  if (vkCreateSampler(device, &globalSamplerInfo, nullptr, &sampler) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create global texture sampler");
  }
}

void UnitCreateLightingSampler(VkDevice device, VkSampler& sampler)
{
  VkSamplerCreateInfo lightingSamplerInfo{};
  lightingSamplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  lightingSamplerInfo.magFilter               = VK_FILTER_LINEAR;
  lightingSamplerInfo.minFilter               = VK_FILTER_LINEAR;
  lightingSamplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  lightingSamplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  lightingSamplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  lightingSamplerInfo.anisotropyEnable        = VK_FALSE;
  lightingSamplerInfo.maxAnisotropy           = 1.0f;
  lightingSamplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  lightingSamplerInfo.unnormalizedCoordinates = VK_FALSE;
  lightingSamplerInfo.compareEnable           = VK_FALSE;
  lightingSamplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
  lightingSamplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  lightingSamplerInfo.mipLodBias              = 0.0f;
  lightingSamplerInfo.minLod                  = 0.0f;
  lightingSamplerInfo.maxLod                  = 1.0f;

  if (vkCreateSampler(device, &lightingSamplerInfo, nullptr, &sampler) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create placeholder lighting sampler");
  }
}

void UnitCreateAOSampler(VkDevice device, VkSampler& sampler)
{
  VkSamplerCreateInfo aoSamplerInfo{};
  aoSamplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  aoSamplerInfo.magFilter               = VK_FILTER_LINEAR;
  aoSamplerInfo.minFilter               = VK_FILTER_LINEAR;
  aoSamplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  aoSamplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  aoSamplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  aoSamplerInfo.anisotropyEnable        = VK_FALSE;
  aoSamplerInfo.maxAnisotropy           = 1.0f;
  aoSamplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  aoSamplerInfo.unnormalizedCoordinates = VK_FALSE;
  aoSamplerInfo.compareEnable           = VK_FALSE;
  aoSamplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
  aoSamplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  aoSamplerInfo.mipLodBias              = 0.0f;
  aoSamplerInfo.minLod                  = 0.0f;
  aoSamplerInfo.maxLod                  = 1.0f;

  if (vkCreateSampler(device, &aoSamplerInfo, nullptr, &sampler) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create AO sampler");
  }
}

} // namespace Rl::Client::Render
