#include "rl/Client/Render/Unit/UnitRendererPlaceholderResource.h"
#include "rl/Client/Render/Unit/UnitRendererBasicBuffer.h"
#include "rl/Client/Render/Unit/UnitRendererInfo.h"

#include <stdexcept>

namespace Rl::Client::Render
{

void UnitCreatePlaceholderLightingTexture(VkDevice device,
    VkPhysicalDevice                               physicalDevice,
    VkImage&                                       texture,
    VkDeviceMemory&                                textureMemory,
    VkImageView&                                   textureView,
    VkSampler&                                     sampler)
{
  // Create a 1x1 white texture as placeholder
  VkImageCreateInfo imageInfo{};
  imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width  = 1;
  imageInfo.extent.height = 1;
  imageInfo.extent.depth  = 1;
  imageInfo.mipLevels     = 1;
  imageInfo.arrayLayers   = 1;
  imageInfo.format        = VK_FORMAT_R8G8B8A8_UNORM;
  imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

  if (vkCreateImage(device, &imageInfo, nullptr, &texture) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create placeholder lighting texture");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, texture, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize  = memRequirements.size;
  allocInfo.memoryTypeIndex = 0; // TODO: Find appropriate memory type

  if (vkAllocateMemory(device, &allocInfo, nullptr, &textureMemory) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate memory for placeholder lighting texture");
  }

  vkBindImageMemory(device, texture, textureMemory, 0);

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image                           = texture;
  viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format                          = VK_FORMAT_R8G8B8A8_UNORM;
  viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = 1;

  if (vkCreateImageView(device, &viewInfo, nullptr, &textureView) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create placeholder lighting texture view");
  }

  // Create sampler
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter               = VK_FILTER_LINEAR;
  samplerInfo.minFilter               = VK_FILTER_LINEAR;
  samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable        = VK_FALSE;
  samplerInfo.maxAnisotropy           = 1.0f;
  samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable           = VK_FALSE;
  samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias              = 0.0f;
  samplerInfo.minLod                  = 0.0f;
  samplerInfo.maxLod                  = 1.0f;

  if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create placeholder lighting sampler");
  }
}

void UnitCreatePlaceholderSettingsBuffer(VkDevice device,
    VkPhysicalDevice                              physicalDevice,
    VkBuffer&                                     buffer,
    VkDeviceMemory&                               bufferMemory)
{
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size        = 256; // Placeholder size
  bufferInfo.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create placeholder settings buffer");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize  = memRequirements.size;
  allocInfo.memoryTypeIndex = 0; // TODO: Find appropriate memory type

  if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate memory for placeholder settings buffer");
  }

  vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void UnitCreatePlaceholderLightingBuffer(VkDevice device,
    VkPhysicalDevice                              physicalDevice,
    VkBuffer&                                     buffer,
    VkDeviceMemory&                               bufferMemory)
{
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size        = sizeof(UnitRenderLightingUniforms);
  bufferInfo.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create placeholder lighting buffer");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      UnitFindMemoryTypeIndex(physicalDevice, memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate memory for placeholder lighting buffer");
  }

  vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void UnitCreatePlaceholderAOTexture(VkDevice device,
    VkPhysicalDevice                         physicalDevice,
    VkImage&                                 texture,
    VkDeviceMemory&                          textureMemory,
    VkImageView&                             textureView,
    VkSampler&                               sampler)
{
  // Create a 1x1 white texture as placeholder for AO
  VkImageCreateInfo imageInfo{};
  imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width  = 1;
  imageInfo.extent.height = 1;
  imageInfo.extent.depth  = 1;
  imageInfo.mipLevels     = 1;
  imageInfo.arrayLayers   = 1;
  imageInfo.format        = VK_FORMAT_R8_UNORM;
  imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

  if (vkCreateImage(device, &imageInfo, nullptr, &texture) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create placeholder AO texture");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, texture, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      UnitFindMemoryTypeIndex(physicalDevice, memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &textureMemory) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate memory for placeholder AO texture");
  }

  vkBindImageMemory(device, texture, textureMemory, 0);

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image                           = texture;
  viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format                          = VK_FORMAT_R8_UNORM;
  viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = 1;

  if (vkCreateImageView(device, &viewInfo, nullptr, &textureView) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create placeholder AO texture view");
  }

  // Create sampler
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter               = VK_FILTER_LINEAR;
  samplerInfo.minFilter               = VK_FILTER_LINEAR;
  samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable        = VK_FALSE;
  samplerInfo.maxAnisotropy           = 1.0f;
  samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable           = VK_FALSE;
  samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias              = 0.0f;
  samplerInfo.minLod                  = 0.0f;
  samplerInfo.maxLod                  = 1.0f;

  if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create placeholder AO sampler");
  }
}

void UnitCreateTriplanarSettingsBuffer(VkDevice device,
    VkPhysicalDevice                            physicalDevice,
    VkBuffer&                                   buffer,
    VkDeviceMemory&                             bufferMemory)
{
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size        = sizeof(UnitRenderTriplanarSettings);
  bufferInfo.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create triplanar settings buffer");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize  = memRequirements.size;
  allocInfo.memoryTypeIndex = 0; // TODO: Find appropriate memory type

  if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate memory for triplanar settings buffer");
  }

  vkBindBufferMemory(device, buffer, bufferMemory, 0);

  // Initialize triplanar settings with default values
  UnitRenderTriplanarSettings settings{};
  settings.scale     = 1.0f;
  settings.sharpness = 2.0f;
  settings.offsetX   = 0.0f;
  settings.offsetY   = 0.0f;
  settings.offsetZ   = 0.0f;
  settings.blendMix  = 1.0f;

  void* data;
  vkMapMemory(device, bufferMemory, 0, sizeof(settings), 0, &data);
  memcpy(data, &settings, sizeof(settings));
  vkUnmapMemory(device, bufferMemory);
}

void UnitCreatePlaceholderNormalTexture(VkDevice device,
    VkPhysicalDevice                             physicalDevice,
    VkImage&                                     texture,
    VkDeviceMemory&                              textureMemory,
    VkImageView&                                 textureView,
    VkSampler&                                   sampler)
{
  // Create a 1x1 white texture as placeholder for normal map
  VkImageCreateInfo imageInfo{};
  imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width  = 1;
  imageInfo.extent.height = 1;
  imageInfo.extent.depth  = 1;
  imageInfo.mipLevels     = 1;
  imageInfo.arrayLayers   = 1;
  imageInfo.format        = VK_FORMAT_R8G8B8A8_UNORM;
  imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

  if (vkCreateImage(device, &imageInfo, nullptr, &texture) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create placeholder normal texture");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, texture, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize  = memRequirements.size;
  allocInfo.memoryTypeIndex = 0;

  if (vkAllocateMemory(device, &allocInfo, nullptr, &textureMemory) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate memory for placeholder normal texture");
  }

  vkBindImageMemory(device, texture, textureMemory, 0);

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image                           = texture;
  viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format                          = VK_FORMAT_R8G8B8A8_UNORM;
  viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = 1;

  if (vkCreateImageView(device, &viewInfo, nullptr, &textureView) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create placeholder normal texture view");
  }

  // Create sampler
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter               = VK_FILTER_LINEAR;
  samplerInfo.minFilter               = VK_FILTER_LINEAR;
  samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable        = VK_FALSE;
  samplerInfo.maxAnisotropy           = 1.0f;
  samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable           = VK_FALSE;
  samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias              = 0.0f;
  samplerInfo.minLod                  = 0.0f;
  samplerInfo.maxLod                  = 1.0f;

  if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create placeholder normal sampler");
  }
}

} // namespace Rl::Client::Render
