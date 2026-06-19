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
    glm::vec3 sunDirection;
    glm::vec3 sunColor;
    float ambientStrength;
    glm::vec3 cameraPosition;
};

// Frustum planes for culling
struct FrustumPlanes {
    glm::vec4 planes[6];
};

void UnitStateDrawable::OnCreate(UnitStateResource& resource,
                                 UnitStateDrawableVulkan& vk,
                                 Game::VulkanContext& context)
{
    // Create vertex buffer
    VkDeviceSize bufferSize = sizeof(unitVertices[0]) * unitVertices.size();
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(context.device, &bufferInfo, nullptr, &vk.vertexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex buffer");
    }
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(context.device, vk.vertexBuffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = 0; // TODO: Find appropriate memory type
    
    if (vkAllocateMemory(context.device, &allocInfo, nullptr, &vk.vertexBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate vertex buffer memory");
    }
    
    vkBindBufferMemory(context.device, vk.vertexBuffer, vk.vertexBufferMemory, 0);
    // Copy vertex data
    void* data;
    vkMapMemory(context.device, vk.vertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, unitVertices.data(), bufferSize);
    vkUnmapMemory(context.device, vk.vertexBufferMemory);
    
    // Create uniform buffers
    VkBufferCreateInfo uniformBufferInfo{};
    uniformBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    uniformBufferInfo.size = sizeof(UniformBufferObject);
    uniformBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    uniformBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    vkCreateBuffer(context.device, &uniformBufferInfo, nullptr, &vk.uniformBuffer);
    
    VkMemoryRequirements uniformMemRequirements;
    vkGetBufferMemoryRequirements(context.device, vk.uniformBuffer, &uniformMemRequirements);
    
    VkMemoryAllocateInfo uniformAllocInfo{};
    uniformAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    uniformAllocInfo.allocationSize = uniformMemRequirements.size;
    uniformAllocInfo.memoryTypeIndex = 0;
    
    vkAllocateMemory(context.device, &uniformAllocInfo, nullptr, &vk.uniformBufferMemory);
    vkBindBufferMemory(context.device, vk.uniformBuffer, vk.uniformBufferMemory, 0);
    
    // Load shaders
    auto vertShaderCode = ShaderObject::Shader("unit.vert.spv");
    auto geomShaderCode = ShaderObject::Shader("unit.face.geom.spv");
    auto lightingFragCode = ShaderObject::Shader("unit.lightning.frag.spv");
    auto fragShaderCode = ShaderObject::Shader("unit.frag.spv");
    
    auto vertShaderModule = ShaderObject::CreateShaderModule(context.device, vertShaderCode);
    auto geomShaderModule = ShaderObject::CreateShaderModule(context.device, geomShaderCode);
    auto lightingFragModule = ShaderObject::CreateShaderModule(context.device, lightingFragCode);
    auto fragShaderModule = ShaderObject::CreateShaderModule(context.device, fragShaderCode);
    
    // Create shader stages
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule.module;
    vertShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo geomShaderStageInfo{};
    geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
    geomShaderStageInfo.module = geomShaderModule.module;
    geomShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule.module;
    fragShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, geomShaderStageInfo, fragShaderStageInfo};
    
    // Vertex input
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(UnitVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    std::array<VkVertexInputAttributeDescription, 10> attributeDescriptions{};
    // Position
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(UnitVertex, position);
    // TexCoords
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(UnitVertex, texCoords);
    // LightingEmit
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32_UINT;
    attributeDescriptions[2].offset = offsetof(UnitVertex, lightingEmit);
    // TransparencyLevel
    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32_UINT;
    attributeDescriptions[3].offset = offsetof(UnitVertex, transparencyLevel);
    // FaceIndex
    attributeDescriptions[4].binding = 0;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32_UINT;
    attributeDescriptions[4].offset = offsetof(UnitVertex, faceIndex);
    // PolRight
    attributeDescriptions[5].binding = 0;
    attributeDescriptions[5].location = 5;
    attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[5].offset = offsetof(UnitVertex, polRight);
    // PolLeft
    attributeDescriptions[6].binding = 0;
    attributeDescriptions[6].location = 6;
    attributeDescriptions[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[6].offset = offsetof(UnitVertex, polLeft);
    // Albedo
    attributeDescriptions[7].binding = 0;
    attributeDescriptions[7].location = 7;
    attributeDescriptions[7].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[7].offset = offsetof(UnitVertex, albedo);
    // Metallic
    attributeDescriptions[8].binding = 0;
    attributeDescriptions[8].location = 8;
    attributeDescriptions[8].format = VK_FORMAT_R32_SFLOAT;
    attributeDescriptions[8].offset = offsetof(UnitVertex, metallic);
    // Roughness
    attributeDescriptions[9].binding = 0;
    attributeDescriptions[9].location = 9;
    attributeDescriptions[9].format = VK_FORMAT_R32_SFLOAT;
    attributeDescriptions[9].offset = offsetof(UnitVertex, roughness);
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    
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
    
    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // TODO: Add descriptor set layout
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(UniformBufferObject);
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    if (vkCreatePipelineLayout(context.device, &pipelineLayoutInfo, nullptr, &vk.pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }
    
    // Graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 3;
    pipelineInfo.pStages = shaderStages;
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
    ShaderObject::DestroyShaderModule(context.device, geomShaderModule);
    ShaderObject::DestroyShaderModule(context.device, lightingFragModule);
    ShaderObject::DestroyShaderModule(context.device, fragShaderModule);
}

void UnitStateDrawable::OnUpdate(UnitStateResource& resource,
                                 UnitStateDrawableVulkan& vk,
                                 Game::VulkanContext& context)
{
    // Reuse the Camera model
    // This avoids unnecessary code duplication
    // More maintainable code
    resource.cam->UpdateFromStateModel(context);
}

void UnitStateDrawable::OnDraw(UnitStateResource& resource,
                               UnitStateDrawableVulkan& vk,
                               Game::VulkanContext& context)
{
    // Bind vertex buffer
    VkBuffer vertexBuffers[] = {vk.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(context.commandBuffers[0], 0, 1, vertexBuffers, offsets);

    // Bind uniform buffer
    vkCmdBindDescriptorSets(context.commandBuffers[0], VK_PIPELINE_BIND_POINT_GRAPHICS,
                           vk.pipelineLayout, 0, 1, &vk.descriptorSet, 0, nullptr);
    // Draw unit
    vkCmdDraw(context.commandBuffers[0], static_cast<uint32_t>(unitVertices.size()), 1, 0, 0);
}

void UnitStateDrawable::OnDestroy(UnitStateResource& resource,
                                  UnitStateDrawableVulkan& vk,
                                  Game::VulkanContext& context)
{
    vkDestroyBuffer(context.device, vk.vertexBuffer, nullptr);
    vkFreeMemory(context.device, vk.vertexBufferMemory, nullptr);
    vkDestroyBuffer(context.device, vk.uniformBuffer, nullptr);
    vkFreeMemory(context.device, vk.uniformBufferMemory, nullptr);
    vkDestroyPipeline(context.device, vk.pipeline, nullptr);
    vkDestroyPipelineLayout(context.device, vk.pipelineLayout, nullptr);
    vkDestroyDescriptorPool(context.device, vk.descriptorPool, nullptr);
}

} // namespace Rl::Providers
