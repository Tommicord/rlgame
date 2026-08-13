#ifndef RL_BASE_GAME_VULKAN_QUEUE_SUBMITTER_H
#define RL_BASE_GAME_VULKAN_QUEUE_SUBMITTER_H

#include <vulkan/vulkan.hpp>
#include <string>
#include <chrono>

namespace rl
{

/**
 * @brief Utility class for robust Vulkan queue submission with retry logic
 *
 * This class provides a static method to submit Vulkan command buffers to queues
 * with configurable retry logic to handle transient failures.
 */
class GameVulkanQueueSubmitter
{
        public:
                /**
                 * @brief Configuration for queue submission retry behavior
                 */
                struct RetryConfig
                {
                                int maxAttempts = 8; ///< Maximum number of submission attempts
                                std::chrono::milliseconds retryDelay{
                                    100}; ///< Delay between retry attempts
                                bool throwOnFailure =
                                    true; ///< Whether to throw exception on final failure
                };

                /**
                 * @brief Submits a command buffer to a Vulkan queue with retry logic
                 *
                 * @param queue The Vulkan queue to submit to
                 * @param submitInfo Pointer to the submit info structure
                 * @param fence Optional fence to signal when the command buffer completes
                 * @param config Retry configuration (uses defaults if not provided)
                 * @return VkResult The result of the submission (VK_SUCCESS on success, error code
                 * on failure)
                 * @throws std::runtime_error If submission fails after all attempts and
                 * throwOnFailure is true
                 */
                static void submit(VkQueue             queue,
                                   const VkSubmitInfo* submitInfo,
                                   VkFence             fence,
                                   const RetryConfig&  config = RetryConfig{});

                /**
                 * @brief Submits a command buffer to a Vulkan queue with retry logic (overload)
                 *
                 * @param queue The Vulkan queue to submit to
                 * @param submitInfo Pointer to the submit info structure
                 * @param fence Optional fence to signal when the command buffer completes
                 * @param maxAttempts Maximum number of submission attempts
                 * @param retryDelayMs Delay between retry attempts in milliseconds
                 * @param throwOnFailure Whether to throw exception on final failure
                 * @return VkResult The result of the submission (VK_SUCCESS on success, error code
                 * on failure)
                 * @throws std::runtime_error If submission fails after all attempts and
                 * throwOnFailure is true
                 */
                static void submit(VkQueue             queue,
                                   const VkSubmitInfo* submitInfo,
                                   VkFence             fence,
                                   int                 maxAttempts,
                                   int                 retryDelayMs   = 100,
                                   bool                throwOnFailure = true);

                /**
                 * @brief Converts a VkResult to a human-readable string
                 *
                 * @param result The Vulkan result code
                 * @return std::string String representation of the result
                 */
                static std::string resultToString(VkResult result);
};

} // namespace rl

#endif
