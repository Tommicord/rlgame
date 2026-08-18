#include "rlgame.base/cvulkan/cvulkan_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

const char*
R_CVulkan_ErrorToString (const enum R_CVulkan_Error error)
{
        switch (error)
        {
        case R_CVULKAN_ERROR_OK:
                return "Success";
        case R_CVULKAN_ERROR_FAILED:
                return "General failure";
        case R_CVULKAN_ERROR_OUT_OF_MEMORY:
                return "Out of memory";
        case R_CVULKAN_ERROR_INVALID_ARGUMENT:
                return "Invalid argument";
        case R_CVULKAN_ERROR_NULL_POINTER:
                return "Null pointer";
        case R_CVULKAN_ERROR_NOT_INITIALIZED:
                return "Vulkan not initialized";
        case R_CVULKAN_ERROR_ALREADY_INITIALIZED:
                return "Already initialized";
        case R_CVULKAN_ERROR_INSTANCE_CREATE_FAILED:
                return "Failed to create Vulkan instance";
        case R_CVULKAN_ERROR_DEVICE_CREATE_FAILED:
                return "Failed to create Vulkan device";
        case R_CVULKAN_ERROR_EXTENSION_NOT_FOUND:
                return "Required extension not found";
        case R_CVULKAN_ERROR_LAYER_NOT_FOUND:
                return "Required layer not found";
        case R_CVULKAN_ERROR_PHYSICAL_DEVICE_NOT_FOUND:
                return "No suitable physical device";
        case R_CVULKAN_ERROR_QUEUE_FAMILY_NOT_FOUND:
                return "Required queue family not found";
        case R_CVULKAN_ERROR_SURFACE_CREATE_FAILED:
                return "Failed to create surface";
        case R_CVULKAN_ERROR_SWAPCHAIN_CREATE_FAILED:
                return "Failed to create swapchain";
        case R_CVULKAN_ERROR_SWAPCHAIN_OUT_OF_DATE:
                return "Swapchain is out of date, needs recreation";
        case R_CVULKAN_ERROR_SWAPCHAIN_SUBOPTIMAL:
                return "Swapchain is suboptimal, can recreate";
        case R_CVULKAN_ERROR_MAP_MEMORY_FAILED:
                return "Failed to map memory";
        case R_CVULKAN_ERROR_UNMAP_MEMORY_FAILED:
                return "Failed to unmap memory";
        case R_CVULKAN_ERROR_FENCE_WAIT_FAILED:
                return "Failed to wait for fence";
        case R_CVULKAN_ERROR_FENCE_RESET_FAILED:
                return "Failed to reset fence";
        case R_CVULKAN_ERROR_BUFFER_CREATE_FAILED:
                return "Failed to create buffer";
        case R_CVULKAN_ERROR_IMAGE_CREATE_FAILED:
                return "Failed to create image";
        case R_CVULKAN_ERROR_MEMORY_ALLOCATE_FAILED:
                return "Failed to allocate memory";
        case R_CVULKAN_ERROR_COMMAND_POOL_CREATE_FAILED:
                return "Failed to create command pool";
        case R_CVULKAN_ERROR_COMMAND_BUFFER_BEGIN_FAILED:
                return "Failed to begin command buffer";
        case R_CVULKAN_ERROR_COMMAND_BUFFER_END_FAILED:
                return "Failed to end command buffer";
        case R_CVULKAN_ERROR_RENDER_PASS_CREATE_FAILED:
                return "Failed to create render pass";
        case R_CVULKAN_ERROR_FRAMEBUFFER_CREATE_FAILED:
                return "Failed to create framebuffer";
        case R_CVULKAN_ERROR_SHADER_MODULE_CREATE_FAILED:
                return "Failed to create shader module";
        case R_CVULKAN_ERROR_PIPELINE_CREATE_FAILED:
                return "Failed to create pipeline";
        case R_CVULKAN_ERROR_DESCRIPTOR_POOL_CREATE_FAILED:
                return "Failed to create descriptor pool";
        case R_CVULKAN_ERROR_DESCRIPTOR_SET_LAYOUT_CREATE_FAILED:
                return "Failed to create descriptor set layout";
        case R_CVULKAN_ERROR_SAMPLER_CREATE_FAILED:
                return "Failed to create sampler";
        case R_CVULKAN_ERROR_DEVICE_LOST:
                return "Device lost, must exit immediately";
        case R_CVULKAN_ERROR_SURFACE_LOST:
                return "Surface lost, needs recreation";
        case R_CVULKAN_ERROR_OUT_OF_DATE_KHR:
                return "Out of date (window resize)";
        case R_CVULKAN_ERROR_UNKNOWN:
        default:
                return "Unknown error";
        }
}

enum R_CVulkan_Error
R_CVulkan_ResultToError (const VkResult result)
{
        switch (result)
        {
        case VK_SUCCESS:
                return R_CVULKAN_ERROR_OK;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        case VK_ERROR_INITIALIZATION_FAILED:
                return R_CVULKAN_ERROR_INSTANCE_CREATE_FAILED;
        case VK_ERROR_DEVICE_LOST:
                return R_CVULKAN_ERROR_DEVICE_LOST;
        case VK_ERROR_MEMORY_MAP_FAILED:
                return R_CVULKAN_ERROR_MAP_MEMORY_FAILED;
        case VK_ERROR_LAYER_NOT_PRESENT:
                return R_CVULKAN_ERROR_LAYER_NOT_FOUND;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
                return R_CVULKAN_ERROR_EXTENSION_NOT_FOUND;
        case VK_ERROR_FEATURE_NOT_PRESENT:
        case VK_ERROR_INCOMPATIBLE_DRIVER:
        case VK_ERROR_TOO_MANY_OBJECTS:
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
                return R_CVULKAN_ERROR_FAILED;
        case VK_ERROR_SURFACE_LOST_KHR:
                return R_CVULKAN_ERROR_SURFACE_LOST;
        case VK_ERROR_OUT_OF_DATE_KHR:
                return R_CVULKAN_ERROR_OUT_OF_DATE_KHR;
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                return R_CVULKAN_ERROR_SURFACE_CREATE_FAILED;
        case VK_SUBOPTIMAL_KHR:
                return R_CVULKAN_ERROR_SWAPCHAIN_SUBOPTIMAL;
        case VK_ERROR_OUT_OF_POOL_MEMORY_KHR:
        case VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR:
                return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        case VK_ERROR_FRAGMENTATION_EXT:
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR:
                return R_CVULKAN_ERROR_FAILED;
        default:
                return R_CVULKAN_ERROR_UNKNOWN;
        }
}

int
R_CVulkan_ShouldRecreateSwapchain (const enum R_CVulkan_Error error)
{
        return error == R_CVULKAN_ERROR_SWAPCHAIN_OUT_OF_DATE || error == R_CVULKAN_ERROR_OUT_OF_DATE_KHR
               || error == R_CVULKAN_ERROR_SURFACE_LOST;
}
