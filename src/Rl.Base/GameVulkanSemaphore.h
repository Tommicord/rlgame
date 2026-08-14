#ifndef RL_BASE_GAME_VULKAN_SEMAPHORE_H
#define RL_BASE_GAME_VULKAN_SEMAPHORE_H

#include <vulkan/vulkan.hpp>

namespace rl
{

/** @brief Create info for semaphore creation */
struct GameVulkanSemaphoreCreateInfo
{
    VkSemaphoreCreateFlags flags         = 0;
    VkSemaphoreType        semaphoreType = VK_SEMAPHORE_TYPE_BINARY;
    uint64_t               initialValue  = 0; // For timeline semaphores
};

/** @brief RAII wrapper for Vulkan semaphore objects */
class GameVulkanSemaphore
{
  public:
    /**
     * @brief Constructs a semaphore (VK_NULL_HANDLE by default)
     */
    GameVulkanSemaphore() noexcept;
    /* @brief Constructs a semaphore by createInfo
     * @param device Vulkan device
     * @param createInfo Semaphore creation info */
    GameVulkanSemaphore(VkDevice device, const GameVulkanSemaphoreCreateInfo& createInfo);

    /** @brief Destroys the semaphore */
    ~GameVulkanSemaphore();

    GameVulkanSemaphore(GameVulkanSemaphore& other);
    GameVulkanSemaphore(const GameVulkanSemaphore& other) = delete;
    GameVulkanSemaphore(GameVulkanSemaphore&& other) noexcept;
    GameVulkanSemaphore& operator=(const GameVulkanSemaphore& other) = delete;
    GameVulkanSemaphore& operator=(GameVulkanSemaphore&& other) noexcept;

    /** @brief Returns the semaphore handle
     * @return Vulkan semaphore handle */
    VkSemaphore getSemaphore() const;

    /** @brief Sets the semaphore to the current state (takes ownership)
     * @param other Vulkan semaphore handle to take ownership of */
    void setSemaphore(VkSemaphore other);

    /** @brief Sets the semaphore without taking ownership (non-owning reference)
     * @param other Vulkan semaphore handle to reference without ownership */
    void setSemaphoreNonOwning(VkSemaphore other);

  private:
    VkDevice    device        = VK_NULL_HANDLE;
    VkSemaphore semaphore     = VK_NULL_HANDLE;
    bool        ownsSemaphore = true;
};

} // namespace rl

#endif // RL_BASE_GAME_VULKAN_SEMAPHORE_H
