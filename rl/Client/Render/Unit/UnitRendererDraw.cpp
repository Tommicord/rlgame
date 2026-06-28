import Rl.Client.Render.Unit.UnitRendererDraw;
import Rl.Client.Render.Unit.UnitRendererShadowMap;
import Rl.Client.Render.Unit.UnitRendererVertices;
import Rl.Client.State.UnitState;
import Rl.Player.PlayerCamera;
import Rl.Base.Binding;

import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

static glm::mat4 CalculateLightSpaceMatrix(
    const glm::vec3& sunDirection, const glm::vec3& targetPosition)
{
  float     orthoSize = 20.0f;
  glm::mat4 lightProjection =
      glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 50.0f);
  glm::vec3 shadowCameraPos = targetPosition + (sunDirection * 30.0f);
  glm::mat4 lightView =
      glm::lookAt(shadowCameraPos, targetPosition, glm::vec3(0.0f, 1.0f, 0.0));
  return lightProjection * lightView;
}

void UnitRenderShadowMap(Providers::UnitStateResource& resource,
    Providers::UnitStateBinding&                       vk,
    Main::MainBinding&                                 context)
{
  if (vk.shadowPipeline == VK_NULL_HANDLE || vk.shadowPipelineLayout == VK_NULL_HANDLE ||
      vk.shadowMapRenderPass == VK_NULL_HANDLE ||
      vk.shadowMapFramebuffer == VK_NULL_HANDLE)
    return;

  const Player::IPlayerCamera& cam = *resource.player.camera;

  // Calculate light space matrix
  glm::vec3                  sunDirection = glm::normalize(glm::vec3(0.5f, 0.8f, 0.6f));
  Player::IPlayerCamera::Eye eyePos = cam.eye;
  glm::vec3                  cameraPosition = glm::vec3(eyePos.x, eyePos.y, eyePos.z);
  glm::mat4 lightSpaceMatrix = CalculateLightSpaceMatrix(sunDirection, cameraPosition);

  // Transition shadow map image to depth attachment layout
  VkImageMemoryBarrier shadowBarrier{};
  shadowBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  shadowBarrier.oldLayout = vk.shadowMapInitialized
                                ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                : VK_IMAGE_LAYOUT_UNDEFINED;
  shadowBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  shadowBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  shadowBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  shadowBarrier.image = vk.shadowMapImage;
  shadowBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  shadowBarrier.subresourceRange.baseMipLevel = 0;
  shadowBarrier.subresourceRange.levelCount = 1;
  shadowBarrier.subresourceRange.baseArrayLayer = 0;
  shadowBarrier.subresourceRange.layerCount = 1;
  shadowBarrier.srcAccessMask = vk.shadowMapInitialized ? VK_ACCESS_SHADER_READ_BIT : 0;
  shadowBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  vkCmdPipelineBarrier(context.commandBuffers[0],
      vk.shadowMapInitialized ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                              : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
      0, 0, nullptr, 0, nullptr, 1, &shadowBarrier);

  // Mark as initialized after first transition
  vk.shadowMapInitialized = true;

  // Begin shadow map render pass
  UnitBeginShadowMapRenderPass(context.commandBuffers[0], vk.shadowMapRenderPass,
      vk.shadowMapFramebuffer, 1024, 1024);

  // Bind shadow pipeline
  vkCmdBindPipeline(
      context.commandBuffers[0], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.shadowPipeline);

  // Bind vertex buffer
  VkBuffer     vertexBuffers[] = {vk.vertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(context.commandBuffers[0], 0, 1, vertexBuffers, offsets);

  // Bind index buffer
  vkCmdBindIndexBuffer(
      context.commandBuffers[0], vk.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

  // Push light space matrix
  vkCmdPushConstants(context.commandBuffers[0], vk.shadowPipelineLayout,
      VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &lightSpaceMatrix);

  // Draw geometry
  vkCmdDrawIndexed(context.commandBuffers[0], 36, 1, 0, 0, 0);

  // End shadow map render pass
  UnitEndShadowMapRenderPass(context.commandBuffers[0]);

  // Transition shadow map image to shader read layout
  shadowBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  shadowBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  shadowBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  shadowBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(context.commandBuffers[0],
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
      &shadowBarrier);
}

void UnitRender(Providers::UnitStateResource& resource,
    Providers::UnitStateBinding&              vk,
    Main::MainBinding&                        context)
{
  const auto&                  vertices = UnitGetTestVertices();
  const Player::IPlayerCamera& cam = *resource.player.camera;
  glm::mat4                    model = cam.GetModelMatrix();
  glm::mat4                    view = cam.GetViewMatrix();
  glm::mat4                    projection = cam.GetProjectionMatrix();
  glm::mat4                    matrices[3] = {model, view, projection};

  // Check if any face has curvature
  bool hasCurvature = false;
  for (const auto& vertex : vertices)
  {
    if (vertex.polCurve.x != 0.0f || vertex.polCurve.y != 0.0f)
    {
      hasCurvature = true;
      break;
    }
  }

  // Bind graphics pipeline
  if (vk.pipeline != VK_NULL_HANDLE && vk.pipelineLayout != VK_NULL_HANDLE)
  {
    vkCmdBindPipeline(
        context.commandBuffers[0], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline);

    if (hasCurvature && vk.curvedVertexBuffer != VK_NULL_HANDLE &&
        vk.curvedIndexBuffer != VK_NULL_HANDLE)
    {
      // Use curved vertex buffer and curved index buffer
      VkBuffer     vertexBuffers[] = {vk.curvedVertexBuffer};
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(context.commandBuffers[0], 0, 1, vertexBuffers, offsets);
      vkCmdBindIndexBuffer(
          context.commandBuffers[0], vk.curvedIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

      // Bind graphics descriptor set for textures
      vkCmdBindDescriptorSets(context.commandBuffers[0], VK_PIPELINE_BIND_POINT_GRAPHICS,
          vk.pipelineLayout, 0, 1, &vk.descriptorSet, 0, nullptr);
      // Push camera matrices for vertex shader
      vkCmdPushConstants(context.commandBuffers[0], vk.pipelineLayout,
          VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(matrices), matrices);
      // Draw using indirect draw parameters from curvature compute shader
      vkCmdDrawIndexedIndirect(context.commandBuffers[0], vk.curveIndirectDrawBuffer, 0,
          1, sizeof(VkDrawIndexedIndirectCommand));
    }
    else
    {
      // Use original vertex buffer (all vertices)
      VkBuffer     vertexBuffers[] = {vk.vertexBuffer};
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(context.commandBuffers[0], 0, 1, vertexBuffers, offsets);

      // Bind output index buffer (culled indices from compute shader)
      vkCmdBindIndexBuffer(
          context.commandBuffers[0], vk.outputIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

      // Bind graphics descriptor set for textures
      vkCmdBindDescriptorSets(context.commandBuffers[0], VK_PIPELINE_BIND_POINT_GRAPHICS,
          vk.pipelineLayout, 0, 1, &vk.descriptorSet, 0, nullptr);
      // Push camera matrices for vertex shader
      vkCmdPushConstants(context.commandBuffers[0], vk.pipelineLayout,
          VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(matrices), matrices);
      // Draw using indexed indirect draw parameters from compute shader
      vkCmdDrawIndexedIndirect(context.commandBuffers[0], vk.indirectDrawBuffer, 0, 1,
          sizeof(VkDrawIndexedIndirectCommand));
    }
  }
}

} // namespace Rl::Client::Render
