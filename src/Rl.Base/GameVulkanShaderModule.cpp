#include "Rl.Base/GameVulkanShaderModule.h"

#include "Rl.Base/GameError.h"

#include <stdexcept>

namespace rl
{

GameVulkanShaderModule
GameVulkanShader::shader(VkDevice device, const uint32_t* pCode, const size_t size)
{
  if (size == 0 || pCode == nullptr)
  {
    GameError::exitWithError(
      "SPIR-V Shader Load failure",
      "Invalid SPIR-V bytecode passed to shader "
      "(size: " + std::to_string(size) + ", "
      "pCode: " + (pCode ? "valid" : "null") + ")"
    );
  }

  uint32_t magic = pCode[0];
  if (magic != 0x07230203)
  {
    GameError::exitWithError("SPIR-V Shader Load failure",
                             "Invalid SPIR-V bytecode passed to shader "
                             "(expected magic 0x07230203, got 0x" + 
                             std::to_string(magic) + ")");
  }

  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = size * sizeof(uint32_t);
  createInfo.pCode    = pCode;

  VkShaderModule shaderModule;
  VkResult       result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);

  if (result != VK_SUCCESS)
  {
    GameError::exitWithError(
      "SPIR-V Shader Load failure",
      "Failed to create shader module "
      "(result = " + GameError::vulkanResultToString(result) + ")");
  }
  return GameVulkanShaderModule(device, shaderModule);
}

} // namespace rl
