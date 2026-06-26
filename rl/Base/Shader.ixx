export module Rl.Base.Shader;

import <string>;
import <vector>;
import <vulkan/vulkan.hpp>;

namespace Rl::Providers
{

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
