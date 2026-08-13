#ifndef RL_CLIENT_RENDERING_COMPOSITOR_SYNCHRONIZATION_HANDLER_H
#define RL_CLIENT_RENDERING_COMPOSITOR_SYNCHRONIZATION_HANDLER_H

#include "Rl.Base/GameVulkanFence.h"
#include "Rl.Base/GameVulkanSemaphore.h"

#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
#include <mutex>

namespace rl
{

/**
 * @brief Manages Vulkan synchronization primitives
 *
 * Provides thread-safe access to fences and semaphores for GPU synchronization.
 * Handles automatic cleanup and resource management using RAII.
 */
class CompositorSynchronizationHandler
{
        public:
                /**
                 * @brief Constructor
                 * @param device Vulkan device
                 * @param maxFramesInFlight Maximum number of frames that can be in flight
                 */
                CompositorSynchronizationHandler(VkDevice device, uint32_t maxFramesInFlight);

                /**
                 * @brief Destructor
                 */
                ~CompositorSynchronizationHandler();

                /**
                 * @brief Get the fence for the current frame
                 * @param frameIndex Current frame index
                 * @return Reference to the fence
                 */
                GameVulkanFence& getFence(uint32_t frameIndex);

                /**
                 * @brief Get the image available semaphore for the current frame
                 * @param frameIndex Current frame index
                 * @return Reference to the semaphore
                 */
                GameVulkanSemaphore& getImageAvailableSemaphore(uint32_t frameIndex);

                /**
                 * @brief Get the render finished semaphore for the current frame
                 * @param frameIndex Current frame index
                 * @return Reference to the semaphore
                 */
                GameVulkanSemaphore& getRenderFinishedSemaphore(uint32_t frameIndex);

                /**
                 * @brief Wait for a fence to complete
                 * @param frameIndex Frame index of the fence to wait for
                 * @param timeout Timeout in nanoseconds
                 * @return true if the fence was signaled, false if timeout occurred
                 */
                void waitForFence(uint32_t frameIndex, uint64_t timeout = UINT64_MAX);

                /**
                 * @brief Reset a fence to unsignaled state
                 * @param frameIndex Frame index of the fence to reset
                 */
                void resetFence(uint32_t frameIndex);

                /**
                 * @brief Get the maximum number of frames in flight
                 * @return Maximum frames in flight
                 */
                uint32_t getMaxFramesInFlight() const
                {
                        return maxFramesInFlight;
                }

        private:
                void createFences();
                void createSemaphores();

                VkDevice device;
                uint32_t maxFramesInFlight;

                std::vector<GameVulkanFence>     inFlightFences;
                std::vector<GameVulkanSemaphore> imageAvailableSemaphores;
                std::vector<GameVulkanSemaphore> renderFinishedSemaphores;

                mutable std::mutex mutex;
};

} // namespace rl

#endif
