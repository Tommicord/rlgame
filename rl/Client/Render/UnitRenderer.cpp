#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <iostream>

#include "rl/Client/State/UnitState.h"
#include "rl/World/Unit/Unit.h"
#include "rl/World/Unit/UnitGrass.h"
#include "rl/Base/ShaderFactory.h"
#include "rl/Base/Texture2.h"
#include "rl/Base/Game.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>

namespace Rl::Providers {

// Vertex structure for Unit rendering
struct UnitVertex {
    glm::vec3 position;
    glm::vec2 texCoords;
    uint32_t lightingEmit;
    uint32_t transparencyLevel;
    uint32_t faceIndex;
    glm::vec4 polRight;
    glm::vec4 polLeft;
    glm::vec3 albedo;
    float metallic;
    float roughness;
};

// Unit cube vertices (6 faces, 2 triangles each, 3 vertices per triangle)
static const std::vector<UnitVertex> unitVertices = {
    // Top face (faceIndex = 0)
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f}, 0, 0, 0, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{ 0.5f, 0.5f, -0.5f}, {1.0f, 0.0f}, 0, 0, 0, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{ 0.5f, 0.5f,  0.5f}, {1.0f, 1.0f}, 0, 0, 0, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{ 0.5f, 0.5f,  0.5f}, {1.0f, 1.0f}, 0, 0, 0, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{-0.5f, 0.5f,  0.5f}, {0.0f, 1.0f}, 0, 0, 0, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f}, 0, 0, 0, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    
    // Bottom face (faceIndex = 1)
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, 0, 0, 1, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}, 0, 0, 1, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, 0, 0, 1, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}, 0, 0, 1, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, 0, 0, 1, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, 0, 0, 1, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    
    // Left face (faceIndex = 2)
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, 0, 0, 2, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, 0, 0, 2, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, 0, 0, 2, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, 0, 0, 2, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, 0, 0, 2, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, 0, 0, 2, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    
    // Right face (faceIndex = 3)
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, 0, 0, 3, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, 0, 0, 3, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, 0, 0, 3, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, 0, 0, 3, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, 0, 0, 3, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, 0, 0, 3, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    
    // Front face (faceIndex = 4)
    {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, 0, 0, 4, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}, 0, 0, 4, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, 0, 0, 4, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, 0, 0, 4, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}, 0, 0, 4, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, 0, 0, 4, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    
    // Back face (faceIndex = 5)
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, 0, 0, 5, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}, 0, 0, 5, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}, 0, 0, 5, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}, 0, 0, 5, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}, 0, 0, 5, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, 0, 0, 5, {0,0,0,0}, {0,0,0,0}, {1,1,1}, 0.0f, 0.5f},
};

// Uniform buffer structure
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};

// Lighting uniforms
struct LightingUniforms {
    alignas(16) glm::vec3 sunDirection;
    alignas(16) glm::vec3 sunColor;
    alignas(4)  float ambientStrength;
    alignas(16) glm::vec3 cameraPosition;
};

// Frustum planes for culling
struct FrustumPlanes {
    glm::vec4 planes[6];
};

// Vertex output structure matching compute shader
struct VertexOutput {
    glm::vec4 position;
    glm::vec3 worldPos;
    glm::vec2 texCoords;
    uint32_t lightingEmit;
    uint32_t transparencyLevel;
    uint32_t faceIndex;
    glm::vec3 albedo;
    float metallic;
    float roughness;
};

// Indirect draw parameters
struct DrawParams {
    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;
};

uint32_t FindMemoryTypeIndex(VkPhysicalDevice physicalDevice, VkMemoryRequirements memRequirements, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("Failed to find suitable memory type");
}

void UnitStateDrawable::OnCreate(UnitStateResource& resource,
                                 UnitStateDrawableVulkan& vk,
                                 Game::VulkanContext& context)
{
    // Create input vertex buffer (SSBO for compute shader)
    VkDeviceSize inputBufferSize = sizeof(unitVertices[0]) * unitVertices.size();
    
    VkBufferCreateInfo inputBufferInfo{};
    inputBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    inputBufferInfo.size = inputBufferSize;
    inputBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    inputBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(context.device, &inputBufferInfo, nullptr, &vk.vertexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create input vertex buffer");
    }
    
    VkMemoryRequirements inputMemRequirements;
    vkGetBufferMemoryRequirements(context.device, vk.vertexBuffer, &inputMemRequirements);
    
    VkMemoryAllocateInfo inputAllocInfo{};
    inputAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    inputAllocInfo.allocationSize = inputMemRequirements.size;
    inputAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(context.physicalDevice, inputMemRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(context.device, &inputAllocInfo, nullptr, &vk.vertexBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate input vertex buffer memory");
    }
    
    vkBindBufferMemory(context.device, vk.vertexBuffer, vk.vertexBufferMemory, 0);
    
    // Copy vertex data to input buffer
    void* inputData;
    vkMapMemory(context.device, vk.vertexBufferMemory, 0, inputBufferSize, 0, &inputData);
    memcpy(inputData, unitVertices.data(), inputBufferSize);
    vkUnmapMemory(context.device, vk.vertexBufferMemory);
    
    // Create output vertex buffer (SSBO for compute shader, vertex buffer for graphics)
    VkDeviceSize outputBufferSize = sizeof(VertexOutput) * unitVertices.size(); // Same size as input
    
    VkBufferCreateInfo outputBufferInfo{};
    outputBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    outputBufferInfo.size = outputBufferSize;
    outputBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    outputBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(context.device, &outputBufferInfo, nullptr, &vk.outputVertexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create output vertex buffer");
    }
    
    VkMemoryRequirements outputMemRequirements;
    vkGetBufferMemoryRequirements(context.device, vk.outputVertexBuffer, &outputMemRequirements);
    
    VkMemoryAllocateInfo outputAllocInfo{};
    outputAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    outputAllocInfo.allocationSize = outputMemRequirements.size;
    outputAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(context.physicalDevice, outputMemRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (vkAllocateMemory(context.device, &outputAllocInfo, nullptr, &vk.outputVertexBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate output vertex buffer memory");
    }
    
    vkBindBufferMemory(context.device, vk.outputVertexBuffer, vk.outputVertexBufferMemory, 0);
    
    // Create visible count buffer (atomic counter)
    VkBufferCreateInfo countBufferInfo{};
    countBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    countBufferInfo.size = sizeof(uint32_t);
    countBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    countBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(context.device, &countBufferInfo, nullptr, &vk.visibleCountBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create visible count buffer");
    }
    
    VkMemoryRequirements countMemRequirements;
    vkGetBufferMemoryRequirements(context.device, vk.visibleCountBuffer, &countMemRequirements);
    
    VkMemoryAllocateInfo countAllocInfo{};
    countAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    countAllocInfo.allocationSize = countMemRequirements.size;
    countAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(context.physicalDevice, countMemRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(context.device, &countAllocInfo, nullptr, &vk.visibleCountBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate visible count buffer memory");
    }
    
    vkBindBufferMemory(context.device, vk.visibleCountBuffer, vk.visibleCountBufferMemory, 0);
    
    // Initialize visible count to 0
    uint32_t initialCount = 0;
    void* countData;
    vkMapMemory(context.device, vk.visibleCountBufferMemory, 0, sizeof(uint32_t), 0, &countData);
    memcpy(countData, &initialCount, sizeof(uint32_t));
    vkUnmapMemory(context.device, vk.visibleCountBufferMemory);
    
    // Create indirect draw buffer
    VkBufferCreateInfo indirectBufferInfo{};
    indirectBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indirectBufferInfo.size = sizeof(DrawParams);
    indirectBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    indirectBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(context.device, &indirectBufferInfo, nullptr, &vk.indirectDrawBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create indirect draw buffer");
    }
    
    VkMemoryRequirements indirectMemRequirements;
    vkGetBufferMemoryRequirements(context.device, vk.indirectDrawBuffer, &indirectMemRequirements);
    
    VkMemoryAllocateInfo indirectAllocInfo{};
    indirectAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    indirectAllocInfo.allocationSize = indirectMemRequirements.size;
    indirectAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(context.physicalDevice, indirectMemRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(context.device, &indirectAllocInfo, nullptr, &vk.indirectDrawBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate indirect draw buffer memory");
    }
    
    vkBindBufferMemory(context.device, vk.indirectDrawBuffer, vk.indirectDrawBufferMemory, 0);
    
    // Create frustum planes uniform buffer
    VkBufferCreateInfo frustumBufferInfo{};
    frustumBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    frustumBufferInfo.size = sizeof(FrustumPlanes);
    frustumBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    frustumBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(context.device, &frustumBufferInfo, nullptr, &vk.frustumBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create frustum buffer");
    }
    
    VkMemoryRequirements frustumMemRequirements;
    vkGetBufferMemoryRequirements(context.device, vk.frustumBuffer, &frustumMemRequirements);
    
    VkMemoryAllocateInfo frustumAllocInfo{};
    frustumAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    frustumAllocInfo.allocationSize = frustumMemRequirements.size;
    frustumAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(context.physicalDevice, frustumMemRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(context.device, &frustumAllocInfo, nullptr, &vk.frustumBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate frustum buffer memory");
    }
    
    vkBindBufferMemory(context.device, vk.frustumBuffer, vk.frustumBufferMemory, 0);
    
    // Load compute shader
    auto computeShaderCode = ShaderObject::Shader("unit.face.comp.spv");
    auto computeShaderModule = ShaderObject::CreateShaderModule(context.device, computeShaderCode);
    
    // Create compute pipeline
    VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
    computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = computeShaderModule.module;
    computeShaderStageInfo.pName = "main";
    
    VkComputePipelineCreateInfo computePipelineInfo{};
    computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineInfo.stage = computeShaderStageInfo;
    computePipelineInfo.layout = vk.pipelineLayout; // Will be created below
    
    // Create descriptor set layout for compute shader (SSBOs)
    VkDescriptorSetLayoutBinding computeBindings[5]{};
    // Input vertices SSBO (binding 0)
    computeBindings[0].binding = 0;
    computeBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    computeBindings[0].descriptorCount = 1;
    computeBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    // Output vertices SSBO (binding 1)
    computeBindings[1].binding = 1;
    computeBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    computeBindings[1].descriptorCount = 1;
    computeBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    // Visible count SSBO (binding 2)
    computeBindings[2].binding = 2;
    computeBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    computeBindings[2].descriptorCount = 1;
    computeBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    // Indirect draw SSBO (binding 3)
    computeBindings[3].binding = 3;
    computeBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    computeBindings[3].descriptorCount = 1;
    computeBindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    // Frustum planes uniform buffer (binding 4)
    computeBindings[4].binding = 4;
    computeBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    computeBindings[4].descriptorCount = 1;
    computeBindings[4].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    VkDescriptorSetLayoutCreateInfo computeLayoutInfo{};
    computeLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    computeLayoutInfo.bindingCount = 5;
    computeLayoutInfo.pBindings = computeBindings;
    
    if (vkCreateDescriptorSetLayout(context.device, &computeLayoutInfo, nullptr, &vk.computeDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create compute descriptor set layout");
    }
    
    // Create descriptor set layout for graphics pipeline (textures)
    VkDescriptorSetLayoutBinding graphicsBindings[3]{};
    // Texture array (binding 2)
    graphicsBindings[0].binding = 2;
    graphicsBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    graphicsBindings[0].descriptorCount = 6; // 6 textures for 6 faces
    graphicsBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // Lighting texture (binding 8)
    graphicsBindings[1].binding = 8;
    graphicsBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    graphicsBindings[1].descriptorCount = 1;
    graphicsBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // Settings uniform buffer (binding 9)
    graphicsBindings[2].binding = 9;
    graphicsBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    graphicsBindings[2].descriptorCount = 1;
    graphicsBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo graphicsLayoutInfo{};
    graphicsLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    graphicsLayoutInfo.bindingCount = 3;
    graphicsLayoutInfo.pBindings = graphicsBindings;
    
    if (vkCreateDescriptorSetLayout(context.device, &graphicsLayoutInfo, nullptr, &vk.descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics descriptor set layout");
    }
    
    // Create descriptor pool
    VkDescriptorPoolSize poolSizes[3]{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[0].descriptorCount = 4; // 4 SSBOs for compute
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 7; // 6 textures + 1 lighting texture
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[2].descriptorCount = 4;
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 3;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 2; // 1 for compute, 1 for graphics
    
    if (vkCreateDescriptorPool(context.device, &poolInfo, nullptr, &vk.descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool");
    }
    
    // Allocate compute descriptor set
    VkDescriptorSetAllocateInfo computeAllocInfo{};
    computeAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    computeAllocInfo.descriptorPool = vk.descriptorPool;
    computeAllocInfo.descriptorSetCount = 1;
    computeAllocInfo.pSetLayouts = &vk.computeDescriptorSetLayout;
    
    if (vkAllocateDescriptorSets(context.device, &computeAllocInfo, &vk.computeDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate compute descriptor set");
    }
    
    // Update compute descriptor set with SSBOs
    VkDescriptorBufferInfo inputBufferInfo2{};
    inputBufferInfo2.buffer = vk.vertexBuffer;
    inputBufferInfo2.offset = 0;
    inputBufferInfo2.range = sizeof(unitVertices[0]) * unitVertices.size();
    
    VkDescriptorBufferInfo outputBufferInfo2{};
    outputBufferInfo2.buffer = vk.outputVertexBuffer;
    outputBufferInfo2.offset = 0;
    outputBufferInfo2.range = sizeof(VertexOutput) * unitVertices.size();
    
    VkDescriptorBufferInfo countBufferInfo2{};
    countBufferInfo2.buffer = vk.visibleCountBuffer;
    countBufferInfo2.offset = 0;
    countBufferInfo2.range = sizeof(uint32_t);
    
    VkDescriptorBufferInfo indirectBufferInfo2{};
    indirectBufferInfo2.buffer = vk.indirectDrawBuffer;
    indirectBufferInfo2.offset = 0;
    indirectBufferInfo2.range = sizeof(DrawParams);
    
    VkDescriptorBufferInfo frustumBufferInfo2{};
    frustumBufferInfo2.buffer = vk.frustumBuffer;
    frustumBufferInfo2.offset = 0;
    frustumBufferInfo2.range = sizeof(FrustumPlanes);
    
    VkWriteDescriptorSet computeWrites[5]{};
    computeWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    computeWrites[0].dstSet = vk.computeDescriptorSet;
    computeWrites[0].dstBinding = 0;
    computeWrites[0].dstArrayElement = 0;
    computeWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    computeWrites[0].descriptorCount = 1;
    computeWrites[0].pBufferInfo = &inputBufferInfo2;
    
    computeWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    computeWrites[1].dstSet = vk.computeDescriptorSet;
    computeWrites[1].dstBinding = 1;
    computeWrites[1].dstArrayElement = 0;
    computeWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    computeWrites[1].descriptorCount = 1;
    computeWrites[1].pBufferInfo = &outputBufferInfo2;
    
    computeWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    computeWrites[2].dstSet = vk.computeDescriptorSet;
    computeWrites[2].dstBinding = 2;
    computeWrites[2].dstArrayElement = 0;
    computeWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    computeWrites[2].descriptorCount = 1;
    computeWrites[2].pBufferInfo = &countBufferInfo2;
    
    computeWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    computeWrites[3].dstSet = vk.computeDescriptorSet;
    computeWrites[3].dstBinding = 3;
    computeWrites[3].dstArrayElement = 0;
    computeWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    computeWrites[3].descriptorCount = 1;
    computeWrites[3].pBufferInfo = &indirectBufferInfo2;
    
    computeWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    computeWrites[4].dstSet = vk.computeDescriptorSet;
    computeWrites[4].dstBinding = 4;
    computeWrites[4].dstArrayElement = 0;
    computeWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    computeWrites[4].descriptorCount = 1;
    computeWrites[4].pBufferInfo = &frustumBufferInfo2;
    
    vkUpdateDescriptorSets(context.device, 5, computeWrites, 0, nullptr);
    
    // Allocate graphics descriptor set
    VkDescriptorSetAllocateInfo graphicsAllocInfo{};
    graphicsAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    graphicsAllocInfo.descriptorPool = vk.descriptorPool;
    graphicsAllocInfo.descriptorSetCount = 1;
    graphicsAllocInfo.pSetLayouts = &vk.descriptorSetLayout;
    
    if (vkAllocateDescriptorSets(context.device, &graphicsAllocInfo, &vk.descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate graphics descriptor set");
    }
    
    // Create placeholder resources BEFORE pipeline creation
    // Create placeholder lighting texture (1x1 white texture)
    VkImageCreateInfo lightingImageInfo{};
    lightingImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    lightingImageInfo.imageType = VK_IMAGE_TYPE_2D;
    lightingImageInfo.extent.width = 1;
    lightingImageInfo.extent.height = 1;
    lightingImageInfo.extent.depth = 1;
    lightingImageInfo.mipLevels = 1;
    lightingImageInfo.arrayLayers = 1;
    lightingImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    lightingImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    lightingImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    lightingImageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    lightingImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    lightingImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(context.device, &lightingImageInfo, nullptr, &vk.placeholderLightingTexture) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create placeholder lighting texture");
    }

    VkMemoryRequirements lightingMemRequirements;
    vkGetImageMemoryRequirements(context.device, vk.placeholderLightingTexture, &lightingMemRequirements);

    VkMemoryAllocateInfo lightingAllocInfo{};
    lightingAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    lightingAllocInfo.allocationSize = lightingMemRequirements.size;
    lightingAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(context.physicalDevice, lightingMemRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(context.device, &lightingAllocInfo, nullptr, &vk.placeholderLightingTextureMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate placeholder lighting texture memory");
    }

    vkBindImageMemory(context.device, vk.placeholderLightingTexture, vk.placeholderLightingTextureMemory, 0);
    
    VkCommandPool tempCommandPool = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo poolInfo2{};
    poolInfo2.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo2.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo2.queueFamilyIndex = context.queueFamilyIndices.graphicsFamily.value();

    if (context.commandPool == VK_NULL_HANDLE) {
        if (vkCreateCommandPool(context.device, &poolInfo2, nullptr, &tempCommandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create temporary command pool");
        }
    }

    VkCommandBufferAllocateInfo transitionAllocInfo{};
    transitionAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    transitionAllocInfo.commandPool = (context.commandPool != VK_NULL_HANDLE) ? context.commandPool : tempCommandPool;
    transitionAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    transitionAllocInfo.commandBufferCount = 1;

    VkCommandBuffer transitionCommandBuffer;
    vkAllocateCommandBuffers(context.device, &transitionAllocInfo, &transitionCommandBuffer);

    VkCommandBufferBeginInfo transitionBeginInfo{};
    transitionBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    transitionBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(transitionCommandBuffer, &transitionBeginInfo);

    VkImageMemoryBarrier transitionBarrier{};
    transitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transitionBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    transitionBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    transitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    transitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    transitionBarrier.image = vk.placeholderLightingTexture;
    transitionBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    transitionBarrier.subresourceRange.baseMipLevel = 0;
    transitionBarrier.subresourceRange.levelCount = 1;
    transitionBarrier.subresourceRange.baseArrayLayer = 0;
    transitionBarrier.subresourceRange.layerCount = 1;
    transitionBarrier.srcAccessMask = 0;
    transitionBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(transitionCommandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &transitionBarrier);

    vkEndCommandBuffer(transitionCommandBuffer);

    VkSubmitInfo transitionSubmitInfo{};
    transitionSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    transitionSubmitInfo.commandBufferCount = 1;
    transitionSubmitInfo.pCommandBuffers = &transitionCommandBuffer;

    vkQueueSubmit(context.graphicsQueue, 1, &transitionSubmitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(context.graphicsQueue);

    VkCommandPool poolToFree = (context.commandPool != VK_NULL_HANDLE) ? context.commandPool : tempCommandPool;
    vkFreeCommandBuffers(context.device, poolToFree, 1, &transitionCommandBuffer);
    if (tempCommandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(context.device, tempCommandPool, nullptr);
    }

    // Create image view for placeholder lighting texture
    VkImageViewCreateInfo lightingViewInfo{};
    lightingViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    lightingViewInfo.image = vk.placeholderLightingTexture;
    lightingViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    lightingViewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    lightingViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    lightingViewInfo.subresourceRange.baseMipLevel = 0;
    lightingViewInfo.subresourceRange.levelCount = 1;
    lightingViewInfo.subresourceRange.baseArrayLayer = 0;
    lightingViewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(context.device, &lightingViewInfo, nullptr, &vk.placeholderLightingTextureView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create placeholder lighting texture view");
    }

    // Create placeholder sampler
    VkSamplerCreateInfo lightingSamplerInfo{};
    lightingSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    lightingSamplerInfo.magFilter = VK_FILTER_LINEAR;
    lightingSamplerInfo.minFilter = VK_FILTER_LINEAR;
    lightingSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    lightingSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    lightingSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    lightingSamplerInfo.anisotropyEnable = VK_FALSE;
    lightingSamplerInfo.maxAnisotropy = 1.0f;
    lightingSamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    lightingSamplerInfo.unnormalizedCoordinates = VK_FALSE;
    lightingSamplerInfo.compareEnable = VK_FALSE;
    lightingSamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    lightingSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    lightingSamplerInfo.mipLodBias = 0.0f;
    lightingSamplerInfo.minLod = 0.0f;
    lightingSamplerInfo.maxLod = 1.0f;

    if (vkCreateSampler(context.device, &lightingSamplerInfo, nullptr, &vk.placeholderLightingSampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create placeholder lighting sampler");
    }

    // Create placeholder settings buffer
    VkBufferCreateInfo settingsBufferInfo{};
    settingsBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    settingsBufferInfo.size = 256; // Enough for settings
    settingsBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    settingsBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(context.device, &settingsBufferInfo, nullptr, &vk.placeholderSettingsBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create placeholder settings buffer");
    }

    VkMemoryRequirements settingsMemRequirements;
    vkGetBufferMemoryRequirements(context.device, vk.placeholderSettingsBuffer, &settingsMemRequirements);

    VkMemoryAllocateInfo settingsAllocInfo{};
    settingsAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    settingsAllocInfo.allocationSize = settingsMemRequirements.size;
    settingsAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(context.physicalDevice, settingsMemRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(context.device, &settingsAllocInfo, nullptr, &vk.placeholderSettingsBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate placeholder settings buffer memory");
    }

    vkBindBufferMemory(context.device, vk.placeholderSettingsBuffer, vk.placeholderSettingsBufferMemory, 0);

    // Update graphics descriptor set with placeholder resources for bindings 8 and 9
    VkDescriptorImageInfo lightingImageInfo2{};
    lightingImageInfo2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    lightingImageInfo2.imageView = vk.placeholderLightingTextureView;
    lightingImageInfo2.sampler = vk.placeholderLightingSampler;
    
    VkWriteDescriptorSet lightingWrite{};
    lightingWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    lightingWrite.dstSet = vk.descriptorSet;
    lightingWrite.dstBinding = 8;
    lightingWrite.dstArrayElement = 0;
    lightingWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    lightingWrite.descriptorCount = 1;
    lightingWrite.pImageInfo = &lightingImageInfo2;

    vkUpdateDescriptorSets(context.device, 1, &lightingWrite, 0, nullptr);
    
    VkDescriptorBufferInfo settingsBufferInfo2{};
    settingsBufferInfo2.buffer = vk.placeholderSettingsBuffer;
    settingsBufferInfo2.offset = 0;
    settingsBufferInfo2.range = VK_WHOLE_SIZE;
    
    VkWriteDescriptorSet settingsWrite{};
    settingsWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    settingsWrite.dstSet = vk.descriptorSet;
    settingsWrite.dstBinding = 9;
    settingsWrite.dstArrayElement = 0;
    settingsWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    settingsWrite.descriptorCount = 1;
    settingsWrite.pBufferInfo = &settingsBufferInfo2;
    
    vkUpdateDescriptorSets(context.device, 1, &settingsWrite, 0, nullptr);
    
    // Pipeline layout for compute shader
    VkPipelineLayoutCreateInfo computePipelineLayoutInfo{};
    computePipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computePipelineLayoutInfo.setLayoutCount = 1;
    computePipelineLayoutInfo.pSetLayouts = &vk.computeDescriptorSetLayout;
    computePipelineLayoutInfo.pushConstantRangeCount = 1;
    
    VkPushConstantRange computePushConstantRange{};
    computePushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    computePushConstantRange.offset = 0;
    computePushConstantRange.size = sizeof(UniformBufferObject);
    computePipelineLayoutInfo.pPushConstantRanges = &computePushConstantRange;
    
    if (vkCreatePipelineLayout(context.device, &computePipelineLayoutInfo, nullptr, &vk.computePipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create compute pipeline layout");
    }
    computePipelineInfo.layout = vk.computePipelineLayout;
    if (vkCreateComputePipelines(context.device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &vk.computePipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create compute pipeline");
    }
    
    ShaderObject::DestroyShaderModule(context.device, computeShaderModule);
    // Load graphics shaders (no geometry shader)
    auto vertShaderCode = ShaderObject::Shader("unit.vert.spv");
    auto fragShaderCode = ShaderObject::Shader("unit.frag.spv");
    auto vertShaderModule = ShaderObject::CreateShaderModule(context.device, vertShaderCode);
    auto fragShaderModule = ShaderObject::CreateShaderModule(context.device, fragShaderCode);
    
    // Create shader stages
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule.module;
    vertShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule.module;
    fragShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo graphicsShaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    // Vertex input for output buffer (VertexOutput structure)
    VkVertexInputBindingDescription outputBindingDescription{};
    outputBindingDescription.binding = 0;
    outputBindingDescription.stride = sizeof(VertexOutput);
    outputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    std::array<VkVertexInputAttributeDescription, 9> outputAttributeDescriptions{};
    // Position (vec4)
    outputAttributeDescriptions[0].binding = 0;
    outputAttributeDescriptions[0].location = 0;
    outputAttributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    outputAttributeDescriptions[0].offset = offsetof(VertexOutput, position);
    // WorldPos (vec3)
    outputAttributeDescriptions[1].binding = 0;
    outputAttributeDescriptions[1].location = 1;
    outputAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    outputAttributeDescriptions[1].offset = offsetof(VertexOutput, worldPos);
    // TexCoords (vec2)
    outputAttributeDescriptions[2].binding = 0;
    outputAttributeDescriptions[2].location = 2;
    outputAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    outputAttributeDescriptions[2].offset = offsetof(VertexOutput, texCoords);
    // LightingEmit (uint)
    outputAttributeDescriptions[3].binding = 0;
    outputAttributeDescriptions[3].location = 3;
    outputAttributeDescriptions[3].format = VK_FORMAT_R32_UINT;
    outputAttributeDescriptions[3].offset = offsetof(VertexOutput, lightingEmit);
    // TransparencyLevel (uint)
    outputAttributeDescriptions[4].binding = 0;
    outputAttributeDescriptions[4].location = 4;
    outputAttributeDescriptions[4].format = VK_FORMAT_R32_UINT;
    outputAttributeDescriptions[4].offset = offsetof(VertexOutput, transparencyLevel);
    // FaceIndex (uint)
    outputAttributeDescriptions[5].binding = 0;
    outputAttributeDescriptions[5].location = 5;
    outputAttributeDescriptions[5].format = VK_FORMAT_R32_UINT;
    outputAttributeDescriptions[5].offset = offsetof(VertexOutput, faceIndex);
    // Albedo (vec3)
    outputAttributeDescriptions[6].binding = 0;
    outputAttributeDescriptions[6].location = 6;
    outputAttributeDescriptions[6].format = VK_FORMAT_R32G32B32_SFLOAT;
    outputAttributeDescriptions[6].offset = offsetof(VertexOutput, albedo);
    // Metallic (float)
    outputAttributeDescriptions[7].binding = 0;
    outputAttributeDescriptions[7].location = 7;
    outputAttributeDescriptions[7].format = VK_FORMAT_R32_SFLOAT;
    outputAttributeDescriptions[7].offset = offsetof(VertexOutput, metallic);
    // Roughness (float)
    outputAttributeDescriptions[8].binding = 0;
    outputAttributeDescriptions[8].location = 8;
    outputAttributeDescriptions[8].format = VK_FORMAT_R32_SFLOAT;
    outputAttributeDescriptions[8].offset = offsetof(VertexOutput, roughness);
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &outputBindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(outputAttributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = outputAttributeDescriptions.data();
    
    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    // Viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(context.swapChainExtent.width);
    viewport.height = static_cast<float>(context.swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = context.swapChainExtent;
    
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    // Depth stencil
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    
    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;
    
    // Graphics pipeline layout with push constants for vertex shader
    VkPipelineLayoutCreateInfo graphicsPipelineLayoutInfo{};
    graphicsPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    graphicsPipelineLayoutInfo.setLayoutCount = 1;
    graphicsPipelineLayoutInfo.pSetLayouts = &vk.descriptorSetLayout;
    graphicsPipelineLayoutInfo.pushConstantRangeCount = 1;
    
    VkPushConstantRange graphicsPushConstantRange{};
    graphicsPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    graphicsPushConstantRange.offset = 0;
    graphicsPushConstantRange.size = sizeof(UniformBufferObject);
    graphicsPipelineLayoutInfo.pPushConstantRanges = &graphicsPushConstantRange;
    
    if (vkCreatePipelineLayout(context.device, &graphicsPipelineLayoutInfo, nullptr, &vk.pipelineLayout) != VK_SUCCESS) {
        std::cerr << "Failed to create graphics pipeline layout" << std::endl;
        throw std::runtime_error("Failed to create graphics pipeline layout");
    }
    std::cerr << "Graphics pipeline layout created successfully" << std::endl;
    
    // Graphics pipeline (no geometry shader)
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2; // Only vertex and fragment
    pipelineInfo.pStages = graphicsShaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = vk.pipelineLayout;
    pipelineInfo.renderPass = context.renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    
    if (vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vk.pipeline) != VK_SUCCESS) {
        std::cerr << "Failed to create graphics pipeline" << std::endl;
        throw std::runtime_error("Failed to create graphics pipeline");
    }
    std::cerr << "Graphics pipeline created successfully" << std::endl;
    // Cleanup shader modules
    ShaderObject::DestroyShaderModule(context.device, vertShaderModule);
    ShaderObject::DestroyShaderModule(context.device, fragShaderModule);
}

void CameraPVMToFrustumPlanes(FrustumPlanes& frustum, World::Camera& cam) {
    glm::mat4 vp = cam.GetViewMatrix() * cam.GetProjectionMatrix();
    glm::mat4 m = glm::transpose(vp);
    frustum.planes[0] = m[3] + m[0];
    frustum.planes[1] = m[3] - m[0];
    frustum.planes[2] = m[3] - m[1];
    frustum.planes[3] = m[3] + m[1];
    frustum.planes[4] = m[2];
    frustum.planes[5] = m[3] - m[2];

    for (int i = 0; i < 6; ++i) {
        float length = glm::length(glm::vec3(frustum.planes[i]));
        if (length > 0.0f) {
            frustum.planes[i] /= length;
        }
    }
}

void UnitStateDrawable::OnUpdate(UnitStateResource& resource,
                                 UnitStateDrawableVulkan& vk,
                                 Game::VulkanContext& context)
{
    if (!resource.cameraModel)
        return;
    
    // Reset visible count to 0 for each frame
    uint32_t initialCount = 0;
    void* countData;
    vkMapMemory(context.device, vk.visibleCountBufferMemory, 0, sizeof(uint32_t), 0, &countData);
    memcpy(countData, &initialCount, sizeof(uint32_t));
    vkUnmapMemory(context.device, vk.visibleCountBufferMemory);
    
    FrustumPlanes frustum { };
    CameraPVMToFrustumPlanes(frustum, resource.cameraModel->GetObject());
    
    void* frustumData;
    vkMapMemory(context.device, vk.frustumBufferMemory, 0, sizeof(FrustumPlanes), 0, &frustumData);
    memcpy(frustumData, &frustum, sizeof(FrustumPlanes));
    vkUnmapMemory(context.device, vk.frustumBufferMemory);
    
    // Update graphics descriptor set with textures from unit
    if (resource.unit) {
        const auto& textures = resource.unit->GetMaterial();
        VkDescriptorImageInfo imageInfos[6] { };
        auto gen = [&imageInfos, &context](Texture2 *texture, const int index)
        {
            if (texture) {
                imageInfos[index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                texture->GetImageView(context);
                imageInfos[index].imageView = texture->binding.vkImageView;
                texture->GetSampler(context);
                imageInfos[index].sampler = texture->binding.vkSampler;
            }
        };
        // Get texture image views and samplers (order: top, down, left, right, front, back)
        gen(textures.top, 0);
        gen(textures.down, 1);
        gen(textures.left, 2);
        gen(textures.right, 3);
        gen(textures.front, 4);
        gen(textures.back, 5);
        
        VkWriteDescriptorSet textureWrite{};
        textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        textureWrite.dstSet = vk.descriptorSet;
        textureWrite.dstBinding = 2;
        textureWrite.dstArrayElement = 0;
        textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureWrite.descriptorCount = 6;
        textureWrite.pImageInfo = imageInfos;
        vkUpdateDescriptorSets(context.device, 1, &textureWrite, 0, nullptr);
    }
}

void UnitStateDrawable::OnDraw(UnitStateResource& resource,
                               UnitStateDrawableVulkan& vk,
                               Game::VulkanContext& context)
{
    if (!resource.cameraModel)
        return;
    // Get camera matrices for push constants
    UniformBufferObject ubo{};
    World::Camera& cam = resource.cameraModel->GetObject();
    ubo.model = cam.GetModelMatrix();
    ubo.view = cam.GetViewMatrix();
    ubo.projection = cam.GetProjectionMatrix();
    
    // Bind graphics pipeline
    if (vk.pipeline != VK_NULL_HANDLE && vk.pipelineLayout != VK_NULL_HANDLE) {
        vkCmdBindPipeline(context.commandBuffers[0], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline);
    
        // Bind output vertex buffer (culled vertices from compute shader)
        VkBuffer vertexBuffers[] = {vk.outputVertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(context.commandBuffers[0], 0, 1, vertexBuffers, offsets);
        
        // Bind graphics descriptor set for textures
        vkCmdBindDescriptorSets(context.commandBuffers[0], VK_PIPELINE_BIND_POINT_GRAPHICS,
                               vk.pipelineLayout, 0, 1, &vk.descriptorSet, 0, nullptr);
        vkCmdPushConstants(context.commandBuffers[0], vk.pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UniformBufferObject), &ubo);
        // Draw using indirect draw parameters from compute shader
        vkCmdDrawIndirect(context.commandBuffers[0], vk.indirectDrawBuffer, 0, 1, 0);
    }
}

void UnitStateDrawable::OnDestroy(UnitStateResource& resource,
                                  UnitStateDrawableVulkan& vk,
                                  Game::VulkanContext& context)
{
    vkDestroyBuffer(context.device, vk.vertexBuffer, nullptr);
    vkFreeMemory(context.device, vk.vertexBufferMemory, nullptr);
    vkDestroyBuffer(context.device, vk.outputVertexBuffer, nullptr);
    vkFreeMemory(context.device, vk.outputVertexBufferMemory, nullptr);
    vkDestroyBuffer(context.device, vk.visibleCountBuffer, nullptr);
    vkFreeMemory(context.device, vk.visibleCountBufferMemory, nullptr);
    vkDestroyBuffer(context.device, vk.indirectDrawBuffer, nullptr);
    vkFreeMemory(context.device, vk.indirectDrawBufferMemory, nullptr);
    vkDestroyBuffer(context.device, vk.frustumBuffer, nullptr);
    vkFreeMemory(context.device, vk.frustumBufferMemory, nullptr);
    vkDestroySampler(context.device, vk.placeholderLightingSampler, nullptr);
    vkDestroyImageView(context.device, vk.placeholderLightingTextureView, nullptr);
    vkDestroyImage(context.device, vk.placeholderLightingTexture, nullptr);
    vkFreeMemory(context.device, vk.placeholderLightingTextureMemory, nullptr);
    vkDestroyBuffer(context.device, vk.placeholderSettingsBuffer, nullptr);
    vkFreeMemory(context.device, vk.placeholderSettingsBufferMemory, nullptr);
    vkDestroyPipeline(context.device, vk.computePipeline, nullptr);
    vkDestroyPipelineLayout(context.device, vk.computePipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(context.device, vk.computeDescriptorSetLayout, nullptr);
    vkDestroyPipeline(context.device, vk.pipeline, nullptr);
    vkDestroyPipelineLayout(context.device, vk.pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(context.device, vk.descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(context.device, vk.descriptorPool, nullptr);
}

void UnitStateDrawable::OnPause()
{
}

void UnitStateDrawable::OnResume()
{
}

} // namespace Rl::Providers
