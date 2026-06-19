#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vulkan/vulkan.h>
#include "CameraProvider.h"
#include "rl/Base/Game.h"
#include "rl/Base/ShaderFactory.h"

namespace Rl::Providers {

void CameraStateDrawable::OnCreate(CameraStateResource& resource, CameraStateDrawableVulkan& vk, Game::VulkanContext& context)
{
    
}

void CameraStateDrawable::OnDestroy(CameraStateResource& resource, CameraStateDrawableVulkan& vk, Game::VulkanContext& context)
{
    // No resources to destroy for push constants
}

void CameraStateDrawable::OnPause()
{
    // Pause camera updates
}

void CameraStateDrawable::OnResume()
{
    // Resume camera updates
}

void CameraStateDrawable::OnUpdate(CameraStateResource& resource, CameraStateDrawableVulkan& vk, Game::VulkanContext& context)
{
    glm::mat4 model = resource.cam->GetModelMatrix();
    glm::mat4 view = resource.cam->GetViewMatrix();
    glm::mat4 projection = resource.cam->GetProjectionMatrix();
    glm::mat4 matrices[3] = {model, view, projection};
    vkCmdPushConstants(context.commandBuffers[0], context.pipelineLayout,
                        VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(matrices), matrices);
}

void CameraStateDrawable::OnDraw(const CameraStateResource& resource, CameraStateDrawableVulkan& vk, Game::VulkanContext& context)
{
}

} // namespace Rl::Providers
