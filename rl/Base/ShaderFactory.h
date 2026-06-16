#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace Rl::Providers
{

class ShaderObject {
public:
    struct ShaderModule {
        VkShaderModule module;
    };

    static ShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code);
    static void DestroyShaderModule(VkDevice device, ShaderModule& shaderModule);
    static std::vector<char> ReadShaderFile(const std::string& filename);

    ShaderObject() = delete;
    ~ShaderObject() = delete;
    ShaderObject(const ShaderObject&) = delete;
    ShaderObject& operator=(const ShaderObject&) = delete;
};

}