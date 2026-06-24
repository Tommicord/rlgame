#include "rl/Client/Render/Unit/UnitRendererDrawCompute.h"
#include "rl/Client/Render/Unit/UnitRendererFrustum.h"
#include "rl/Client/Render/Unit/UnitRendererLightingBlock.h"
#include <glm/glm.hpp>

namespace Rl::Client::Render
{

void DispatchComputeShaders(Providers::UnitStateResource& resource,
    Providers::UnitStateDrawableVulkan&                   vk,
    Game::VulkanContext&                                  context)
{
  if (!resource.cameraModel)
    return;

  // Get camera matrices for push constants
  UnitRenderUBO        ubo{};
  const World::Camera& cam = resource.cameraModel->GetObject();
  ubo.model                = cam.GetModelMatrix();
  ubo.view                 = cam.GetViewMatrix();
  ubo.projection           = cam.GetProjectionMatrix();

  // Reset visible vertex counter using vkCmdFillBuffer (GPU-side operation)
  vkCmdFillBuffer(context.commandBuffers[0], vk.visibleCountBuffer, 0, sizeof(uint32_t), 0);

  // Update frustum data outside render pass
  UnitRenderFrustumPlanes frustum{};
  UnitCameraToFrustumPlanes(frustum, resource.cameraModel->GetObject());

  constexpr VkDeviceSize frustumSize = sizeof(UnitRenderFrustumPlanes);
  vkCmdUpdateBuffer(context.commandBuffers[0], vk.frustumBuffer, 0, frustumSize, &frustum);

  UnitRenderLightingBlock lightingData{};
  lightingData.sunDirection         = glm::normalize(glm::vec3(0.5f, 0.8f, 0.6f));
  lightingData.sunColor             = glm::vec3(1.0f, 0.3f, 0.6f);
  lightingData.ambientStrength      = 0.7f;
  World::AbstractCamera::Eye eyePos = cam.eye;
  lightingData.cameraPosition       = glm::vec3(eyePos.x, eyePos.y, eyePos.z);
  lightingData.exposure             = 1.0f;
  lightingData.padding1             = 0.0f;
  lightingData.padding2             = 0.0f;
  lightingData.padding3             = 0.0f;
  lightingData.groundColor          = glm::vec3(0.1f, 0.1f, 0.15f);
  lightingData.skyColor             = glm::vec3(0.5f, 0.7f, 1.0f);

  constexpr VkDeviceSize lightingBlockSize = sizeof(UnitRenderLightingBlock);
  vkCmdUpdateBuffer(
      context.commandBuffers[0], vk.placeholderLightingBuffer, 0, lightingBlockSize, &lightingData);

  VkMemoryBarrier fillBarrier{};
  fillBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  fillBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  fillBarrier.dstAccessMask =
      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_UNIFORM_READ_BIT;

  vkCmdPipelineBarrier(context.commandBuffers[0], VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &fillBarrier, 0, nullptr, 0, nullptr);

  // Dispatch curvature compute shader before frustum culling
  if (vk.curveComputePipeline != VK_NULL_HANDLE && vk.curveComputePipelineLayout != VK_NULL_HANDLE)
  {
    // Reset counters for curvature compute
    uint32_t zeroCounters[2] = {0, 0};
    vkCmdUpdateBuffer(
        context.commandBuffers[0], vk.curveCountersBuffer, 0, 2 * sizeof(uint32_t), zeroCounters);

    VkMemoryBarrier curveResetBarrier{};
    curveResetBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    curveResetBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    curveResetBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

    vkCmdPipelineBarrier(context.commandBuffers[0], VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &curveResetBarrier, 0, nullptr, 0, nullptr);

    vkCmdBindPipeline(
        context.commandBuffers[0], VK_PIPELINE_BIND_POINT_COMPUTE, vk.curveComputePipeline);
    vkCmdBindDescriptorSets(context.commandBuffers[0], VK_PIPELINE_BIND_POINT_COMPUTE,
        vk.curveComputePipelineLayout, 0, 1, &vk.curveComputeDescriptorSet, 0, nullptr);

    // Push constants: tessellation level (8) and face count (6)
    uint32_t curvePushConstants[2] = {8, 6};
    vkCmdPushConstants(context.commandBuffers[0], vk.curveComputePipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(curvePushConstants), curvePushConstants);

    // Dispatch one workgroup per face (6 faces)
    vkCmdDispatch(context.commandBuffers[0], 6, 1, 1);

    VkBufferMemoryBarrier curveBarrier{};
    curveBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    curveBarrier.srcAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
    curveBarrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
    curveBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    curveBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    curveBarrier.buffer              = vk.curvedVertexBuffer;
    curveBarrier.offset              = 0;
    curveBarrier.size                = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(context.commandBuffers[0], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &curveBarrier, 0, nullptr);
  }

  // Dispatch compute shader for frustum culling
  if (vk.computePipeline != VK_NULL_HANDLE && vk.computePipelineLayout != VK_NULL_HANDLE)
  {
    vkCmdBindPipeline(
        context.commandBuffers[0], VK_PIPELINE_BIND_POINT_COMPUTE, vk.computePipeline);
    vkCmdBindDescriptorSets(context.commandBuffers[0], VK_PIPELINE_BIND_POINT_COMPUTE,
        vk.computePipelineLayout, 0, 1, &vk.computeDescriptorSet, 0, nullptr);
    vkCmdPushConstants(context.commandBuffers[0], vk.computePipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UnitRenderUBO), &ubo);

    // Calculate workgroup count, each workgroup processes 64 triangles
    uint32_t triangleCount  = 36 / 3; // 12 triangles
    uint32_t workgroupCount = (triangleCount + 63) / 64;
    vkCmdDispatch(context.commandBuffers[0], workgroupCount, 1, 1);

    VkBufferMemoryBarrier computeToCopyBarrier{};
    computeToCopyBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    computeToCopyBarrier.srcAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
    computeToCopyBarrier.dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT;
    computeToCopyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    computeToCopyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    computeToCopyBarrier.buffer              = vk.visibleCountBuffer;
    computeToCopyBarrier.offset              = 0;
    computeToCopyBarrier.size                = sizeof(uint32_t);

    vkCmdPipelineBarrier(context.commandBuffers[0], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &computeToCopyBarrier, 0, nullptr);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size      = sizeof(uint32_t);
    vkCmdCopyBuffer(
        context.commandBuffers[0], vk.visibleCountBuffer, vk.indirectDrawBuffer, 1, &copyRegion);

    VkBufferMemoryBarrier indirectTransferBarrier{};
    indirectTransferBarrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    indirectTransferBarrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
    indirectTransferBarrier.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
    indirectTransferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    indirectTransferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    indirectTransferBarrier.buffer              = vk.indirectDrawBuffer;
    indirectTransferBarrier.offset              = 0;
    indirectTransferBarrier.size                = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(context.commandBuffers[0], VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &indirectTransferBarrier, 0, nullptr);

    vkCmdFillBuffer(
        context.commandBuffers[0], vk.indirectDrawBuffer, sizeof(uint32_t), sizeof(uint32_t), 1);
    vkCmdFillBuffer(context.commandBuffers[0], vk.indirectDrawBuffer, sizeof(uint32_t) * 2,
        sizeof(uint32_t), 0);
    vkCmdFillBuffer(context.commandBuffers[0], vk.indirectDrawBuffer, sizeof(uint32_t) * 3,
        sizeof(uint32_t), 0);
    vkCmdFillBuffer(context.commandBuffers[0], vk.indirectDrawBuffer, sizeof(uint32_t) * 4,
        sizeof(uint32_t), 0);

    VkBufferMemoryBarrier barriers[2]{};

    barriers[0].sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barriers[0].srcAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
    barriers[0].dstAccessMask       = VK_ACCESS_INDEX_READ_BIT;
    barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].buffer              = vk.outputIndexBuffer;
    barriers[0].offset              = 0;
    barriers[0].size                = VK_WHOLE_SIZE;

    barriers[1].sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barriers[1].srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[1].dstAccessMask       = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[1].buffer              = vk.indirectDrawBuffer;
    barriers[1].offset              = 0;
    barriers[1].size                = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(context.commandBuffers[0],
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 0, nullptr, 2,
        barriers, 0, nullptr);
  }
}

} // namespace Rl::Client::Render
