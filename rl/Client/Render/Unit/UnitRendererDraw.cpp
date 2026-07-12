import Rl.Client.Render.Unit.UnitRendererDraw;
import Rl.Client.Render.Unit.UnitRendererShadowMap;
import Rl.Client.Render.Unit.UnitRendererVertices;
import Rl.Client.Render.Unit.UnitRendererInfo;
import Rl.Client.Render.Unit.UnitRendererBasicBuffer;
import Rl.Client.State.UnitState;
import Rl.Player.PlayerCamera;
import Rl.World.ServiceLocator;
import Rl.Base.Binding;

import <algorithm>;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <vulkan/vulkan.hpp>;

namespace Rl::Client::Render
{

void UnitUpdateCascadeShadowMaps(const glm::mat4& cameraView,
    const glm::mat4&                              cameraPerspective,
    const glm::vec3&                              sunDirection,
    float                                         cameraAspect,
    float                                         cameraNear,
    float                                         cameraFar,
    uint32_t                                      numCascades,
    UnitCascadeShadowLightingUniforms&            shadowUniforms,
    glm::vec4&                                    cascadeSplits,
    uint32_t&                                     cascadeCount)
{
#pragma push_macro("min")

#undef min
  const uint32_t effectiveCascades = std::min(numCascades, 4u);
#pragma pop_macro("min")

  glm::vec3          lightDir = glm::normalize(sunDirection);
  std::vector<float> splits;
  splits.push_back(cameraNear);

  for (uint32_t i = 1; i < effectiveCascades; ++i)
  {
    float lambda = i / static_cast<float>(effectiveCascades);
    float linearSplit = cameraNear + (cameraFar - cameraNear) * lambda;
    float logSplit = cameraNear * glm::pow(cameraFar / cameraNear, lambda);
    float split = glm::mix(linearSplit, logSplit, 0.7f);
    splits.push_back(split);
  }
  splits.push_back(cameraFar);
  for (uint32_t i = 0; i < effectiveCascades; ++i)
  {
    float                  nearSplit = splits[i];
    float                  farSplit = splits[i + 1];
    std::vector<glm::vec3> frustumCorners;

    float tanHalfFovY = 1.0f / cameraPerspective[1][1];
    float tanHalfFovX = tanHalfFovY * cameraAspect;

    // Generate corners
    for (int corner = 0; corner < 8; ++corner)
    {
      float depth = (corner < 4) ? nearSplit : farSplit;
      float xSign = (corner % 2 == 0) ? -1.0f : 1.0f;
      float ySign = (corner % 4 < 2) ? 1.0f : -1.0f;

      glm::vec3 cornerView;
      cornerView.x = tanHalfFovX * depth * xSign;
      cornerView.y = tanHalfFovY * depth * ySign;
      cornerView.z = -depth;

      glm::vec4 cornerWorld = glm::inverse(cameraView) * glm::vec4(cornerView, 1.0f);
      frustumCorners.push_back(glm::vec3(cornerWorld) / cornerWorld.w);
    }

    glm::vec3 frustumCenter = glm::vec3(0.0f);
    for (const auto& corner : frustumCorners)
    {
      frustumCenter += corner;
    }
    frustumCenter /= 8.0f;

    glm::vec3 lightPos = frustumCenter - lightDir * 100.0f;
    glm::mat4 lightView = glm::lookAt(lightPos, frustumCenter, glm::vec3(0, 1, 0));

    // Calcular bounds en espacio de luz
    float minX = FLT_MAX, maxX = -FLT_MAX;
    float minY = FLT_MAX, maxY = -FLT_MAX;
    float minZ = FLT_MAX, maxZ = -FLT_MAX;

    for (const auto& corner : frustumCorners)
    {
      glm::vec4 lightSpace = lightView * glm::vec4(corner, 1.0f);
#pragma push_macro("min")

#undef min

#pragma push_macro("max")

#undef max
      minX = glm::min(minX, lightSpace.x);
      maxX = glm::max(maxX, lightSpace.x);
      minY = glm::min(minY, lightSpace.y);
      maxY = glm::max(maxY, lightSpace.y);
      minZ = glm::min(minZ, lightSpace.z);
      maxZ = glm::max(maxZ, lightSpace.z);
    }
#pragma pop_macro("min")

#pragma pop_macro("max")
    float     marginX = (maxX - minX) * 0.1f;
    float     marginY = (maxY - minY) * 0.1f;
    glm::mat4 lightProjection = glm::ortho(
        minX - marginX, maxX + marginX, minY - marginY, maxY + marginY, minZ, maxZ);
    shadowUniforms.lightSpaceMatrices[i] = lightProjection * lightView;
  }

  glm::vec4 splitValues(cameraFar, cameraFar, cameraFar, cameraFar);
  if (effectiveCascades > 1)
  {
    splitValues.x = splits[1];
  }
  if (effectiveCascades > 2)
  {
    splitValues.y = splits[2];
  }
  if (effectiveCascades > 3)
  {
    splitValues.z = splits[3];
    splitValues.w = splits[3];
  }
  cascadeSplits = splitValues;
  cascadeCount = effectiveCascades;
}

void UnitRenderCascadeShadowMap(Providers::UnitStateResource& resource,
    Providers::UnitStateBinding&                              vk,
    Main::MainBinding&                                        context)
{
  if (vk.shadowPipeline == VK_NULL_HANDLE || vk.shadowPipelineLayout == VK_NULL_HANDLE ||
      vk.shadowMapRenderPass == VK_NULL_HANDLE || vk.shadowMapCascades.empty())
    return;
  for (uint32_t i = 0; i < vk.shadowMapCascades.size(); ++i)
  {
    UnitBeginCascadeShadowMapRenderPass(context.commandBuffers[0], vk.shadowMapRenderPass,
            vk.shadowMapCascades[i].framebuffer, 2048, 2048);

    vkCmdBindPipeline(context.commandBuffers[0], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.shadowPipeline);

    const VkBuffer vertexBuffers[] = {vk.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(context.commandBuffers[0], 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(context.commandBuffers[0], vk.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    glm::mat4 lightSpaceMatrix = vk.shadowCascadeLighting.lightSpaceMatrices[i];
    vkCmdPushConstants(context.commandBuffers[0], vk.shadowPipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &lightSpaceMatrix);

    vkCmdDrawIndexed(context.commandBuffers[0], 36, 1, 0, 0, 0);

    UnitEndCascadeShadowMapRenderPass(context.commandBuffers[0]);
  }
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

  UnitRenderPushConstants pushConstants{};
  pushConstants.useUnitArray = vk.useUnitArrayMode ? 1 : 0;
  pushConstants.singleUnitMode = vk.singleUnitMode ? 1 : 0;
  pushConstants.singleUnitId = vk.singleUnitId;
  pushConstants.padding1 = 0;

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
  if (vk.pipeline != VK_NULL_HANDLE && vk.pipelineLayout != VK_NULL_HANDLE)
  {
    // Update MVP uniform buffer for this frame
    UnitRenderUBO mvpUBO{};
    mvpUBO.model = model;
    mvpUBO.view = view;
    mvpUBO.projection = projection;
    UnitCopyDataToBuffer(context.device, vk.mvpBufferMemory, 0, sizeof(UnitRenderUBO), &mvpUBO);

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
      // Push camera matrices and unit array mode constants
      vkCmdPushConstants(context.commandBuffers[0], vk.pipelineLayout,
          VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UnitRenderPushConstants), &pushConstants);
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
      // Push camera matrices and unit array mode constants
      vkCmdPushConstants(context.commandBuffers[0], vk.pipelineLayout,
          VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UnitRenderPushConstants), &pushConstants);
      // Draw using indexed indirect draw parameters from compute shader
      vkCmdDrawIndexedIndirect(context.commandBuffers[0], vk.indirectDrawBuffer, 0, 1,
          sizeof(VkDrawIndexedIndirectCommand));
    }
  }
}

} // namespace Rl::Client::Render
