#pragma once

#include "rl/World/Unit/Unit.h"
#include "rl/Base/StateDrawable.h"
#include "rl/Base/StateModel.h"
#include "rl/Client/State/CameraState.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace Rl::Providers {

// Just for data interchange between classes
struct UnitStateResource : StateResource {
    World::BaseUnit* unit;
    CameraModel* cam;
    explicit UnitStateResource(World::BaseUnit& unit, CameraModel& cameraModel) :
        StateResource(), unit(&unit), cam(&cameraModel)
    {
    }
};

struct UnitStateDrawableVulkan : StateDrawableVulkan {
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
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
    void OnPause() override = 0;
    void OnResume() override = 0;
};

class UnitModel :
    public StateModel<
        World::BaseUnit,
        UnitStateDrawable,
        UnitStateResource, UnitStateDrawableVulkan
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
