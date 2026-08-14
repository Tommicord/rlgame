#ifndef RL_BASE_GAME_SHADER_MODULE_H
#define RL_BASE_GAME_SHADER_MODULE_H

#include <cstdint>
#include <vulkan/vulkan.hpp>

namespace rl
{

struct GameShaderModule
{
    VkDevice       device       = VK_NULL_HANDLE;
    VkShaderModule shaderModule = VK_NULL_HANDLE;

    GameShaderModule() = default;
    GameShaderModule(VkDevice device, VkShaderModule module) : device(device), shaderModule(module)
    {
    }

    ~GameShaderModule()
    {
      if (shaderModule != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
      {
        vkDestroyShaderModule(device, shaderModule, nullptr);
      }
    }

    GameShaderModule(const GameShaderModule&)            = delete;
    GameShaderModule& operator=(const GameShaderModule&) = delete;
    GameShaderModule(GameShaderModule&& other) noexcept :
        device(other.device), shaderModule(other.shaderModule)
    {
      other.device       = VK_NULL_HANDLE;
      other.shaderModule = VK_NULL_HANDLE;
    }
    GameShaderModule& operator=(GameShaderModule&& other) noexcept
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
};

class GameShaderLoader
{
  public:
    static GameShaderModule createShaderModule(VkDevice device, const uint32_t* code, size_t size);
};

} // namespace rl

#endif // RL_BASE_GAME_SHADER_MODULE_H
