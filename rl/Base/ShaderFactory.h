#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace Rl::Providers
{

constexpr int Vert    = (0 << 1);
constexpr int Frag    = (0 << 2);
constexpr int Compute = (0 << 3);

class ShaderObject
{
  public:
  struct ShaderModule
  {
    VkShaderModule module;
  };
  static ShaderModule      CreateShaderModule(VkDevice device, const std::vector<char>& code);
  static void              DestroyShaderModule(VkDevice device, ShaderModule& shaderModule);
  static std::vector<char> Shader(const std::string& filename);
  ShaderObject()                               = delete;
  ~ShaderObject()                              = delete;
  ShaderObject(const ShaderObject&)            = delete;
  ShaderObject& operator=(const ShaderObject&) = delete;
};

} // namespace Rl::Providers
