export module Rl.Client.Render.Unit.UnitRendererBasicBuffer;

import Rl.Client.Render.Unit.UnitRendererInfo;
import Rl.Client.State.UnitState;
import <vulkan/vulkan.hpp>;
import <vector>;

namespace Rl::Client::Render
{

/* Gets a memory type index frm device memory requirements */
export uint32_t UnitFindMemoryTypeIndex(VkPhysicalDevice physicalDevice,
    VkMemoryRequirements                                 memRequirements,
    VkMemoryPropertyFlags                                properties);

/* Base function for creating a Vulkan Buffer */
export void UnitCreateBuffer(VkDevice device,
    VkPhysicalDevice                  physicalDevice,
    VkDeviceSize                      size,
    VkBufferUsageFlags                usage,
    VkMemoryPropertyFlags             properties,
    VkBuffer&                         buffer,
    VkDeviceMemory&                   bufferMemory);

/* Creates a Vulkan Index Buffer, requires the Index buffer memory */
export void UnitCreateIndexBuffer(VkDevice device,
    VkPhysicalDevice                       physicalDevice,
    uint32_t                               queueFamilyIndex,
    const std::vector<uint32_t>&           indices,
    VkBuffer&                              indexBuffer,
    VkDeviceMemory&                        indexBufferMemory);

/*  Maps the memory to the output destination (data) with the specified size */
export void UnitCopyDataToBuffer(VkDevice device,
    VkDeviceMemory                        bufferMemory,
    VkDeviceSize                          offset,
    VkDeviceSize                          size,
    const void*                           data);

/* Creates a Vertex Buffer for rendering in Vulkan, requires the Index Buffer memory */
export void UnitCreateVertexBuffer(VkDevice device,
    VkPhysicalDevice                        physicalDevice,
    uint32_t                                queueFamilyIndex,
    const std::vector<UnitRenderVertex>&    vertices,
    VkBuffer&                               vertexBuffer,
    VkDeviceMemory&                         vertexBufferMemory);

/* Creates a Storage buffer, requires vertex count */
export void UnitCreateSSBOBuffers(VkDevice device,
    VkPhysicalDevice                       physicalDevice,
    size_t                                 vertexCount,
    Providers::UnitStateBinding&           vk);

/* Creates the buffers for the curvature compute shader */
export void UnitCreateCurvatureComputeBuffers(VkDevice device,
    VkPhysicalDevice                                   physicalDevice,
    size_t                                             vertexCount,
    Providers::UnitStateBinding&                       vk);

/* Creates a Vulkan uniform buffer */
export void UnitCreateUniformBuffers(
    VkDevice device, VkPhysicalDevice physicalDevice, Providers::UnitStateBinding& vk);

/* Creates unit array buffer for GPU unit data */
export void UnitCreateUnitArrayBuffer(VkDevice device,
    VkPhysicalDevice                       physicalDevice,
    uint32_t                               queueFamilyIndex,
    const std::vector<UnitRenderUnitData>& unitData,
    VkBuffer&                              unitArrayBuffer,
    VkDeviceMemory&                        unitArrayMemory);

/* Creates polygon fence array buffer for GPU fence data */
export void UnitCreatePolFenceArrayBuffer(VkDevice device,
    VkPhysicalDevice                       physicalDevice,
    uint32_t                               queueFamilyIndex,
    const std::vector<UnitRenderPolFence>& fenceData,
    VkBuffer&                              fenceArrayBuffer,
    VkDeviceMemory&                        fenceArrayMemory);

} // namespace Rl::Client::Render
