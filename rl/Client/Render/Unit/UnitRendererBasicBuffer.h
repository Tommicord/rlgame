#pragma once

#include "rl/Client/Render/Unit/UnitRendererInfo.h"
#include "rl/Client/State/UnitState.h"

#include <vulkan/vulkan.hpp>

namespace Rl::Client::Render
{

/* Base function for creating a Vulkan Buffer */
void UnitCreateBuffer(VkDevice device,
    VkPhysicalDevice           physicalDevice,
    VkDeviceSize               size,
    VkBufferUsageFlags         usage,
    VkMemoryPropertyFlags      properties,
    VkBuffer&                  buffer,
    VkDeviceMemory&            bufferMemory);

/* Creates a Vulkan Index Buffer, requires the Index buffer memory */
void UnitCreateIndexBuffer(VkDevice device,
    VkPhysicalDevice                physicalDevice,
    const std::vector<uint32_t>&    indices,
    VkBuffer&                       indexBuffer,
    VkDeviceMemory&                 indexBufferMemory);

/*  Maps the memory to the output destination (data) with the specified size */
void UnitCopyDataToBuffer(VkDevice device,
    VkDeviceMemory                 bufferMemory,
    VkDeviceSize                   offset,
    VkDeviceSize                   size,
    const void*                    data);

/* Creates a Vertex Buffer for rendering in Vulkan, requires the Index Buffer memory */
void UnitCreateVertexBuffer(VkDevice     device,
    VkPhysicalDevice                     physicalDevice,
    const std::vector<UnitRenderVertex>& vertices,
    VkBuffer&                            vertexBuffer,
    VkDeviceMemory&                      vertexBufferMemory);

/* Creates a Storage buffer, requires vertex count */
void UnitCreateSSBOBuffers(VkDevice     device,
    VkPhysicalDevice                    physicalDevice,
    size_t                              vertexCount,
    Providers::UnitStateDrawableVulkan& vk);

/* Creates the buffers for the curvature compute shader */
void UnitCreateCurvatureComputeBuffers(VkDevice device,
    VkPhysicalDevice                            physicalDevice,
    size_t                                      vertexCount,
    Providers::UnitStateDrawableVulkan&         vk);

/* Creates a Vulkan uniform buffer */
void UnitCreateUniformBuffers(
    VkDevice device, VkPhysicalDevice physicalDevice, Providers::UnitStateDrawableVulkan& vk);

} // namespace Rl::Client::Render
