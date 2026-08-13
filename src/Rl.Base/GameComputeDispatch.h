#ifndef RL_BASE_GAME_COMPUTE_DISPATCH_H
#define RL_BASE_GAME_COMPUTE_DISPATCH_H

#include "Rl.Base/IGameComputeDispatch.h"
#include "Rl.Base/GameVulkanFence.h"
#include "Rl.Base/GameVulkanCommandPool.h"
#include "Rl.Base/GameVulkanCommandBuffer.h"
#include "Rl.Base/GameVulkanQueueSubmitter.h"
#include "Rl.Base/GameVulkanSemaphore.h"

#include <vector>
#include <mutex>
#include <memory>
#include <atomic>

namespace rl
{

/**
 * @brief Manages compute shader dispatch with automatic synchronization
 *
 * This class handles semaphore and fence management for compute shader dispatches,
 * allowing chained dispatches where each stage waits for the previous one to complete.
 *
 * Key improvements:
 * - Proper fence state tracking to prevent reuse before signaling
 * - Internal semaphore creation and lifecycle management for chaining
 * - Correct pipeline stage masks for compute operations
 * - Comprehensive error handling and validation
 * - Thread-safe operations with proper synchronization
 */
class GameComputeDispatch
{
        public:
                /**
                 * @brief Constructor
                 * @param device Vulkan device
                 * @param queue Queue for compute operations
                 * @param commandPool Command pool for recording commands
                 */
                GameComputeDispatch(VkDevice device, VkQueue queue, VkCommandPool commandPool);

                /**
                 * @brief Convenience constructor that creates an internal command pool from a queue
                 * family index
                 * @param device Vulkan device
                 * @param queue Vulkan queue
                 * @param queueFamilyIndex Queue family index to create command pool for
                 */
                GameComputeDispatch(VkDevice device, VkQueue queue, uint32_t queueFamilyIndex);

                /**
                 * @brief Destructs a GameComputeDispatch object, Ensures all pending operations
                 * complete
                 */
                ~GameComputeDispatch();

                GameComputeDispatch(const GameComputeDispatch&)            = delete;
                GameComputeDispatch& operator=(const GameComputeDispatch&) = delete;

                GameComputeDispatch(GameComputeDispatch&&)            = delete;
                GameComputeDispatch& operator=(GameComputeDispatch&&) = delete;

                /**
                 * @brief Dispatch a single compute shader
                 * @param computeDispatch The compute shader to dispatch
                 * @param pResource Pointer to resource data (cast to appropriate type in
                 * implementation)
                 * @param waitSemaphore Optional semaphore to wait on before dispatch
                 */
                void dispatchSingle(IGameComputeDispatch& computeDispatch,
                                    void*                 pResource,
                                    GameVulkanSemaphore&  waitSemaphore);

                /**
                 * @brief Dispatch multiple compute shaders in a chain
                 *
                 * Each compute shader waits for the previous one to complete via semaphores.
                 * Internal semaphores are created and managed automatically for synchronization.
                 * The final semaphore can be retrieved via getLastCompletionSemaphore().
                 *
                 * @param pComputeDispatches Pointer to array of compute shaders to dispatch in
                 * order
                 * @param computeDispatchCount Number of compute shaders
                 * @param pComputeResources Pointer to array of resource data (cast to appropriate
                 * type in implementation)
                 * @param computeResourceCount Number of resource data
                 * @param initialWaitSemaphore Optional semaphore to wait on before first dispatch
                 *
                 * @note If the resource count is less than the dispatch count, the remaining
                 * resources will be nullptr
                 * @note Creates internal semaphores for chaining if computeDispatchCount > 1
                 */
                void dispatchChained(void*                pComputeDispatches,
                                     size_t               computeDispatchCount,
                                     void*                pComputeResources,
                                     size_t               computeResourceCount,
                                     GameVulkanSemaphore& initialWaitSemaphore);

                /**
                 * @brief Get a const reference the completion semaphore from the last dispatch
                 * @return The semaphore signaled by the last dispatch (may be VK_NULL_HANDLE)
                 */
                const GameVulkanSemaphore& getLastCompletionSemaphore() const;

                /**
                 * @brief Get a mutable reference the completion semaphore from the last dispatch
                 * @return The semaphore signaled by the last dispatch (may be VK_NULL_HANDLE)
                 */
                GameVulkanSemaphore& getLastCompletionSemaphore();

                /**
                 * @brief Wait for all pending operations to complete
                 * @param timeout Timeout in nanoseconds (default: UINT64_MAX for infinite wait)
                 * @return true if operations completed, false if timeout occurred
                 */
                bool waitForCompletion(uint64_t timeout = UINT64_MAX);

                /**
                 * @brief Reset internal state for reuse
                 *
                 * Clears the last completion semaphore and resets fence state.
                 * Call this when you want to start a fresh sequence of dispatches.
                 */
                void reset();

        private:
                /**
                 * @brief Wait for and synchronize with the previous completion semaphore
                 * @param newWaitSemaphore The new semaphore to wait on (to avoid unnecessary sync
                 * if same)
                 */
                void synchronizeWithPreviousCompletion(const GameVulkanSemaphore& newWaitSemaphore);

                /**
                 * @brief Create a new binary semaphore for internal synchronization
                 * @return The created semaphore
                 */
                GameVulkanSemaphore createBinarySemaphore();

                /**
                 * @brief Safely reset the fence, ensuring it's not in use
                 */
                void safeResetFence();

                /**
                 * @brief Validate that the fence is in a signaled state before reset
                 * @return true if fence is signaled and can be reset
                 */
                bool isFenceSignaled() const;

                /**
                 * @brief Submit empty work to wait on a semaphore (for synchronization)
                 * @param waitSemaphore Semaphore to wait on
                 * @param waitStageMask Pipeline stage to wait at
                 * @param fence Fence to signal when wait completes
                 * @return Vulkan result code
                 */
                void submitWait(const GameVulkanSemaphore& waitSemaphore,
                                VkPipelineStageFlags       waitStageMask,
                                GameVulkanFence&           fence);

        private:
                VkDevice device;
                VkQueue  queue;

                GameVulkanCommandPool commandPool;
                GameVulkanFence       fence;
                mutable std::mutex    fenceMutex;

                GameVulkanSemaphore              lastCompletionSemaphore;
                std::vector<GameVulkanSemaphore> ownedSemaphores;

                std::atomic<bool>  fenceInUse{false};
                mutable std::mutex stateMutex;
};

} // namespace rl

#endif // RL_BASE_GAME_COMPUTE_DISPATCH_H
