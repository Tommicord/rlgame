import Rl.Base.Game;
import Rl.Client.State.CameraState;

import <glm/glm.hpp>;
import <glm/gtc/type_ptr.hpp>;
import <iostream>;
import <vulkan/vulkan.h>;

namespace Rl::Providers
{

void CameraStateDrawable::OnDraw(
    CameraStateResource& resource, CameraStateDrawableVulkan& vk, Game::VulkanContext& context)
{
  if (context.pipelineLayout == VK_NULL_HANDLE)
  {
    return;
  }
  glm::mat4 model       = resource.cam->GetModelMatrix();
  glm::mat4 view        = resource.cam->GetViewMatrix();
  glm::mat4 projection  = resource.cam->GetProjectionMatrix();
  glm::mat4 matrices[3] = {model, view, projection};
  vkCmdPushConstants(context.commandBuffers[0], context.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
      0, sizeof(matrices), matrices);
}
void CameraStateDrawable::OnDrawCompute(
    CameraStateResource& resource, CameraStateDrawableVulkan& vk, Game::VulkanContext& context)
{
}
void CameraStateDrawable::OnUpdate(
    CameraStateResource& resource, CameraStateDrawableVulkan& vk, Game::VulkanContext& context)
{
}
void CameraStateDrawable::OnCreate(
    CameraStateResource& resource, CameraStateDrawableVulkan& vk, Game::VulkanContext& context)
{
}
void CameraStateDrawable::OnDestroy(
    CameraStateResource& resource, CameraStateDrawableVulkan& vk, Game::VulkanContext& context)
{
}

} // namespace Rl::Providers
