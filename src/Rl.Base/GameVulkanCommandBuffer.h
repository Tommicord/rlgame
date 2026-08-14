#ifndef RL_BASE_GAME_VULKAN_COMMAND_BUFFER_H
#define RL_BASE_GAME_VULKAN_COMMAND_BUFFER_H

#include <vulkan/vulkan.hpp>

namespace rl
{

class GameVulkanCommandBuffer
{
  public:
    GameVulkanCommandBuffer();
    GameVulkanCommandBuffer(VkDevice device, VkCommandPool commandPool);
    GameVulkanCommandBuffer(const GameVulkanCommandBuffer& other)            = delete;
    GameVulkanCommandBuffer& operator=(const GameVulkanCommandBuffer& other) = delete;
    GameVulkanCommandBuffer(GameVulkanCommandBuffer&& other) noexcept;
    GameVulkanCommandBuffer& operator=(GameVulkanCommandBuffer&& other) noexcept;
    ~GameVulkanCommandBuffer();

    VkCommandBuffer getCommandBuffer() const;
    VkCommandPool   getCommandPool() const;
    VkDevice        getDevice() const;

    void begin(VkCommandBufferUsageFlags flags = 0);
    void end();
    void reset(VkCommandBufferResetFlags flags = 0);

    /**
     * @brief Binds a pipeline to the command buffer
     * @param pipelineBindPoint Pipeline bind point (graphics or compute)
     * @param pipeline Pipeline to bind
     *
     * This method wraps vkCmdBindPipeline with error checking.
     */
    void bindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);

    /**
     * @brief Binds descriptor sets to the command buffer
     * @param pipelineBindPoint Pipeline bind point (graphics or compute)
     * @param layout Pipeline layout
     * @param firstSet First descriptor set index
     * @param descriptorSetCount Number of descriptor sets
     * @param pDescriptorSets Descriptor sets to bind
     * @param dynamicOffsetCount Number of dynamic offsets
     * @param pDynamicOffsets Dynamic offsets
     *
     * This method wraps vkCmdBindDescriptorSets with error checking.
     */
    void bindDescriptorSets(VkPipelineBindPoint     pipelineBindPoint,
                            VkPipelineLayout         layout,
                            uint32_t                 firstSet,
                            uint32_t                 descriptorSetCount,
                            const VkDescriptorSet*    pDescriptorSets,
                            uint32_t                 dynamicOffsetCount,
                            const uint32_t*           pDynamicOffsets);

    /**
     * @brief Pushes constants to the command buffer
     * @param layout Pipeline layout
     * @param stageFlags Shader stages to receive constants
     * @param offset Offset in push constant range
     * @param size Size of push constants
     * @param pValues Pointer to push constant data
     *
     * This method wraps vkCmdPushConstants with error checking.
     */
    void pushConstants(VkPipelineLayout    layout,
                       VkShaderStageFlags stageFlags,
                       uint32_t            offset,
                       uint32_t            size,
                       const void*         pValues);

    /**
     * @brief Dispatches compute work
     * @param groupCountX Number of work groups in X dimension
     * @param groupCountY Number of work groups in Y dimension
     * @param groupCountZ Number of work groups in Z dimension
     *
     * This method wraps vkCmdDispatch with error checking.
     */
    void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

  private:
    VkDevice        device;
    VkCommandPool   commandPool;
    VkCommandBuffer commandBuffer;
};

} // namespace rl

#endif
