#include <vector>
#include <stdexcept>

#include "rl/Client/State/UnitState.h"
#include "rl/World/Unit/Unit.h"
#include "rl/Base/ShaderFactory.h"
#include "rl/Base/Texture2.h"
#include "rl/Base/Game.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>

namespace Rl::Providers {

// Vertex structure for Unit rendering (must match compute shader VertexInput with std430 layout)
struct UnitVertex {
    glm::vec4 position;     // vec4 in shader (x, y, z, w) local position
    glm::vec4 polRight;      // Polygon fence right offsets (16-byte aligned)
    glm::vec4 polLeft;       // Polygon fence left offsets (16-byte aligned)
    glm::vec2 texCoords;
    uint32_t lightingEmit;
    uint32_t transparencyLevel;
    uint32_t faceIndex;
    float albR, albG, albB;
    float metallic;
    float roughness;
    float tanX, tanY, tanZ;
    float bitanX, bitanY, bitanZ;
    float normX, normY, normZ; // Geometric normal for smooth rim lighting
};

// Unit cube vertices (6 faces, 2 triangles each, 3 vertices per triangle)
// Varied material properties per face for realistic PBR lighting testing
// Smooth normals at edges for better rim lighting interpolation
static const std::vector<UnitVertex> unitVertices = {
    // Top face (faceIndex = 0) - High metallic, low roughness (shiny metal)
    {glm::vec4(-0.5f, 0.5f, -0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(0.0f, 0.0f), 0, 0, 0, 0.8f, 0.9f, 0.7f, 0.9f, 0.1f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, -0.58f, 0.58f, -0.58f},
    {glm::vec4(-0.5f, 0.5f,  0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(0.0f, 1.0f), 0, 0, 0, 0.8f, 0.9f, 0.7f, 0.9f, 0.1f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, -0.58f, 0.58f, 0.58f},
    {glm::vec4( 0.5f, 0.5f,  0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(1.0f, 1.0f), 0, 0, 0, 0.8f, 0.9f, 0.7f, 0.9f, 0.1f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.58f, 0.58f, 0.58f},
    {glm::vec4(-0.5f, 0.5f, -0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(0.0f, 0.0f), 0, 0, 0, 0.8f, 0.9f, 0.7f, 0.9f, 0.1f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, -0.58f, 0.58f, -0.58f},
    {glm::vec4( 0.5f, 0.5f,  0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(1.0f, 1.0f), 0, 0, 0, 0.8f, 0.9f, 0.7f, 0.9f, 0.1f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.58f, 0.58f, 0.58f},
    {glm::vec4( 0.5f, 0.5f, -0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(1.0f, 0.0f), 0, 0, 0, 0.8f, 0.9f, 0.7f, 0.9f, 0.1f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.58f, 0.58f, -0.58f},

    // Bottom face (faceIndex = 1) - Low metallic, high roughness (matte)
    {glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(0.0f, 0.0f), 0, 0, 1, 0.5f, 0.5f, 0.5f, 0.0f, 0.9f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, -0.58f, -0.58f, -0.58f},
    {glm::vec4( 0.5f, -0.5f, -0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(1.0f, 0.0f), 0, 0, 1, 0.5f, 0.5f, 0.5f, 0.0f, 0.9f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.58f, -0.58f, -0.58f},
    {glm::vec4( 0.5f, -0.5f,  0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(1.0f, 1.0f), 0, 0, 1, 0.5f, 0.5f, 0.5f, 0.0f, 0.9f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.58f, -0.58f, 0.58f},
    {glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(0.0f, 0.0f), 0, 0, 1, 0.5f, 0.5f, 0.5f, 0.0f, 0.9f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, -0.58f, -0.58f, -0.58f},
    {glm::vec4( 0.5f, -0.5f,  0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(1.0f, 1.0f), 0, 0, 1, 0.5f, 0.5f, 0.5f, 0.0f, 0.9f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.58f, -0.58f, 0.58f},
    {glm::vec4(-0.5f, -0.5f,  0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(0.0f, 1.0f), 0, 0, 1, 0.5f, 0.5f, 0.5f, 0.0f, 0.9f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, -0.58f, -0.58f, 0.58f},

    // Left face (faceIndex = 2) - Medium metallic, medium roughness (plastic-like)
    {glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(0.0f, 1.0f), 0, 0, 2, 0.6f, 0.4f, 0.8f, 0.3f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, -0.58f, -0.58f, -0.58f},
    {glm::vec4(-0.5f, -0.5f,  0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(0.0f, 0.0f), 0, 0, 2, 0.6f, 0.4f, 0.8f, 0.3f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, -0.58f, -0.58f, 0.58f},
    {glm::vec4(-0.5f,  0.5f,  0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(1.0f, 0.0f), 0, 0, 2, 0.6f, 0.4f, 0.8f, 0.3f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, -0.58f, 0.58f, 0.58f},

    {glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(0.0f, 1.0f), 0, 0, 2, 0.6f, 0.4f, 0.8f, 0.3f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, -0.58f, -0.58f, -0.58f},
    {glm::vec4(-0.5f,  0.5f,  0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(1.0f, 0.0f), 0, 0, 2, 0.6f, 0.4f, 0.8f, 0.3f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, -0.58f, 0.58f, 0.58f},
    {glm::vec4(-0.5f,  0.5f, -0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(1.0f, 1.0f), 0, 0, 2, 0.6f, 0.4f, 0.8f, 0.3f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, -0.58f, 0.58f, -0.58f},

    // Right face (faceIndex = 3) - High metallic, medium roughness (brushed metal)
    {glm::vec4( 0.5f,  0.5f,  0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(1.0f, 0.0f), 0, 0, 3, 0.7f, 0.7f, 0.6f, 0.8f, 0.4f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.58f, 0.58f, 0.58f},
    {glm::vec4( 0.5f, -0.5f, -0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(0.0f, 1.0f), 0, 0, 3, 0.7f, 0.7f, 0.6f, 0.8f, 0.4f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.58f, -0.58f, -0.58f},
    {glm::vec4( 0.5f,  0.5f, -0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(1.0f, 1.0f), 0, 0, 3, 0.7f, 0.7f, 0.6f, 0.8f, 0.4f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.58f, 0.58f, -0.58f},
    {glm::vec4( 0.5f, -0.5f, -0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(0.0f, 1.0f), 0, 0, 3, 0.7f, 0.7f, 0.6f, 0.8f, 0.4f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.58f, -0.58f, -0.58f},
    {glm::vec4( 0.5f,  0.5f,  0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(1.0f, 0.0f), 0, 0, 3, 0.7f, 0.7f, 0.6f, 0.8f, 0.4f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.58f, 0.58f, 0.58f},
    {glm::vec4( 0.5f, -0.5f,  0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(0.0f, 0.0f), 0, 0, 3, 0.7f, 0.7f, 0.6f, 0.8f, 0.4f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.58f, -0.58f, 0.58f},

    // Front face (faceIndex = 4) - Low metallic, low roughness (shiny plastic)
    {glm::vec4(-0.5f, -0.5f,  0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(0.0f, 1.0f), 0, 0, 4, 0.9f, 0.8f, 0.9f, 0.1f, 0.2f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, -0.58f, -0.58f, 0.58f},
    {glm::vec4( 0.5f, -0.5f,  0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(1.0f, 1.0f), 0, 0, 4, 0.9f, 0.8f, 0.9f, 0.1f, 0.2f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.58f, -0.58f, 0.58f},
    {glm::vec4( 0.5f,  0.5f,  0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(1.0f, 0.0f), 0, 0, 4, 0.9f, 0.8f, 0.9f, 0.1f, 0.2f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.58f, 0.58f, 0.58f},
    {glm::vec4( 0.5f,  0.5f,  0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(1.0f, 0.0f), 0, 0, 4, 0.9f, 0.8f, 0.9f, 0.1f, 0.2f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.58f, 0.58f, 0.58f},
    {glm::vec4(-0.5f,  0.5f,  0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(0.0f, 0.0f), 0, 0, 4, 0.9f, 0.8f, 0.9f, 0.1f, 0.2f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, -0.58f, 0.58f, 0.58f},
    {glm::vec4(-0.5f, -0.5f,  0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(0.0f, 1.0f), 0, 0, 4, 0.9f, 0.8f, 0.9f, 0.1f, 0.2f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, -0.58f, -0.58f, 0.58f},

    // Back face (faceIndex = 5) - Medium metallic, high roughness (ceramic-like)
    {glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(0.0f, 1.0f), 0, 0, 5, 0.4f, 0.6f, 0.5f, 0.4f, 0.7f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, -0.58f, -0.58f, -0.58f},
    {glm::vec4(-0.5f,  0.5f, -0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(0.0f, 0.0f), 0, 0, 5, 0.4f, 0.6f, 0.5f, 0.4f, 0.7f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, -0.58f, 0.58f, -0.58f},
    {glm::vec4( 0.5f,  0.5f, -0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(1.0f, 0.0f), 0, 0, 5, 0.4f, 0.6f, 0.5f, 0.4f, 0.7f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.58f, 0.58f, -0.58f},
    {glm::vec4( 0.5f,  0.5f, -0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(1.0f, 0.0f), 0, 0, 5, 0.4f, 0.6f, 0.5f, 0.4f, 0.7f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.58f, 0.58f, -0.58f},
    {glm::vec4( 0.5f, -0.5f, -0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(1.0f, 1.0f), 0, 0, 5, 0.4f, 0.6f, 0.5f, 0.4f, 0.7f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.58f, -0.58f, -0.58f},
    {glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f), glm::vec4(0,0,0,0), glm::vec4(0,0,0,0), glm::vec2(0.0f, 1.0f), 0, 0, 5, 0.4f, 0.6f, 0.5f, 0.4f, 0.7f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, -0.58f, -0.58f, -0.58f},
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

// Triplanar mapping settings
struct TriplanarSettings {
    alignas(4) float scale;
    alignas(4) float sharpness;
};

// Frustum planes for culling
struct FrustumPlanes {
    glm::vec4 planes[6];
};

// Vertex output structure matching compute shader
struct VertexOutput {
    glm::vec4 position;
    glm::vec4 worldPosAndUV;
    glm::vec4 uvAndLighting;
    glm::vec4 material;
    glm::vec4 roughnessAndTan;
    glm::vec4 bitangent;
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
    countBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
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
    indirectBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
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
    
    // Initialize indirect draw buffer with default values (draw all vertices)
    DrawParams initialDrawParams{};
    initialDrawParams.vertexCount = static_cast<uint32_t>(unitVertices.size());
    initialDrawParams.instanceCount = 1;
    initialDrawParams.firstVertex = 0;
    initialDrawParams.firstInstance = 0;
    
    void* indirectData;
    vkMapMemory(context.device, vk.indirectDrawBufferMemory, 0, sizeof(DrawParams), 0, &indirectData);
    memcpy(indirectData, &initialDrawParams, sizeof(DrawParams));
    vkUnmapMemory(context.device, vk.indirectDrawBufferMemory);
    
    // Create frustum planes uniform buffer
    VkBufferCreateInfo frustumBufferInfo{};
    frustumBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    frustumBufferInfo.size = sizeof(FrustumPlanes);
    frustumBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
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
    
    // Create triplanar settings uniform buffer
    VkBufferCreateInfo triplanarBufferInfo{};
    triplanarBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    triplanarBufferInfo.size = sizeof(TriplanarSettings);
    triplanarBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    triplanarBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(context.device, &triplanarBufferInfo, nullptr, &vk.triplanarSettingsBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create triplanar settings buffer");
    }
    
    VkMemoryRequirements triplanarMemRequirements;
    vkGetBufferMemoryRequirements(context.device, vk.triplanarSettingsBuffer, &triplanarMemRequirements);
    
    VkMemoryAllocateInfo triplanarAllocInfo{};
    triplanarAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    triplanarAllocInfo.allocationSize = triplanarMemRequirements.size;
    triplanarAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(context.physicalDevice, triplanarMemRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(context.device, &triplanarAllocInfo, nullptr, &vk.triplanarSettingsBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate triplanar settings buffer memory");
    }
    
    vkBindBufferMemory(context.device, vk.triplanarSettingsBuffer, vk.triplanarSettingsBufferMemory, 0);
    
    // Initialize triplanar settings with default values
    TriplanarSettings initialTriplanar{};
    initialTriplanar.scale = 1.0f;
    initialTriplanar.sharpness = 2.0f;
    
    void* triplanarData;
    vkMapMemory(context.device, vk.triplanarSettingsBufferMemory, 0, sizeof(TriplanarSettings), 0, &triplanarData);
    memcpy(triplanarData, &initialTriplanar, sizeof(TriplanarSettings));
    vkUnmapMemory(context.device, vk.triplanarSettingsBufferMemory);
    
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
    VkDescriptorSetLayoutBinding graphicsBindings[7]{};
    // Texture array (binding 2)
    graphicsBindings[0].binding = 2;
    graphicsBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    graphicsBindings[0].descriptorCount = 6; // 6 textures for 6 faces
    graphicsBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // Lighting block uniform buffer (binding 4)
    graphicsBindings[1].binding = 4;
    graphicsBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    graphicsBindings[1].descriptorCount = 1;
    graphicsBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // Lighting texture (binding 8)
    graphicsBindings[2].binding = 8;
    graphicsBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    graphicsBindings[2].descriptorCount = 1;
    graphicsBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // Settings uniform buffer (binding 9)
    graphicsBindings[3].binding = 9;
    graphicsBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    graphicsBindings[3].descriptorCount = 1;
    graphicsBindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // AO texture (binding 10)
    graphicsBindings[4].binding = 10;
    graphicsBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    graphicsBindings[4].descriptorCount = 1;
    graphicsBindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // Normal texture (binding 11)
    graphicsBindings[5].binding = 11;
    graphicsBindings[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    graphicsBindings[5].descriptorCount = 1;
    graphicsBindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // Triplanar settings uniform buffer (binding 12)
    graphicsBindings[6].binding = 12;
    graphicsBindings[6].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    graphicsBindings[6].descriptorCount = 1;
    graphicsBindings[6].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo graphicsLayoutInfo{};
    graphicsLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    graphicsLayoutInfo.bindingCount = 7;
    graphicsLayoutInfo.pBindings = graphicsBindings;
    
    if (vkCreateDescriptorSetLayout(context.device, &graphicsLayoutInfo, nullptr, &vk.descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics descriptor set layout");
    }
    
    // Create descriptor pool
    VkDescriptorPoolSize poolSizes[3]{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[0].descriptorCount = 4; // 4 SSBOs for compute
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 9; // 6 textures + 1 lighting texture + 1 AO texture + 1 normal texture
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[2].descriptorCount = 6; // 1 frustum + 1 lighting block + 1 settings + 1 triplanar + 2 extra
    
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

    // Create global texture sampler (for all textures to avoid sampler allocation limit)
    VkSamplerCreateInfo globalSamplerInfo{};
    globalSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    globalSamplerInfo.magFilter = VK_FILTER_LINEAR;
    globalSamplerInfo.minFilter = VK_FILTER_LINEAR;
    globalSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    globalSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    globalSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    globalSamplerInfo.anisotropyEnable = VK_FALSE;
    globalSamplerInfo.maxAnisotropy = 1.0f;
    globalSamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    globalSamplerInfo.unnormalizedCoordinates = VK_FALSE;
    globalSamplerInfo.compareEnable = VK_FALSE;
    globalSamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    globalSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    globalSamplerInfo.mipLodBias = 0.0f;
    globalSamplerInfo.minLod = 0.0f;
    globalSamplerInfo.maxLod = 16.0f;

    if (vkCreateSampler(context.device, &globalSamplerInfo, nullptr, &vk.globalTextureSampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create global texture sampler");
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
    
    // Initialize settings buffer with lighting pass disabled
    bool useLightingPass = false;
    void* settingsData;
    vkMapMemory(context.device, vk.placeholderSettingsBufferMemory, 0, sizeof(bool), 0, &settingsData);
    memcpy(settingsData, &useLightingPass, sizeof(bool));
    vkUnmapMemory(context.device, vk.placeholderSettingsBufferMemory);
    
    // Create lighting uniform buffer for PBR shader
    struct LightingBlock {
        alignas(16) glm::vec3 sunDirection;
        alignas(16) glm::vec3 sunColor;
        alignas(4)  float ambientStrength;
        alignas(16) glm::vec3 cameraPosition;
    };
    
    // Ensure buffer size matches the aligned struct size
    constexpr VkDeviceSize lightingBufferSize = sizeof(LightingBlock);
    VkBufferCreateInfo lightingBufferInfo{};
    lightingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    lightingBufferInfo.size = lightingBufferSize;
    lightingBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    lightingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(context.device, &lightingBufferInfo, nullptr, &vk.placeholderLightingBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create lighting buffer");
    }
    
    VkMemoryRequirements lightingMemRequirements2;
    vkGetBufferMemoryRequirements(context.device, vk.placeholderLightingBuffer, &lightingMemRequirements);
    
    VkMemoryAllocateInfo lightingAllocInfo2{};
    lightingAllocInfo2.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    lightingAllocInfo2.allocationSize = lightingMemRequirements.size;
    lightingAllocInfo2.memoryTypeIndex = FindMemoryTypeIndex(context.physicalDevice, lightingMemRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(context.device, &lightingAllocInfo2, nullptr, &vk.placeholderLightingBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate lighting buffer memory");
    }
    
    vkBindBufferMemory(context.device, vk.placeholderLightingBuffer, vk.placeholderLightingBufferMemory, 0);
    
    // Initialize lighting buffer with default values
    LightingBlock lightingData{};
    lightingData.sunDirection = glm::normalize(glm::vec3(0.5f, 0.8f, 0.6f));
    lightingData.sunColor = glm::vec3(1.0f, 1.0f, 1.0f);
    lightingData.ambientStrength = 0.2f;
    lightingData.cameraPosition = glm::vec3(0.0f, 0.0f, 15.0f);
    
    void* lightingBufferData;
    vkMapMemory(context.device, vk.placeholderLightingBufferMemory, 0, lightingBufferSize, 0, &lightingBufferData);
    memcpy(lightingBufferData, &lightingData, lightingBufferSize);
    vkUnmapMemory(context.device, vk.placeholderLightingBufferMemory);
    
    // Create placeholder AO texture (1x1 white texture)
    VkImageCreateInfo aoImageInfo{};
    aoImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    aoImageInfo.imageType = VK_IMAGE_TYPE_2D;
    aoImageInfo.extent.width = 1;
    aoImageInfo.extent.height = 1;
    aoImageInfo.extent.depth = 1;
    aoImageInfo.mipLevels = 1;
    aoImageInfo.arrayLayers = 1;
    aoImageInfo.format = VK_FORMAT_R8_UNORM;
    aoImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    aoImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    aoImageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    aoImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    aoImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateImage(context.device, &aoImageInfo, nullptr, &vk.placeholderAOTexture) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create AO texture");
    }
    
    VkMemoryRequirements aoMemRequirements;
    vkGetImageMemoryRequirements(context.device, vk.placeholderAOTexture, &aoMemRequirements);
    
    VkMemoryAllocateInfo aoAllocInfo{};
    aoAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    aoAllocInfo.allocationSize = aoMemRequirements.size;
    aoAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(context.physicalDevice, aoMemRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (vkAllocateMemory(context.device, &aoAllocInfo, nullptr, &vk.placeholderAOTextureMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate AO texture memory");
    }
    
    vkBindImageMemory(context.device, vk.placeholderAOTexture, vk.placeholderAOTextureMemory, 0);
    
    // Fill AO texture with white (no occlusion)
    VkBuffer aoStagingBuffer;
    VkDeviceMemory aoStagingBufferMemory;
    VkBufferCreateInfo aoStagingBufferInfo{};
    aoStagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    aoStagingBufferInfo.size = 1; // 1 byte for 1x1 R8 texture
    aoStagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    aoStagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(context.device, &aoStagingBufferInfo, nullptr, &aoStagingBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create AO staging buffer");
    }
    
    VkMemoryRequirements aoStagingMemRequirements;
    vkGetBufferMemoryRequirements(context.device, aoStagingBuffer, &aoStagingMemRequirements);
    
    VkMemoryAllocateInfo aoStagingAllocInfo{};
    aoStagingAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    aoStagingAllocInfo.allocationSize = aoStagingMemRequirements.size;
    aoStagingAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(context.physicalDevice, aoStagingMemRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(context.device, &aoStagingAllocInfo, nullptr, &aoStagingBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate AO staging buffer memory");
    }
    
    vkBindBufferMemory(context.device, aoStagingBuffer, aoStagingBufferMemory, 0);
    
    // Fill staging buffer with white (255)
    uint8_t aoData = 255;
    void* aoStagingData;
    vkMapMemory(context.device, aoStagingBufferMemory, 0, sizeof(uint8_t), 0, &aoStagingData);
    memcpy(aoStagingData, &aoData, sizeof(uint8_t));
    vkUnmapMemory(context.device, aoStagingBufferMemory);
    
    // Copy staging buffer to AO texture
    VkCommandPool aoCopyCommandPool;
    VkCommandPoolCreateInfo aoCopyPoolInfo{};
    aoCopyPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    aoCopyPoolInfo.queueFamilyIndex = context.queueFamilyIndices.graphicsFamily.value();
    
    if (vkCreateCommandPool(context.device, &aoCopyPoolInfo, nullptr, &aoCopyCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool for AO texture copy");
    }
    
    VkCommandBuffer aoCopyCommandBuffer;
    VkCommandBufferAllocateInfo aoCopyAllocInfo{};
    aoCopyAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    aoCopyAllocInfo.commandPool = aoCopyCommandPool;
    aoCopyAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    aoCopyAllocInfo.commandBufferCount = 1;
    
    vkAllocateCommandBuffers(context.device, &aoCopyAllocInfo, &aoCopyCommandBuffer);
    
    VkCommandBufferBeginInfo aoCopyBeginInfo{};
    aoCopyBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    aoCopyBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(aoCopyCommandBuffer, &aoCopyBeginInfo);
    
    // Transition AO texture to transfer dst
    VkImageMemoryBarrier aoCopyBarrier1{};
    aoCopyBarrier1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    aoCopyBarrier1.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    aoCopyBarrier1.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    aoCopyBarrier1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    aoCopyBarrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    aoCopyBarrier1.image = vk.placeholderAOTexture;
    aoCopyBarrier1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    aoCopyBarrier1.subresourceRange.baseMipLevel = 0;
    aoCopyBarrier1.subresourceRange.levelCount = 1;
    aoCopyBarrier1.subresourceRange.baseArrayLayer = 0;
    aoCopyBarrier1.subresourceRange.layerCount = 1;
    aoCopyBarrier1.srcAccessMask = 0;
    aoCopyBarrier1.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    
    vkCmdPipelineBarrier(aoCopyCommandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &aoCopyBarrier1);
    
    // Copy buffer to image
    VkBufferImageCopy aoRegion{};
    aoRegion.bufferOffset = 0;
    aoRegion.bufferRowLength = 0;
    aoRegion.bufferImageHeight = 0;
    aoRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    aoRegion.imageSubresource.mipLevel = 0;
    aoRegion.imageSubresource.baseArrayLayer = 0;
    aoRegion.imageSubresource.layerCount = 1;
    aoRegion.imageOffset = {0, 0, 0};
    aoRegion.imageExtent = {1, 1, 1};
    
    vkCmdCopyBufferToImage(aoCopyCommandBuffer, aoStagingBuffer, vk.placeholderAOTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &aoRegion);
    
    // Transition AO texture to shader read only
    VkImageMemoryBarrier aoCopyBarrier2{};
    aoCopyBarrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    aoCopyBarrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    aoCopyBarrier2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    aoCopyBarrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    aoCopyBarrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    aoCopyBarrier2.image = vk.placeholderAOTexture;
    aoCopyBarrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    aoCopyBarrier2.subresourceRange.baseMipLevel = 0;
    aoCopyBarrier2.subresourceRange.levelCount = 1;
    aoCopyBarrier2.subresourceRange.baseArrayLayer = 0;
    aoCopyBarrier2.subresourceRange.layerCount = 1;
    aoCopyBarrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    aoCopyBarrier2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(aoCopyCommandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &aoCopyBarrier2);
    
    vkEndCommandBuffer(aoCopyCommandBuffer);
    
    VkSubmitInfo aoCopySubmitInfo{};
    aoCopySubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    aoCopySubmitInfo.commandBufferCount = 1;
    aoCopySubmitInfo.pCommandBuffers = &aoCopyCommandBuffer;
    
    vkQueueSubmit(context.graphicsQueue, 1, &aoCopySubmitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(context.graphicsQueue);
    
    vkFreeCommandBuffers(context.device, aoCopyCommandPool, 1, &aoCopyCommandBuffer);
    vkDestroyCommandPool(context.device, aoCopyCommandPool, nullptr);
    
    vkDestroyBuffer(context.device, aoStagingBuffer, nullptr);
    vkFreeMemory(context.device, aoStagingBufferMemory, nullptr);
    
    // Create AO texture view
    VkImageViewCreateInfo aoViewInfo{};
    aoViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    aoViewInfo.image = vk.placeholderAOTexture;
    aoViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    aoViewInfo.format = VK_FORMAT_R8_UNORM;
    aoViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    aoViewInfo.subresourceRange.baseMipLevel = 0;
    aoViewInfo.subresourceRange.levelCount = 1;
    aoViewInfo.subresourceRange.baseArrayLayer = 0;
    aoViewInfo.subresourceRange.layerCount = 1;
    
    if (vkCreateImageView(context.device, &aoViewInfo, nullptr, &vk.placeholderAOTextureView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create AO texture view");
    }
    
    // Create AO sampler
    VkSamplerCreateInfo aoSamplerInfo{};
    aoSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    aoSamplerInfo.magFilter = VK_FILTER_LINEAR;
    aoSamplerInfo.minFilter = VK_FILTER_LINEAR;
    aoSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    aoSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    aoSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    aoSamplerInfo.anisotropyEnable = VK_FALSE;
    aoSamplerInfo.maxAnisotropy = 1.0f;
    aoSamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    aoSamplerInfo.unnormalizedCoordinates = VK_FALSE;
    aoSamplerInfo.compareEnable = VK_FALSE;
    aoSamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    aoSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    aoSamplerInfo.mipLodBias = 0.0f;
    aoSamplerInfo.minLod = 0.0f;
    aoSamplerInfo.maxLod = 1.0f;
    
    if (vkCreateSampler(context.device, &aoSamplerInfo, nullptr, &vk.placeholderAOSampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create AO sampler");
    }
    
    // Transition AO texture layout
    VkCommandPool aoTempCommandPool;
    VkCommandPoolCreateInfo aoPoolInfo{};
    aoPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    aoPoolInfo.queueFamilyIndex = context.queueFamilyIndices.graphicsFamily.value();
    
    if (vkCreateCommandPool(context.device, &aoPoolInfo, nullptr, &aoTempCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create temporary command pool for AO texture");
    }
    
    VkCommandBuffer aoTransitionCommandBuffer;
    VkCommandBufferAllocateInfo aoTransitionAllocInfo{};
    aoTransitionAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    aoTransitionAllocInfo.commandPool = aoTempCommandPool;
    aoTransitionAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    aoTransitionAllocInfo.commandBufferCount = 1;
    
    vkAllocateCommandBuffers(context.device, &aoTransitionAllocInfo, &aoTransitionCommandBuffer);
    
    VkCommandBufferBeginInfo aoTransitionBeginInfo{};
    aoTransitionBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    aoTransitionBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(aoTransitionCommandBuffer, &aoTransitionBeginInfo);
    
    VkImageMemoryBarrier aoTransitionBarrier{};
    aoTransitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    aoTransitionBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    aoTransitionBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    aoTransitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    aoTransitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    aoTransitionBarrier.image = vk.placeholderAOTexture;
    aoTransitionBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    aoTransitionBarrier.subresourceRange.baseMipLevel = 0;
    aoTransitionBarrier.subresourceRange.levelCount = 1;
    aoTransitionBarrier.subresourceRange.baseArrayLayer = 0;
    aoTransitionBarrier.subresourceRange.layerCount = 1;
    aoTransitionBarrier.srcAccessMask = 0;
    aoTransitionBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(aoTransitionCommandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &aoTransitionBarrier);
    
    vkEndCommandBuffer(aoTransitionCommandBuffer);
    
    VkSubmitInfo aoTransitionSubmitInfo{};
    aoTransitionSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    aoTransitionSubmitInfo.commandBufferCount = 1;
    aoTransitionSubmitInfo.pCommandBuffers = &aoTransitionCommandBuffer;
    
    vkQueueSubmit(context.graphicsQueue, 1, &aoTransitionSubmitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(context.graphicsQueue);
    
    vkFreeCommandBuffers(context.device, aoTempCommandPool, 1, &aoTransitionCommandBuffer);
    vkDestroyCommandPool(context.device, aoTempCommandPool, nullptr);

    // Update graphics descriptor set with placeholder resources for bindings 4, 8, 9, and 10
    VkDescriptorBufferInfo lightingBufferInfo2{};
    lightingBufferInfo2.buffer = vk.placeholderLightingBuffer;
    lightingBufferInfo2.offset = 0;
    lightingBufferInfo2.range = sizeof(LightingBlock);
    
    VkWriteDescriptorSet lightingBufferWrite{};
    lightingBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    lightingBufferWrite.dstSet = vk.descriptorSet;
    lightingBufferWrite.dstBinding = 4;
    lightingBufferWrite.dstArrayElement = 0;
    lightingBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightingBufferWrite.descriptorCount = 1;
    lightingBufferWrite.pBufferInfo = &lightingBufferInfo2;
    
    VkDescriptorImageInfo lightingImageInfo2{};
    lightingImageInfo2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    lightingImageInfo2.imageView = vk.placeholderLightingTextureView;
    lightingImageInfo2.sampler = vk.placeholderLightingSampler;
    
    VkWriteDescriptorSet lightingTextureWrite{};
    lightingTextureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    lightingTextureWrite.dstSet = vk.descriptorSet;
    lightingTextureWrite.dstBinding = 8;
    lightingTextureWrite.dstArrayElement = 0;
    lightingTextureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    lightingTextureWrite.descriptorCount = 1;
    lightingTextureWrite.pImageInfo = &lightingImageInfo2;
    
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
    
    VkDescriptorImageInfo aoImageInfo2{};
    aoImageInfo2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    aoImageInfo2.imageView = vk.placeholderAOTextureView;
    aoImageInfo2.sampler = vk.placeholderAOSampler;
    
    VkWriteDescriptorSet aoTextureWrite{};
    aoTextureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    aoTextureWrite.dstSet = vk.descriptorSet;
    aoTextureWrite.dstBinding = 10;
    aoTextureWrite.dstArrayElement = 0;
    aoTextureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    aoTextureWrite.descriptorCount = 1;
    aoTextureWrite.pImageInfo = &aoImageInfo2;
    
    // Triplanar settings buffer descriptor (binding 12)
    VkDescriptorBufferInfo triplanarBufferInfo2{};
    triplanarBufferInfo2.buffer = vk.triplanarSettingsBuffer;
    triplanarBufferInfo2.offset = 0;
    triplanarBufferInfo2.range = sizeof(TriplanarSettings);
    
    VkWriteDescriptorSet triplanarSettingsWrite{};
    triplanarSettingsWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    triplanarSettingsWrite.dstSet = vk.descriptorSet;
    triplanarSettingsWrite.dstBinding = 12;
    triplanarSettingsWrite.dstArrayElement = 0;
    triplanarSettingsWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    triplanarSettingsWrite.descriptorCount = 1;
    triplanarSettingsWrite.pBufferInfo = &triplanarBufferInfo2;
    
    std::array<VkWriteDescriptorSet, 5> descriptorWrites = {
        lightingBufferWrite,
        lightingTextureWrite,
        settingsWrite,
        aoTextureWrite,
        triplanarSettingsWrite
    };
    
    vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    
    // Pipeline layout for compute shader
    VkPipelineLayoutCreateInfo computePipelineLayoutInfo{};
    computePipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computePipelineLayoutInfo.setLayoutCount = 1;
    computePipelineLayoutInfo.pSetLayouts = &vk.computeDescriptorSetLayout;
    computePipelineLayoutInfo.pushConstantRangeCount = 1;
    
    VkPushConstantRange computePushConstantRange{};
    computePushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
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
    auto fragShaderCode = ShaderObject::Shader("unit.lightning.frag.spv");
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
    // Vertex input for output buffer (VertexOutput structure from compute shader)
    VkVertexInputBindingDescription inputBindingDescription{};
    inputBindingDescription.binding = 0;
    inputBindingDescription.stride = sizeof(VertexOutput);
    inputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    std::array<VkVertexInputAttributeDescription, 6> inputAttributeDescriptions{};
    // Position (vec4) - Clip space from compute shader
    inputAttributeDescriptions[0].binding = 0;
    inputAttributeDescriptions[0].location = 0;
    inputAttributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    inputAttributeDescriptions[0].offset = offsetof(VertexOutput, position);
    // WorldPosAndUV (vec4) - worldX, worldY, worldZ, texCoords.x
    inputAttributeDescriptions[1].binding = 0;
    inputAttributeDescriptions[1].location = 1;
    inputAttributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    inputAttributeDescriptions[1].offset = offsetof(VertexOutput, worldPosAndUV);
    // UVAndLighting (vec4) - texCoords.y, lightingEmit, transparencyLevel, faceIndex
    inputAttributeDescriptions[2].binding = 0;
    inputAttributeDescriptions[2].location = 2;
    inputAttributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    inputAttributeDescriptions[2].offset = offsetof(VertexOutput, uvAndLighting);
    // Material (vec4) - albR, albG, albB, metallic
    inputAttributeDescriptions[3].binding = 0;
    inputAttributeDescriptions[3].location = 3;
    inputAttributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    inputAttributeDescriptions[3].offset = offsetof(VertexOutput, material);
    // RoughnessAndTan (vec4) - roughness, tanX, tanY, tanZ
    inputAttributeDescriptions[4].binding = 0;
    inputAttributeDescriptions[4].location = 4;
    inputAttributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    inputAttributeDescriptions[4].offset = offsetof(VertexOutput, roughnessAndTan);
    // Bitangent (vec4) - bitanX, bitanY, bitanZ, padding
    inputAttributeDescriptions[5].binding = 0;
    inputAttributeDescriptions[5].location = 5;
    inputAttributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    inputAttributeDescriptions[5].offset = offsetof(VertexOutput, bitangent);
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &inputBindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(inputAttributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = inputAttributeDescriptions.data();
    
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
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
    colorBlendAttachment.blendEnable = VK_FALSE; // Disable alpha blending for opaque rendering
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
    graphicsPushConstantRange.size = 3 * sizeof(glm::mat4); // model, view, projection matrices
    graphicsPipelineLayoutInfo.pPushConstantRanges = &graphicsPushConstantRange;
    
    if (vkCreatePipelineLayout(context.device, &graphicsPipelineLayoutInfo, nullptr, &vk.pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline layout");
    }
    
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
        throw std::runtime_error("Failed to create graphics pipeline");
    }
    // Cleanup shader modules
    ShaderObject::DestroyShaderModule(context.device, vertShaderModule);
    ShaderObject::DestroyShaderModule(context.device, fragShaderModule);
}

void CameraPVMToFrustumPlanes(FrustumPlanes& frustum, World::Camera& cam) {
    // Extract frustum planes from view-projection matrix (world space)
    // Negate to get inward-pointing normals (points inside have positive distance)
    glm::mat4 vp = cam.GetProjectionMatrix() * cam.GetViewMatrix();
    glm::mat4 m = glm::transpose(vp);

    frustum.planes[0] = -(m[3] + m[0]); // Left
    frustum.planes[1] = -(m[3] - m[0]); // Right
    frustum.planes[2] = -(m[3] + m[1]); // Bottom
    frustum.planes[3] = -(m[3] - m[1]); // Top
    frustum.planes[4] = -(m[3] + m[2]); // Near
    frustum.planes[5] = -(m[3] - m[2]); // Far

    for (int i = 0; i < 6; ++i) {
        float length = glm::length(glm::vec3(frustum.planes[i]));
        if (length > 0.0001f) {
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
    
    // Visible count reset is now handled in OnDrawCompute using vkCmdFillBuffer (GPU-side)
    FrustumPlanes frustum { };
    CameraPVMToFrustumPlanes(frustum, resource.cameraModel->GetObject());
    
    // Update graphics descriptor set with textures from unit (only if textures changed)
    if (resource.unit) {
        const auto& textures = resource.unit->GetMaterial();
        
        // Only update descriptor set if we haven't already set the textures
        static Texture2* lastTextures[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        static bool firstTime = true;
        bool texturesChanged = false;
        
        if (firstTime || textures.top != lastTextures[0] || textures.down != lastTextures[1] ||
            textures.left != lastTextures[2] || textures.right != lastTextures[3] ||
            textures.front != lastTextures[4] || textures.back != lastTextures[5]) {
            texturesChanged = true;
            lastTextures[0] = textures.top;
            lastTextures[1] = textures.down;
            lastTextures[2] = textures.left;
            lastTextures[3] = textures.right;
            lastTextures[4] = textures.front;
            lastTextures[5] = textures.back;
            firstTime = false;
        }
        
        if (texturesChanged) {
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
        
        // Generate AO textures from unit textures (only if not already generated)
        if (vk.aoTexturesView[0] == VK_NULL_HANDLE) {
            TextureProperties aoProperties;
            aoProperties.format = TextureFormat::R8;
            aoProperties.generateMipmaps = true;
            aoProperties.minFilter = TextureFilter::LINEAR_MIPMAP_LINEAR;
            aoProperties.magFilter = TextureFilter::LINEAR;
            Texture2* aoTextures[6] = {
                GenerateLightningTexture(textures.top, aoProperties),
                GenerateLightningTexture(textures.down, aoProperties),
                GenerateLightningTexture(textures.left, aoProperties),
                GenerateLightningTexture(textures.right, aoProperties),
                GenerateLightningTexture(textures.front, aoProperties),
                GenerateLightningTexture(textures.back, aoProperties)
            };
            
            // Create Vulkan resources for generated AO textures
            for (int i = 0; i < 6; ++i) {
                if (aoTextures[i] && aoTextures[i]->IsLoaded()) {
                    aoTextures[i]->GetImageView(context);
                    // Use global sampler instead of creating individual samplers
                    
                    vk.aoTextures[i] = aoTextures[i]->binding.vkImage;
                    vk.aoTexturesMemory[i] = aoTextures[i]->binding.vkImageMemory;
                    vk.aoTexturesView[i] = aoTextures[i]->binding.vkImageView;
                    
                    // Clear handles from temporary Texture2 to prevent double-deletion
                    aoTextures[i]->binding.vkImage = VK_NULL_HANDLE;
                    aoTextures[i]->binding.vkImageMemory = VK_NULL_HANDLE;
                    aoTextures[i]->binding.vkImageView = VK_NULL_HANDLE;
                }
            }
            
            // Update AO texture descriptor
            VkDescriptorImageInfo aoImageInfo{};
            aoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            aoImageInfo.imageView = vk.aoTexturesView[0];
            aoImageInfo.sampler = vk.globalTextureSampler;
            
            VkWriteDescriptorSet aoTextureWrite{};
            aoTextureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            aoTextureWrite.dstSet = vk.descriptorSet;
            aoTextureWrite.dstBinding = 10;
            aoTextureWrite.dstArrayElement = 0;
            aoTextureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            aoTextureWrite.descriptorCount = 1;
            aoTextureWrite.pImageInfo = &aoImageInfo;
            vkUpdateDescriptorSets(context.device, 1, &aoTextureWrite, 0, nullptr);
            
            // Clean up generated textures
            for (int i = 0; i < 6; ++i) {
                if (aoTextures[i]) {
                    aoTextures[i]->CleanupVulkan(context);
                    delete aoTextures[i];
                }
            }
        }
        
        // Generate normal textures from unit textures (only if not already generated)
        if (vk.normalTexturesView[0] == VK_NULL_HANDLE) {
            TextureProperties normalProperties;
            normalProperties.format = TextureFormat::RGBA8; // Use RGBA8 for blit support
            normalProperties.generateMipmaps = true;
            normalProperties.minFilter = TextureFilter::LINEAR_MIPMAP_LINEAR;
            normalProperties.magFilter = TextureFilter::LINEAR;
            normalProperties.sRGB = false; // Normal maps should not be in sRGB space
            
            Texture2* normalTextures[6] = {
                GenerateNormalTexture(textures.top, normalProperties),
                GenerateNormalTexture(textures.down, normalProperties),
                GenerateNormalTexture(textures.left, normalProperties),
                GenerateNormalTexture(textures.right, normalProperties),
                GenerateNormalTexture(textures.front, normalProperties),
                GenerateNormalTexture(textures.back, normalProperties)
            };
            
            // Create Vulkan resources for generated normal textures
            for (int i = 0; i < 6; ++i) {
                if (normalTextures[i] && normalTextures[i]->IsLoaded()) {
                    normalTextures[i]->GetImageView(context);
                    // Use global sampler instead of creating individual samplers
                    
                    vk.normalTextures[i] = normalTextures[i]->binding.vkImage;
                    vk.normalTexturesMemory[i] = normalTextures[i]->binding.vkImageMemory;
                    vk.normalTexturesView[i] = normalTextures[i]->binding.vkImageView;
                    
                    // Clear handles from temporary Texture2 to prevent double-deletion
                    normalTextures[i]->binding.vkImage = VK_NULL_HANDLE;
                    normalTextures[i]->binding.vkImageMemory = VK_NULL_HANDLE;
                    normalTextures[i]->binding.vkImageView = VK_NULL_HANDLE;
                }
            }
            
            // Update normal texture descriptor (binding 11)
            VkDescriptorImageInfo normalImageInfo{};
            normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            normalImageInfo.imageView = vk.normalTexturesView[0];
            normalImageInfo.sampler = vk.globalTextureSampler;
            
            VkWriteDescriptorSet normalTextureWrite{};
            normalTextureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            normalTextureWrite.dstSet = vk.descriptorSet;
            normalTextureWrite.dstBinding = 11;
            normalTextureWrite.dstArrayElement = 0;
            normalTextureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            normalTextureWrite.descriptorCount = 1;
            normalTextureWrite.pImageInfo = &normalImageInfo;
            vkUpdateDescriptorSets(context.device, 1, &normalTextureWrite, 0, nullptr);
            
            // Clean up generated textures
            for (int i = 0; i < 6; ++i) {
                if (normalTextures[i]) {
                    normalTextures[i]->CleanupVulkan(context);
                    delete normalTextures[i];
                }
            }
        }
    }
}

void UnitStateDrawable::OnDraw(UnitStateResource& resource,
                               UnitStateDrawableVulkan& vk,
                               Game::VulkanContext& context)
{
    if (!resource.cameraModel)
        return;
    // Get camera matrices for push constants
    const World::Camera& cam = resource.cameraModel->GetObject();
    glm::mat4 model = cam.GetModelMatrix();
    glm::mat4 view = cam.GetViewMatrix();
    glm::mat4 projection = cam.GetProjectionMatrix();
    glm::mat4 matrices[3] = {model, view, projection};

    // Bind graphics pipeline
    if (vk.pipeline != VK_NULL_HANDLE && vk.pipelineLayout != VK_NULL_HANDLE) {
        vkCmdBindPipeline(
            context.commandBuffers[0],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            vk.pipeline
        );
        // Bind output vertex buffer (culled vertices from compute shader)
        VkBuffer vertexBuffers[] = {vk.outputVertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(context.commandBuffers[0], 0, 1, vertexBuffers, offsets);
        
        // Bind graphics descriptor set for textures
        vkCmdBindDescriptorSets(context.commandBuffers[0], VK_PIPELINE_BIND_POINT_GRAPHICS,
                               vk.pipelineLayout, 0, 1, &vk.descriptorSet, 0, nullptr);
        // Push camera matrices for vertex shader
        vkCmdPushConstants(context.commandBuffers[0], vk.pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(matrices), matrices);
        // Draw using indirect draw parameters from compute shader
        vkCmdDrawIndirect(
            context.commandBuffers[0],
            vk.indirectDrawBuffer,
            0,
            1,
            sizeof(VkDrawIndirectCommand)
        );
    }
}

void UnitStateDrawable::OnDrawCompute(UnitStateResource& resource,
                                    UnitStateDrawableVulkan& vk,
                                    Game::VulkanContext& context)
{
    if (!resource.cameraModel)
        return;

    // Get camera matrices for push constants
    UniformBufferObject ubo{};
    const World::Camera& cam = resource.cameraModel->GetObject();
    ubo.model = cam.GetModelMatrix();
    ubo.view = cam.GetViewMatrix();
    ubo.projection = cam.GetProjectionMatrix();

    // Reset visible vertex counter
    // using vkCmdFillBuffer (GPU-side operation)
    vkCmdFillBuffer(context.commandBuffers[0], vk.visibleCountBuffer, 0, sizeof(uint32_t), 0);

    // Update frustum data outside render pass
    FrustumPlanes frustum { };
    CameraPVMToFrustumPlanes(frustum, resource.cameraModel->GetObject());

    // Use the same size as the buffer creation
    constexpr VkDeviceSize frustumSize = sizeof(FrustumPlanes);
    vkCmdUpdateBuffer(context.commandBuffers[0], vk.frustumBuffer, 0, frustumSize, &frustum);

    // Update lighting data outside render pass
    struct LightingBlock {
        alignas(16) glm::vec3 sunDirection;
        alignas(16) glm::vec3 sunColor;
        alignas(4)  float ambientStrength;
        alignas(16) glm::vec3 cameraPosition;
    };

    LightingBlock lightingData{};
    lightingData.sunDirection = glm::normalize(glm::vec3(0.5f, 0.8f, 0.6f));
    lightingData.sunColor = glm::vec3(0.6f, 0.7f, 1.0f);
    lightingData.ambientStrength = 0.3f;
    World::AbstractCamera::Eye eyePos = cam.eye;
    lightingData.cameraPosition = glm::vec3(eyePos.x, eyePos.y, eyePos.z);

    // Use the same size as the buffer creation
    constexpr VkDeviceSize lightingBlockSize = sizeof(LightingBlock);
    vkCmdUpdateBuffer(context.commandBuffers[0], vk.placeholderLightingBuffer, 0, lightingBlockSize, &lightingData);

    VkMemoryBarrier fillBarrier{};
    fillBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    fillBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    fillBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_UNIFORM_READ_BIT;

    vkCmdPipelineBarrier(context.commandBuffers[0],
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0, 1, &fillBarrier, 0, nullptr, 0, nullptr);

    // Dispatch compute shader for frustum culling
    if (vk.computePipeline != VK_NULL_HANDLE && vk.computePipelineLayout != VK_NULL_HANDLE) {
        vkCmdBindPipeline(
            context.commandBuffers[0],
            VK_PIPELINE_BIND_POINT_COMPUTE,
            vk.computePipeline);
        vkCmdBindDescriptorSets(
            context.commandBuffers[0],
            VK_PIPELINE_BIND_POINT_COMPUTE,
            vk.computePipelineLayout,
            0,
            1,
            &vk.computeDescriptorSet,
            0,
            nullptr);
        vkCmdPushConstants(
            context.commandBuffers[0],
            vk.computePipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(UniformBufferObject),
            &ubo);

        // Calculate workgroup count (each workgroup processes 64 triangles)
        uint32_t triangleCount = unitVertices.size() / 3;
        uint32_t workgroupCount = (triangleCount + 63) / 64;
        vkCmdDispatch(context.commandBuffers[0], workgroupCount, 1, 1);

        // Barrier to ensure compute shader finishes writing before the copy operation starts
        VkBufferMemoryBarrier computeToCopyBarrier{};
        computeToCopyBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        computeToCopyBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        computeToCopyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        computeToCopyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        computeToCopyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        computeToCopyBarrier.buffer = vk.visibleCountBuffer;
        computeToCopyBarrier.offset = 0;
        computeToCopyBarrier.size = sizeof(uint32_t);

        vkCmdPipelineBarrier(
            context.commandBuffers[0],
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            1, &computeToCopyBarrier,
            0, nullptr
        );

        // Copy visible vertex count to indirect draw buffer (vertexCount field) [🖈]
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = sizeof(uint32_t);
        vkCmdCopyBuffer(context.commandBuffers[0], vk.visibleCountBuffer, vk.indirectDrawBuffer, 1, &copyRegion);

        VkBufferMemoryBarrier indirectTransferBarrier{};
        indirectTransferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        indirectTransferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        indirectTransferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        indirectTransferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        indirectTransferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        indirectTransferBarrier.buffer = vk.indirectDrawBuffer;
        indirectTransferBarrier.offset = 0;
        indirectTransferBarrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(context.commandBuffers[0],
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 1, &indirectTransferBarrier, 0, nullptr);

        // Fill instanceCount, firstVertex, firstInstance fields with total safety [🖈]
        vkCmdFillBuffer(
            context.commandBuffers[0],
            vk.indirectDrawBuffer,
            sizeof(uint32_t), sizeof(uint32_t), 1  // instanceCount = 1
        );
        vkCmdFillBuffer(
            context.commandBuffers[0],
            vk.indirectDrawBuffer,
            sizeof(uint32_t) * 2, sizeof(uint32_t), 0  // firstVertex = 0
        );
        vkCmdFillBuffer(
            context.commandBuffers[0],
            vk.indirectDrawBuffer,
            sizeof(uint32_t) * 3, sizeof(uint32_t), 0  // firstInstance = 0
        );

        // Final memory barrier to ensure everything is ready before entering Graphics [🖈]
        VkBufferMemoryBarrier barriers[2]{};

        // Output vertex buffer barrier
        barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barriers[0].dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barriers[0].buffer = vk.outputVertexBuffer;
        barriers[0].offset = 0;
        barriers[0].size = VK_WHOLE_SIZE;

        // Indirect draw buffer barrier [🖈]
        barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // Espera al último vkCmdFillBuffer parcial
        barriers[1].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barriers[1].buffer = vk.indirectDrawBuffer;
        barriers[1].offset = 0;
        barriers[1].size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(
            context.commandBuffers[0],
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, // Bloquea ambas etapas previas
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
            0,
            0, nullptr,
            2, barriers,
            0, nullptr
        );
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
    vkDestroyBuffer(context.device, vk.triplanarSettingsBuffer, nullptr);
    vkFreeMemory(context.device, vk.triplanarSettingsBufferMemory, nullptr);
    vkDestroySampler(context.device, vk.placeholderLightingSampler, nullptr);
    vkDestroyImageView(context.device, vk.placeholderLightingTextureView, nullptr);
    vkDestroyImage(context.device, vk.placeholderLightingTexture, nullptr);
    vkFreeMemory(context.device, vk.placeholderLightingTextureMemory, nullptr);
    vkDestroyBuffer(context.device, vk.placeholderSettingsBuffer, nullptr);
    vkFreeMemory(context.device, vk.placeholderSettingsBufferMemory, nullptr);
    
    // Cleanup PBR lighting resources
    vkDestroyBuffer(context.device, vk.placeholderLightingBuffer, nullptr);
    vkFreeMemory(context.device, vk.placeholderLightingBufferMemory, nullptr);
    vkDestroySampler(context.device, vk.placeholderAOSampler, nullptr);
    vkDestroyImageView(context.device, vk.placeholderAOTextureView, nullptr);
    vkDestroyImage(context.device, vk.placeholderAOTexture, nullptr);
    vkFreeMemory(context.device, vk.placeholderAOTextureMemory, nullptr);
    
    // Cleanup AO textures
    for (int i = 0; i < 6; ++i) {
        if (vk.aoTexturesView[i] != VK_NULL_HANDLE) {
            vkDestroyImageView(context.device, vk.aoTexturesView[i], nullptr);
        }
        if (vk.aoTextures[i] != VK_NULL_HANDLE) {
            vkDestroyImage(context.device, vk.aoTextures[i], nullptr);
        }
        if (vk.aoTexturesMemory[i] != VK_NULL_HANDLE) {
            vkFreeMemory(context.device, vk.aoTexturesMemory[i], nullptr);
        }
    }
    
    // Cleanup normal textures
    for (int i = 0; i < 6; ++i) {
        if (vk.normalTexturesView[i] != VK_NULL_HANDLE) {
            vkDestroyImageView(context.device, vk.normalTexturesView[i], nullptr);
        }
        if (vk.normalTextures[i] != VK_NULL_HANDLE) {
            vkDestroyImage(context.device, vk.normalTextures[i], nullptr);
        }
        if (vk.normalTexturesMemory[i] != VK_NULL_HANDLE) {
            vkFreeMemory(context.device, vk.normalTexturesMemory[i], nullptr);
        }
    }
    
    // Cleanup global sampler
    if (vk.globalTextureSampler != VK_NULL_HANDLE) {
        vkDestroySampler(context.device, vk.globalTextureSampler, nullptr);
    }
    
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
