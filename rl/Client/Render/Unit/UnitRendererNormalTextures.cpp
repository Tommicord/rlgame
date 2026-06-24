#include "rl/Client/Render/Unit/UnitRendererNormalTextures.h"
#include "rl/Base/Texture2.h"

namespace Rl::Client::Render
{

void UnitGenerateNormalTextures(VkDevice device,
    Game::VulkanContext&                 context,
    Providers::UnitStateDrawableVulkan&  vk,
    const World::UnitTextureMaterial&    textures)
{
  if (vk.normalTexturesView[0] == VK_NULL_HANDLE)
  {
    Providers::TextureProperties normalProperties;
    normalProperties.format          = Providers::TextureFormat::RGBA8;
    normalProperties.generateMipmaps = true;
    normalProperties.minFilter       = Providers::TextureFilter::LINEAR_MIPMAP_LINEAR;
    normalProperties.magFilter       = Providers::TextureFilter::LINEAR;
    normalProperties.sRGB            = false;

    Providers::Texture2* normalTextures[6] = {GenerateNormalTexture(textures.top, normalProperties),
        GenerateNormalTexture(textures.down, normalProperties),
        GenerateNormalTexture(textures.left, normalProperties),
        GenerateNormalTexture(textures.right, normalProperties),
        GenerateNormalTexture(textures.front, normalProperties),
        GenerateNormalTexture(textures.back, normalProperties)};

    // Create Vulkan resources for generated normal textures
    for (int i = 0; i < 6; ++i)
    {
      if (normalTextures[i] && normalTextures[i]->IsLoaded())
      {
        normalTextures[i]->GetImageView(context);
        vk.normalTextures[i]       = normalTextures[i]->binding.vkImage;
        vk.normalTexturesMemory[i] = normalTextures[i]->binding.vkImageMemory;
        vk.normalTexturesView[i]   = normalTextures[i]->binding.vkImageView;

        // Clear handles from temporary Texture2 to prevent double-deletion
        normalTextures[i]->binding.vkImage       = VK_NULL_HANDLE;
        normalTextures[i]->binding.vkImageMemory = VK_NULL_HANDLE;
        normalTextures[i]->binding.vkImageView   = VK_NULL_HANDLE;
      }
    }

    // Clean up generated textures
    for (int i = 0; i < 6; ++i)
    {
      if (normalTextures[i])
      {
        normalTextures[i]->CleanupVulkan(context);
        delete normalTextures[i];
      }
    }
  }
}

void UnitUpdateNormalTextureDescriptor(VkDevice device,
    VkDescriptorSet                             descriptorSet,
    VkImageView                                 normalTextureView,
    VkSampler                                   sampler)
{
  VkDescriptorImageInfo normalImageInfo{};
  normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  normalImageInfo.imageView   = normalTextureView;
  normalImageInfo.sampler     = sampler;

  VkWriteDescriptorSet normalTextureWrite{};
  normalTextureWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  normalTextureWrite.dstSet          = descriptorSet;
  normalTextureWrite.dstBinding      = 11;
  normalTextureWrite.dstArrayElement = 0;
  normalTextureWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  normalTextureWrite.descriptorCount = 1;
  normalTextureWrite.pImageInfo      = &normalImageInfo;

  vkUpdateDescriptorSets(device, 1, &normalTextureWrite, 0, nullptr);
}

} // namespace Rl::Client::Render
