#include "Rl.Base/GameShaderModule.h"

#include <stdexcept>

namespace rl
{

GameShaderModule
GameShaderLoader::createShaderModule(VkDevice device, const uint32_t* code, size_t size)
{
        if (size == 0 || code == nullptr)
        {
                throw std::runtime_error(
                    "Invalid SPIR-V bytecode passed to createShaderModule (size: " +
                    std::to_string(size) + ", code: " + (code ? "valid" : "null") + ")");
        }

        uint32_t magic = code[0];
        if (magic != 0x07230203)
        {
                throw std::runtime_error("Invalid SPIR-V bytecode passed to createShaderModule "
                                         "(expected magic 0x07230203, got 0x" +
                                         std::to_string(magic) + ")");
        }

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = size * sizeof(uint32_t);
        createInfo.pCode    = code;

        VkShaderModule shaderModule;
        VkResult       result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);

        if (result != VK_SUCCESS)
        {
                throw std::runtime_error("Failed to create shader module");
        }
        return GameShaderModule(device, shaderModule);
}

} // namespace rl
