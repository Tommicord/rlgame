#include "rl/Client/Render/Unit/UnitRendererDraw.h"
#include <glm/glm.hpp>
#include "rl/Client/Render/Unit/UnitRendererVertices.h"

namespace Rl::Client::Render
{

void UnitRender(Providers::UnitStateResource& resource,
    Providers::UnitStateDrawableVulkan&       vk,
    Game::VulkanContext&                      context)
{
  if (!resource.cameraModel)
    return;

  const auto&          vertices    = UnitGetTestVertices();
  const World::Camera& cam         = resource.cameraModel->GetObject();
  glm::mat4            model       = cam.GetModelMatrix();
  glm::mat4            view        = cam.GetViewMatrix();
  glm::mat4            projection  = cam.GetProjectionMatrix();
  glm::mat4            matrices[3] = {model, view, projection};

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
    vkCmdBindPipeline(context.commandBuffers[0], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline);

    if (hasCurvature && vk.curvedVertexBuffer != VK_NULL_HANDLE &&
        vk.curvedIndexBuffer != VK_NULL_HANDLE)
    {
      // Use curved vertex buffer and curved index buffer
      VkBuffer     vertexBuffers[] = {vk.curvedVertexBuffer};
      VkDeviceSize offsets[]       = {0};
      vkCmdBindVertexBuffers(context.commandBuffers[0], 0, 1, vertexBuffers, offsets);
      vkCmdBindIndexBuffer(
          context.commandBuffers[0], vk.curvedIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

      // Bind graphics descriptor set for textures
      vkCmdBindDescriptorSets(context.commandBuffers[0], VK_PIPELINE_BIND_POINT_GRAPHICS,
          vk.pipelineLayout, 0, 1, &vk.descriptorSet, 0, nullptr);
      // Push camera matrices for vertex shader
      vkCmdPushConstants(context.commandBuffers[0], vk.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
          0, sizeof(matrices), matrices);
      // Draw using indirect draw parameters from curvature compute shader
      vkCmdDrawIndexedIndirect(context.commandBuffers[0], vk.curveIndirectDrawBuffer, 0, 1,
          sizeof(VkDrawIndexedIndirectCommand));
    }
    else
    {
      // Use original vertex buffer (all vertices)
      VkBuffer     vertexBuffers[] = {vk.vertexBuffer};
      VkDeviceSize offsets[]       = {0};
      vkCmdBindVertexBuffers(context.commandBuffers[0], 0, 1, vertexBuffers, offsets);

      // Bind output index buffer (culled indices from compute shader)
      vkCmdBindIndexBuffer(
          context.commandBuffers[0], vk.outputIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

      // Bind graphics descriptor set for textures
      vkCmdBindDescriptorSets(context.commandBuffers[0], VK_PIPELINE_BIND_POINT_GRAPHICS,
          vk.pipelineLayout, 0, 1, &vk.descriptorSet, 0, nullptr);
      // Push camera matrices for vertex shader
      vkCmdPushConstants(context.commandBuffers[0], vk.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
          0, sizeof(matrices), matrices);
      // Draw using indexed indirect draw parameters from compute shader
      vkCmdDrawIndexedIndirect(context.commandBuffers[0], vk.indirectDrawBuffer, 0, 1,
          sizeof(VkDrawIndexedIndirectCommand));
    }
  }
}

} // namespace Rl::Client::Render
