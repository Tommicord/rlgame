#ifndef RL_BASE_IGAME_COMPUTE_DISPATCH_H
#define RL_BASE_IGAME_COMPUTE_DISPATCH_H

#include "Rl.Base/GameVulkanFence.h"
#include "Rl.Base/GameVulkanSemaphore.h"

#include <vulkan/vulkan.hpp>
#include <mutex>

namespace rl
{

/**
 * @brief Interface for compute shader dispatch operations
 *
 * Classes that implement this interface can be dispatched through GameComputeDispatch
 * which handles synchronization (semaphores and fences) automatically.
 */
class IGameComputeDispatch
{
  public:
    virtual ~IGameComputeDispatch() = default;

    /**
     * @brief Get a const reference the completion semaphore for this dispatch
     * @return The semaphore that will be signaled when the dispatch completes
     */
    virtual const GameVulkanSemaphore& getCompletionSemaphore() const = 0;

    /**
     * @brief Get a mutable reference the completion semaphore for this dispatch
     * @return The semaphore that will be signaled when the dispatch completes
     */
    virtual GameVulkanSemaphore& getCompletionSemaphore() = 0;

    /**
     * @brief Get a const reference to the completion fence for this dispatch
     * @return The fence that will be signaled when the dispatch completes
     */
    virtual const GameVulkanFence& getCompletionFence() const = 0;

    /**
     * @brief Get a mutable reference to the completion fence for this dispatch
     * @return The fence that will be signaled when the dispatch completes
     */
    virtual GameVulkanFence& getCompletionFence() = 0;

    /**
     * @brief Get the generate mutex for external synchronization
     * @return Reference to the generate mutex
     */
    virtual std::recursive_mutex& getGenerateMutex() = 0;

  protected:
    friend class GameComputeDispatch;
    /**
     * @brief Internal dispatch method called by GameComputeDispatch
     *
     * This method is private to IGameComputeDispatch and can only be called by
     * GameComputeDispatch. The pResource parameter is a void pointer that should be
     * cast to the appropriate type.
     *
     * @param pResource Pointer to the resource data (cast to appropriate type in
     * implementation)
     * @param waitSemaphore Semaphore to wait on before dispatch (may be nullptr)
     * @param fence Fence to signal when dispatch completes (may be nullptr)
     */
    virtual void
    dispatch(void* pResource, const GameVulkanSemaphore& waitSemaphore, GameVulkanFence& fence) = 0;
};

} // namespace rl

#endif // RL_BASE_IGAME_COMPUTE_DISPATCH_H
