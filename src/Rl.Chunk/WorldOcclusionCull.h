#ifndef RL_CHUNK_WORLD_OCCLUSION_CULL_H
#define RL_CHUNK_WORLD_OCCLUSION_CULL_H

#include "Rl.Base/GameShaderModule.h"
#include "Rl.Base/GameVulkanImage.h"
#include "Rl.Base/GameVulkanImageView.h"
#include "Rl.Base/GameVulkanCommandBuffer.h"
#include "Rl.Base/GameVulkanSemaphore.h"
#include "Rl.Base/GameVulkanCommandPool.h"
#include "Rl.Base/IGameComputeDispatch.h"
#include "Rl.Base/GameVulkanFence.h"
#include "Rl.Base/GameDevice.h"

#include <cstdint>
#include <mutex>
#include <vulkan/vulkan.hpp>

namespace rl
{

/** @brief Configuration parameters for WorldOcclusionCull */
struct WorldOcclusionCullData
{
                uint32_t width; /**< Output width */
                uint32_t height; /**< Output height */
                uint32_t depth; /**< Output depth */
                uint32_t airUnitId; /**< ID of air units to skip */
};

/** @brief Push constants for occlusion culling compute shader */
struct WorldOcclusionCullPushConstants
{
                uint32_t width;
                uint32_t height;
                uint32_t depth;
                uint32_t airUnitId;
                uint32_t _padding[3];
};

/** @brief Resource pointer when dispatching WorldOcclusionCull */
struct WorldOcclusionCullPResource
{
                WorldOcclusionCullPushConstants* pParams;
};

class WorldUnitPlacement;

/** @brief GPU-accelerated occlusion culling using Vulkan compute shaders */
class WorldOcclusionCull : public IGameComputeDispatch
{
        public:
                /** @brief Constructs an occlusion culling generator
                 * @param data Configuration parameters
                 * @param unitPlacement Reference to WorldUnitPlacement to read unit data from GPU
                 * @param instance Vulkan device instance */
                WorldOcclusionCull(const WorldOcclusionCullData& data,
                                   WorldUnitPlacement&           unitPlacement,
                                   GameDeviceInstance&       instance);
                /** @brief Destroys the occlusion culling generator */
                ~WorldOcclusionCull();
                WorldOcclusionCull(const WorldOcclusionCull& other)            = delete;
                WorldOcclusionCull& operator=(const WorldOcclusionCull& other) = delete;

                /** @brief Returns the generate mutex for external synchronization
                 * @return Reference to the generate mutex */
                std::recursive_mutex& getGenerateMutex() override;

                /** @brief Returns the completion semaphore
                 * @return Reference to the completion semaphore */
                const GameVulkanSemaphore& getCompletionSemaphore() const override;
                GameVulkanSemaphore&       getCompletionSemaphore() override;

                /**
                 * @brief Returns the completion fence
                 * @return Reference to the completion fence
                 */
                const GameVulkanFence& getCompletionFence() const override;
                GameVulkanFence&       getCompletionFence() override;

                /** @brief Returns the visibility output image
                 * @return Reference to the visibility output image */
                const GameVulkanImage& getVisibilityOutputImage() const;

                /** @brief Returns the visibility output image view
                 * @return Reference to the visibility output image view */
                const GameVulkanImageView& getVisibilityOutputImageView() const;

        protected:
                void dispatch(void*                      pResource,
                              const GameVulkanSemaphore& waitSemaphore,
                              GameVulkanFence&           fence) override;

        private:
                void createDescriptorSets();
                void createPipeline();
                void createVisibilityOutputImage(VkDevice device, VkPhysicalDevice physicalDevice);
                void createVisibilityOutputImageView(VkDevice device);

                VkDevice         device         = VK_NULL_HANDLE;
                VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
                VkQueue          graphicsQueue  = VK_NULL_HANDLE;
                VkCommandPool    commandPool    = VK_NULL_HANDLE;

                uint32_t            width;
                uint32_t            height;
                uint32_t            depth;
                uint32_t            airUnitId;
                WorldUnitPlacement& unitPlacement;

                GameShaderModule computeShaderModule;

                VkPipeline            pipeline            = VK_NULL_HANDLE;
                VkPipelineLayout      pipelineLayout      = VK_NULL_HANDLE;
                VkInstance            instance            = VK_NULL_HANDLE;
                VkDescriptorSet       descriptorSet       = VK_NULL_HANDLE;
                VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
                VkDescriptorPool      descriptorPool      = VK_NULL_HANDLE;

                GameVulkanSemaphore       completionSemaphore;
                GameVulkanFence           completionFence;
                GameVulkanMemoryAllocator memoryAllocator;
                GameVulkanImage           visibilityOutputImage;
                GameVulkanImageView       visibilityOutputImageView;
                GameVulkanCommandBuffer   commandBuffer;
                std::recursive_mutex      generateMutex;
};

} // namespace rl

#endif
