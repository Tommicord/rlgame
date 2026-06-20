#pragma once

#include "rl/World/Unit/Unit.h"
#include "rl/Base/StateDrawable.h"
#include "rl/Base/StateModel.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace Rl::Providers {

class CameraModel;

// Just for data interchange between classes
struct UnitStateResource : StateResource {
    World::BaseUnit* unit;
    CameraModel* cameraModel;

    /* Note: due to compatibility with one parameter constructor
     * This can't pass the CameraModel
     * the camera must be set manually or will fail everything
     */
    explicit UnitStateResource(World::BaseUnit& unit) :
        StateResource(), unit(&unit), cameraModel(nullptr)
    {
    }
};

struct UnitStateDrawableVulkan : StateDrawableVulkan {
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer outputVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory outputVertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer visibleCountBuffer = VK_NULL_HANDLE;
    VkDeviceMemory visibleCountBufferMemory = VK_NULL_HANDLE;
    VkBuffer indirectDrawBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indirectDrawBufferMemory = VK_NULL_HANDLE;
    VkBuffer frustumBuffer = VK_NULL_HANDLE;
    VkDeviceMemory frustumBufferMemory = VK_NULL_HANDLE;
    VkPipeline computePipeline = VK_NULL_HANDLE;
    VkPipelineLayout computePipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout computeDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet computeDescriptorSet = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    
    // Placeholder resources for bindings 8 and 9
    VkImage placeholderLightingTexture = VK_NULL_HANDLE;
    VkDeviceMemory placeholderLightingTextureMemory = VK_NULL_HANDLE;
    VkImageView placeholderLightingTextureView = VK_NULL_HANDLE;
    VkSampler placeholderLightingSampler = VK_NULL_HANDLE;
    VkBuffer placeholderSettingsBuffer = VK_NULL_HANDLE;
    VkDeviceMemory placeholderSettingsBufferMemory = VK_NULL_HANDLE;
    UnitStateDrawableVulkan() = default;
};

class UnitStateDrawable :
    public StateDrawable<UnitStateResource, UnitStateDrawableVulkan>
{
public:
    /* Stores the base class type */
    using Base = StateDrawable<UnitStateResource, UnitStateDrawableVulkan>;

    void OnDraw(UnitStateResource& resource,
                UnitStateDrawableVulkan& vk,
                Game::VulkanContext& context) override;
    void OnUpdate(UnitStateResource& resource,
                  UnitStateDrawableVulkan& vk,
                  Game::VulkanContext& context) override;
    void OnCreate(UnitStateResource& resource,
                  UnitStateDrawableVulkan& vk,
                  Game::VulkanContext& context) override;
    void OnDestroy(UnitStateResource& resource,
                   UnitStateDrawableVulkan& vk,
                   Game::VulkanContext& context) override;
    void OnPause() override;
    void OnResume() override;
};

class UnitModel :
    public StateModel<
        World::BaseUnit,
        UnitStateDrawable, UnitStateResource,
        UnitStateDrawableVulkan
>
{
    std::shared_ptr<Providers::UnitStateDrawable> unitDrawable;
    std::unique_ptr<Providers::UnitStateResource> unitResource;
    std::unique_ptr<Providers::UnitStateDrawableVulkan> unitVk;
    std::unique_ptr<World::BaseUnit> unit;
public:
    /* Constructs a model of the Camera class */
    explicit UnitModel(Game::VulkanContext& context);

    /* Destructs a CameraModel */
    ~UnitModel() override = default;

    /* Gets the stored camera */
    World::BaseUnit& GetObject() override;

    /* Gets the stored camera */
    UnitStateResource& GetResource() override;

    /* Gets the stored camera */
    UnitStateDrawable& GetDrawable() override;

    /* Gets the stored camera */
    UnitStateDrawableVulkan& GetVulkanState() override;
};

} // namespace Rl::Providers
