export module Rl.Base.Shader;

import <string>;
import <vector>;
import <vulkan/vulkan.h>;

namespace Rl::Providers
{

export constexpr int Vert    = (0 << 1);
export constexpr int Frag    = (0 << 2);
export constexpr int Compute = (0 << 3);

export class ShaderObject
{
  public:
  struct ShaderModule
  {
    VkShaderModule module;
  };
  static ShaderModule      Module(VkDevice device, const std::vector<char>& code);
  static void              DestroyShaderModule(VkDevice device, ShaderModule& shaderModule);
  static std::vector<char> Shader(const std::string& filename);
  ShaderObject()                               = delete;
  ~ShaderObject()                              = delete;
  ShaderObject(const ShaderObject&)            = delete;
  ShaderObject& operator=(const ShaderObject&) = delete;
};

} // namespace Rl::Providers
