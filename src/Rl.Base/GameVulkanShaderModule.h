#ifndef RL_BASE_GAME_SHADER_MODULE_H
#define RL_BASE_GAME_SHADER_MODULE_H

#include <cstdint>
#include <vulkan/vulkan.hpp>

namespace rl
{

class GameVulkanShaderModule
{
  public:

    GameVulkanShaderModule() = default;
    GameVulkanShaderModule(VkDevice device, VkShaderModule module) : device(device), shaderModule(module)
    {
    }

    ~GameVulkanShaderModule()
    {
      if (shaderModule != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
      {
        vkDestroyShaderModule(device, shaderModule, nullptr);
        shaderModule = VK_NULL_HANDLE;
      }
    }

    GameVulkanShaderModule(const GameVulkanShaderModule&)            = delete;
    GameVulkanShaderModule& operator=(const GameVulkanShaderModule&) = delete;
    GameVulkanShaderModule(GameVulkanShaderModule&& other) noexcept :
        device(other.device), shaderModule(other.shaderModule)
    {
      other.device       = VK_NULL_HANDLE;
      other.shaderModule = VK_NULL_HANDLE;
    }
    GameVulkanShaderModule& operator=(GameVulkanShaderModule&& other) noexcept
    {
      if (this != &other)
      {
        if (shaderModule != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
        {
          vkDestroyShaderModule(device, shaderModule, nullptr);
        }
        device             = other.device;
        shaderModule       = other.shaderModule;
        other.device       = VK_NULL_HANDLE;
        other.shaderModule = VK_NULL_HANDLE;
      }
      return *this;
    }

    VkShaderModule getShaderModule() const
    {
      return shaderModule;
    }
  private:
    VkDevice       device       = VK_NULL_HANDLE;
    VkShaderModule shaderModule = VK_NULL_HANDLE;
};

class GameVulkanShader
{
  public:
    static GameVulkanShaderModule shader(VkDevice device, const uint32_t* pCode, const size_t size);
};

} // namespace rl

#endif // RL_BASE_GAME_SHADER_MODULE_H
