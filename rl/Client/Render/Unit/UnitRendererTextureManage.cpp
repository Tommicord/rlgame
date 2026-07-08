import Rl.Client.Render.Unit.UnitRendererTextureManage;
import Rl.Base.Binding;
import Rl.Base.Texture2;
import Rl.World.Unit;

import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

void UnitUpdateUnitTextures(VkDevice device,
    VkDescriptorSet                  descriptorSet,
    World::UnitTextureMaterial&      textures,
    Main::MainBinding&               context)
{
  VkDescriptorImageInfo imageInfos[6]{};
  auto                  genSampler = [&imageInfos, &context](Providers::Texture2* texture, const int index)
  {
    if (texture)
    {
      imageInfos[index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      texture->GetImageView(context);
      imageInfos[index].imageView = texture->binding.vkImageView;
      texture->GetSampler(context);
      imageInfos[index].sampler = texture->binding.vkSampler;
    }
    else
    {
      imageInfos[index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      imageInfos[index].imageView = VK_NULL_HANDLE;
      imageInfos[index].sampler = VK_NULL_HANDLE;
    }
  };
  // Get texture image views and samplers (order: top, down, left, right, front, back)
  const auto& faces = textures.GetFaces();
  genSampler(faces.top, 0);
  genSampler(faces.down, 1);
  genSampler(faces.left, 2);
  genSampler(faces.right, 3);
  genSampler(faces.front, 4);
  genSampler(faces.back, 5);

  VkWriteDescriptorSet textureWrite{};
  textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  textureWrite.dstSet = descriptorSet;
  textureWrite.dstBinding = 2;
  textureWrite.dstArrayElement = 0;
  textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  textureWrite.descriptorCount = 6;
  textureWrite.pImageInfo = imageInfos;
  vkUpdateDescriptorSets(device, 1, &textureWrite, 0, nullptr);
}

} // namespace Rl::Client::Render
