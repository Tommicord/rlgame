import Rl.Client.Render.Unit.UnitRendererAOTextures;
import Rl.Client.State.UnitState;
import Rl.World.Unit;
import Rl.Base.Binding;
import Rl.Base.Texture2;

import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

void UnitGenerateAOTextures(VkDevice  device,
    Main::MainBinding&                context,
    Providers::UnitStateBinding&      vk,
    const World::UnitTextureMaterial& textures)
{
  if (vk.aoTexturesView[0] == VK_NULL_HANDLE)
  {
    Providers::Texture2Properties aoProperties;
    aoProperties.format = Providers::Texture2Format::R8;
    aoProperties.generateMipmaps = true;
    aoProperties.minFilter = Providers::Texture2Filter::LINEAR_MIPMAP_LINEAR;
    aoProperties.magFilter = Providers::Texture2Filter::LINEAR;

    Providers::Texture2* aoTextures[6] = {
        GenerateLightningTexture(textures.top, aoProperties),
        GenerateLightningTexture(textures.down, aoProperties),
        GenerateLightningTexture(textures.left, aoProperties),
        GenerateLightningTexture(textures.right, aoProperties),
        GenerateLightningTexture(textures.front, aoProperties),
        GenerateLightningTexture(textures.back, aoProperties)};

    // Create Vulkan resources for generated AO textures
    for (int i = 0; i < 6; ++i)
    {
      if (aoTextures[i] && aoTextures[i]->IsLoaded())
      {
        aoTextures[i]->GetImageView(context);
        vk.aoTextures[i] = aoTextures[i]->binding.vkImage;
        vk.aoTexturesMemory[i] = aoTextures[i]->binding.vkImageMemory;
        vk.aoTexturesView[i] = aoTextures[i]->binding.vkImageView;

        // Clear handles from temporary Texture2 to prevent
        // double-deletion
        aoTextures[i]->binding.vkImage = VK_NULL_HANDLE;
        aoTextures[i]->binding.vkImageMemory = VK_NULL_HANDLE;
        aoTextures[i]->binding.vkImageView = VK_NULL_HANDLE;
      }
    }

    // Clean up generated textures
    for (auto& aoTexture : aoTextures)
    {
      if (aoTexture)
      {
        aoTexture->CleanupBinding(context);
        delete aoTexture;
      }
    }
  }
}

void UnitUpdateAOTextureDescriptor(VkDevice device,
    VkDescriptorSet                         descriptorSet,
    VkImageView                             aoTextureView[6],
    VkSampler                               sampler)
{
  VkDescriptorImageInfo aoImageInfo{};
  aoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  aoImageInfo.imageView = aoTextureView[0];
  aoImageInfo.sampler = sampler;

  VkWriteDescriptorSet aoTextureWrite{};
  aoTextureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  aoTextureWrite.dstSet = descriptorSet;
  aoTextureWrite.dstBinding = 10;
  aoTextureWrite.dstArrayElement = 0;
  aoTextureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  aoTextureWrite.descriptorCount = 1;
  aoTextureWrite.pImageInfo = &aoImageInfo;

  vkUpdateDescriptorSets(device, 1, &aoTextureWrite, 0, nullptr);
}

} // namespace Rl::Client::Render
