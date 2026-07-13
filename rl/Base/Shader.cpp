import Rl.Base.Shader;

import <fstream>;
import <stdexcept>;
import <vector>;
import <vulkan/vulkan.hpp>;

import Rl.RayLog.Macro;
import Rl.RayLog.Logger;

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
  // Try multiple possible shader directories
  const std::vector<std::string> possiblePaths = {
    "Shaders/",                    // Relative to current working directory
    "build/Shaders/",              // If running from project root
    "build/Debug/Shaders/",        // If running from project root with Debug build
  };

  for (const auto& base : possiblePaths)
  {
    std::ifstream file(base + filename, std::ios::ate | std::ios::binary);
    if (file.is_open())
    {
      const size_t      fileSize = file.tellg();
      std::vector<char> buffer(fileSize);
      file.seekg(0);
      file.read(buffer.data(), fileSize);
      file.close();
      return buffer;
    }
  }
  RayLog::LogFatal(RAYLOG_TAG, "Failed to open shader file %s", filename.c_str());
  return std::vector<char>();
}

} // namespace Rl::Providers
