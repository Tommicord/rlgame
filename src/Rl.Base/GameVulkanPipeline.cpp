#include "Rl.Base/GameVulkanPipeline.h"
#include "Rl.Base/GameError.h"

namespace rl
{

GameVulkanPipeline::GameVulkanPipeline() noexcept :
    device(VK_NULL_HANDLE), pipeline(VK_NULL_HANDLE), ownsPipeline(true)
{
}

GameVulkanPipeline::GameVulkanPipeline(GameVulkanPipeline&& other) noexcept :
    device(other.device), pipeline(other.pipeline), ownsPipeline(other.ownsPipeline)
{
        other.device       = VK_NULL_HANDLE;
        other.pipeline     = VK_NULL_HANDLE;
        other.ownsPipeline = false;
}

GameVulkanPipeline::GameVulkanPipeline(VkDevice                            device,
                                       const GameVulkanPipelineCreateInfo& createInfo) :
    device(device), pipeline(VK_NULL_HANDLE), ownsPipeline(true)
{
        VkResult result = VK_SUCCESS;
        if (createInfo.pGraphicsCreateInfo)
        {
                result =
                    vkCreateGraphicsPipelines(device, createInfo.pipelineCache, 1,
                                              createInfo.pGraphicsCreateInfo, nullptr, &pipeline);
        }
        else if (createInfo.pComputeCreateInfo)
        {
                result =
                    vkCreateComputePipelines(device, createInfo.pipelineCache, 1,
                                             createInfo.pComputeCreateInfo, nullptr, &pipeline);
        }

        if (result != VK_SUCCESS)
        {
                GameError::exitWithError(
                    "vkCreatePipelines",
                    "Failed to create pipeline (result = " +
                        GameError::vulkanResultToString(result) + ")",
                    device, VK_NULL_HANDLE, VK_NULL_HANDLE);
        }
}

GameVulkanPipeline::~GameVulkanPipeline()
{
        if (pipeline != VK_NULL_HANDLE && ownsPipeline)
        {
                vkDestroyPipeline(device, pipeline, nullptr);
                pipeline = VK_NULL_HANDLE;
        }
}

GameVulkanPipeline& GameVulkanPipeline::operator=(GameVulkanPipeline&& other) noexcept
{
        if (this != &other)
        {
                if (pipeline != VK_NULL_HANDLE && ownsPipeline)
                {
                        vkDestroyPipeline(device, pipeline, nullptr);
                }
                device             = other.device;
                pipeline           = other.pipeline;
                ownsPipeline       = other.ownsPipeline;
                other.device       = VK_NULL_HANDLE;
                other.pipeline     = VK_NULL_HANDLE;
                other.ownsPipeline = false;
        }
        return *this;
}

VkPipeline GameVulkanPipeline::getPipeline() const
{
        return pipeline;
}

void GameVulkanPipeline::setPipeline(VkPipeline other)
{
        if (pipeline != VK_NULL_HANDLE && ownsPipeline)
        {
                vkDestroyPipeline(device, pipeline, nullptr);
        }
        pipeline     = other;
        ownsPipeline = true;
}

void GameVulkanPipeline::setPipelineNonOwning(VkPipeline other)
{
        if (pipeline != VK_NULL_HANDLE && ownsPipeline)
        {
                vkDestroyPipeline(device, pipeline, nullptr);
        }
        pipeline     = other;
        ownsPipeline = false;
}

} // namespace rl
