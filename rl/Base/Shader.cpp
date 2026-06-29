import Rl.Base.Shader;

import <fstream>;
import <stdexcept>;
import <vector>;
import <vulkan/vulkan.hpp>;

namespace Rl::Providers
{

ShaderObject::ShaderModule ShaderObject::Module(
    const VkDevice device, const std::vector<char>& code)
{
  ShaderModule             module;
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
  if (vkCreateShaderModule(device, &createInfo, nullptr, &module.module) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create shader module");
  }

  return module;
}

void ShaderObject::DestroyShaderModule(VkDevice device, ShaderModule& shaderModule)
{
  if (shaderModule.module != VK_NULL_HANDLE)
  {
    vkDestroyShaderModule(device, shaderModule.module, nullptr);
    shaderModule.module = VK_NULL_HANDLE;
  }
}

std::vector<char> ShaderObject::Shader(const std::string& filename)
{
  const std::string base = "Shaders/";
  std::ifstream     file(base + filename, std::ios::ate | std::ios::binary);
  if (!file.is_open())
  {
    throw std::runtime_error("Failed to open shader file " + base + filename);
  }
  const size_t      fileSize = file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();
  return buffer;
}

} // namespace Rl::Providers
