#include "rlgame.base/cvulkan/cvulkan_platform.h"

R_CVULKAN_API const char*
r_cvulkan_error_to_string (const enum R_CVulkan_Error error)
{
    switch (error)
    {
    case R_CVULKAN_OK:
        return "Success: Operation completed successfully";
    case R_CVULKAN_ERROR_FAILED:
        return "General failure: An unspecified error occurred. Check logs for more details.";
    case R_CVULKAN_ERROR_OUT_OF_MEMORY:
        return "Out of memory: System or GPU memory exhausted. reduce resource usage.";
    case R_CVULKAN_ERROR_INVALID_ARGUMENT:
        return "Invalid argument: A function parameter was invalid. Check function call parameters.";
    case R_CVULKAN_ERROR_NULL_POINTER:
        return "Null pointer: A required pointer was NULL. Check initialization and parameter "
               "passing.";
    case R_CVULKAN_ERROR_NOT_INITIALIZED:
        return "Vulkan not initialized: Vulkan objects not properly initialized. Call initialization "
               "functions first.";
    case R_CVULKAN_ERROR_ALREADY_INITIALIZED:
        return "Already initialized: Attempted to initialize an already initialized object. Check "
               "initialization state.";
    case R_CVULKAN_ERROR_INSTANCE_CREATE_FAILED:
        return "Failed to create Vulkan instance: Instance creation failed. Check Vulkan driver "
               "installation and extensions.";
    case R_CVULKAN_ERROR_DEVICE_CREATE_FAILED:
        return "Failed to create Vulkan device: Device creation failed. Check physical device "
               "selection and queue families.";
    case R_CVULKAN_ERROR_EXTENSION_NOT_FOUND:
        return "Required extension not found: A required Vulkan extension is not available. Check "
               "device capabilities.";
    case R_CVULKAN_ERROR_LAYER_NOT_FOUND:
        return "Required layer not found: A required validation layer is not available. Check Vulkan "
               "SDK installation.";
    case R_CVULKAN_ERROR_PHYSICAL_DEVICE_NOT_FOUND:
        return "No suitable physical device: No GPU meets the requirements. Check hardware and "
               "driver support.";
    case R_CVULKAN_ERROR_QUEUE_FAMILY_NOT_FOUND:
        return "Required queue family not found: Required queue family not supported by device. "
               "Check device capabilities.";
    case R_CVULKAN_ERROR_SURFACE_CREATE_FAILED:
        return "Failed to create surface: Window surface creation failed. Check window handle and "
               "platform support.";
    case R_CVULKAN_ERROR_SWAPCHAIN_CREATE_FAILED:
        return "Failed to create swapchain: Swapchain creation failed. Check surface format and "
               "present mode support.";
    case R_CVULKAN_ERROR_SWAPCHAIN_OUT_OF_DATE:
        return "Swapchain out of date: Window was resized. Recreate swapchain with new dimensions.";
    case R_CVULKAN_ERROR_SWAPCHAIN_SUBOPTIMAL:
        return "Swapchain suboptimal: Current swapchain configuration is not optimal. Consider "
               "recreating.";
    case R_CVULKAN_ERROR_MAP_MEMORY_FAILED:
        return "Failed to map memory: Memory mapping failed. Check memory allocation and access "
               "permissions.";
    case R_CVULKAN_ERROR_UNMAP_MEMORY_FAILED:
        return "Failed to unmap memory: Memory unmapping failed. Check memory state and flush "
               "operations.";
    case R_CVULKAN_ERROR_FENCE_WAIT_FAILED:
        return "Failed to wait for fence: Fence wait operation failed. Check fence state and "
               "timeout.";
    case R_CVULKAN_ERROR_FENCE_RESET_FAILED:
        return "Failed to reset fence: Fence reset failed. Check fence state and signaling.";
    case R_CVULKAN_ERROR_BUFFER_CREATE_FAILED:
        return "Failed to create buffer: Buffer creation failed. Check buffer size, usage flags, and "
               "memory requirements.";
    case R_CVULKAN_ERROR_IMAGE_CREATE_FAILED:
        return "Failed to create image: Image creation failed. Check image format, extent, and "
               "memory requirements.";
    case R_CVULKAN_ERROR_MEMORY_ALLOCATE_FAILED:
        return "Failed to allocate memory: Memory allocation failed. Check memory type and available "
               "memory.";
    case R_CVULKAN_ERROR_COMMAND_POOL_CREATE_FAILED:
        return "Failed to create command pool: Command pool creation failed. Check queue family "
               "index and flags.";
    case R_CVULKAN_ERROR_COMMAND_BUFFER_BEGIN_FAILED:
        return "Failed to begin command buffer: Command buffer recording failed. Check buffer state "
               "and usage flags.";
    case R_CVULKAN_ERROR_COMMAND_BUFFER_END_FAILED:
        return "Failed to end command buffer: Command buffer recording end failed. Check recording "
               "state.";
    case R_CVULKAN_ERROR_RENDER_PASS_CREATE_FAILED:
        return "Failed to create render pass: Render pass creation failed. Check attachment "
               "descriptions and subpasses.";
    case R_CVULKAN_ERROR_FRAMEBUFFER_CREATE_FAILED:
        return "Failed to create framebuffer: Framebuffer creation failed. Check render pass "
               "compatibility and image views.";
    case R_CVULKAN_ERROR_SHADER_MODULE_CREATE_FAILED:
        return "Failed to create shader module: Shader compilation failed. Check shader source code "
               "and SPIR-V validity.";
    case R_CVULKAN_ERROR_PIPELINE_CREATE_FAILED:
        return "Failed to create pipeline: Pipeline creation failed. Check shaders, layout, and "
               "render pass compatibility.";
    case R_CVULKAN_ERROR_DESCRIPTOR_POOL_CREATE_FAILED:
        return "Failed to create descriptor pool: Descriptor pool creation failed. Check pool sizes "
               "and limits.";
    case R_CVULKAN_ERROR_DESCRIPTOR_SET_LAYOUT_CREATE_FAILED:
        return "Failed to create descriptor set layout: Layout creation failed. Check bindings and "
               "shader stages.";
    case R_CVULKAN_ERROR_SAMPLER_CREATE_FAILED:
        return "Failed to create sampler: Sampler creation failed. Check sampler parameters and "
               "device limits.";
    case R_CVULKAN_ERROR_DEVICE_LOST:
        return "Device lost: GPU device lost due to hardware error or timeout. Application must "
               "exit.";
    case R_CVULKAN_ERROR_SURFACE_LOST:
        return "Surface lost: Window surface was destroyed. Recreate surface and swapchain.";
    case R_CVULKAN_ERROR_OUT_OF_DATE_KHR:
        return "Out of date: Window resize or configuration change. Recreate swapchain.";
    case R_CVULKAN_ERROR_FEATURE_NOT_PRESENT:
        return "Feature not present: Required GPU feature not supported. Check device features.";
    case R_CVULKAN_ERROR_INCOMPATIBLE_DRIVER:
        return "Incompatible driver: Vulkan driver version incompatible. Update graphics driver.";
    case R_CVULKAN_ERROR_SURFACE_NOT_PRESENT:
        return "Surface not present: Surface not available for presentation. Check surface creation.";
    case R_CVULKAN_ERROR_UNKNOWN:
    default:
        return "Unknown error: An unrecognized error occurred. Check Vulkan result codes.";
    }
}

enum R_CVulkan_Error
r_cvulkan_result_to_error (const VkResult result)
{
    switch (result)
    {
    case VK_SUCCESS:
        return R_CVULKAN_OK;
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

R_CVULKAN_API const char*
r_cvulkan_result_to_string (const VkResult result)
{
    switch (result)
    {
    case VK_SUCCESS:
        return "Success";
    case VK_NOT_READY:
        return "Not ready";
    case VK_TIMEOUT:
        return "Timeout";
    case VK_EVENT_SET:
        return "Event set";
    case VK_EVENT_RESET:
        return "Event reset";
    case VK_INCOMPLETE:
        return "Incomplete";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "Out of host memory";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "Out of device memory";
    case VK_ERROR_INITIALIZATION_FAILED:
        return "Initialization failed";
    case VK_ERROR_DEVICE_LOST:
        return "Device lost";
    case VK_ERROR_MEMORY_MAP_FAILED:
        return "Memory map failed";
    case VK_ERROR_LAYER_NOT_PRESENT:
        return "Layer not present";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "Extension not present";
    case VK_ERROR_FEATURE_NOT_PRESENT:
        return "Feature not present";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return "Incompatible driver";
    case VK_ERROR_TOO_MANY_OBJECTS:
        return "Too many objects";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
        return "Format not supported";
    case VK_ERROR_FRAGMENTED_POOL:
        return "Fragmented pool";
    case VK_ERROR_OUT_OF_POOL_MEMORY:
        return "Out of pool memory";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
        return "Invalid external handle";
    case VK_ERROR_SURFACE_LOST_KHR:
        return "Surface lost";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        return "Native window in use";
    case VK_SUBOPTIMAL_KHR:
        return "Suboptimal";
    case VK_ERROR_OUT_OF_DATE_KHR:
        return "Out of date";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        return "Incompatible display";
    case VK_ERROR_VALIDATION_FAILED_EXT:
        return "Invalid shader code";
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
        return "Invalid DRM format modifier plane layout";
    case VK_ERROR_NOT_PERMITTED_KHR:
        return "Not permitted";
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
        return "Invalid opaque capture address";
    case VK_PIPELINE_COMPILE_REQUIRED:
        return "Pipeline compile required";
    case VK_ERROR_FRAGMENTATION:
        return "Memory fragmentation";
    default:
        return "Unknown Vulkan result";
    }
}

R_CVULKAN_API const char*
r_cvulkan_format_to_string (const VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_UNDEFINED:
        return "Undefined";
    case VK_FORMAT_R4G4_UNORM_PACK8:
        return "R4G4_UNORM_PACK8";
    case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
        return "R4G4B4A4_UNORM_PACK16";
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
        return "B4G4R4A4_UNORM_PACK16";
    case VK_FORMAT_R5G6B5_UNORM_PACK16:
        return "R5G6B5_UNORM_PACK16";
    case VK_FORMAT_B5G6R5_UNORM_PACK16:
        return "B5G6R5_UNORM_PACK16";
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
        return "R5G5B5A1_UNORM_PACK16";
    case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
        return "B5G5R5A1_UNORM_PACK16";
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
        return "A1R5G5B5_UNORM_PACK16";
    case VK_FORMAT_R8_UNORM:
        return "R8_UNORM";
    case VK_FORMAT_R8_SNORM:
        return "R8_SNORM";
    case VK_FORMAT_R8_USCALED:
        return "R8_USCALED";
    case VK_FORMAT_R8_SSCALED:
        return "R8_SSCALED";
    case VK_FORMAT_R8_UINT:
        return "R8_UINT";
    case VK_FORMAT_R8_SINT:
        return "R8_SINT";
    case VK_FORMAT_R8_SRGB:
        return "R8_SRGB";
    case VK_FORMAT_R8G8_UNORM:
        return "R8G8_UNORM";
    case VK_FORMAT_R8G8_SNORM:
        return "R8G8_SNORM";
    case VK_FORMAT_R8G8_USCALED:
        return "R8G8_USCALED";
    case VK_FORMAT_R8G8_SSCALED:
        return "R8G8_SSCALED";
    case VK_FORMAT_R8G8_UINT:
        return "R8G8_UINT";
    case VK_FORMAT_R8G8_SINT:
        return "R8G8_SINT";
    case VK_FORMAT_R8G8_SRGB:
        return "R8G8_SRGB";
    case VK_FORMAT_R8G8B8_UNORM:
        return "R8G8B8_UNORM";
    case VK_FORMAT_R8G8B8_SNORM:
        return "R8G8B8_SNORM";
    case VK_FORMAT_R8G8B8_USCALED:
        return "R8G8B8_USCALED";
    case VK_FORMAT_R8G8B8_SSCALED:
        return "R8G8B8_SSCALED";
    case VK_FORMAT_R8G8B8_UINT:
        return "R8G8B8_UINT";
    case VK_FORMAT_R8G8B8_SINT:
        return "R8G8B8_SINT";
    case VK_FORMAT_R8G8B8_SRGB:
        return "R8G8B8_SRGB";
    case VK_FORMAT_B8G8R8_UNORM:
        return "B8G8R8_UNORM";
    case VK_FORMAT_B8G8R8_SNORM:
        return "B8G8R8_SNORM";
    case VK_FORMAT_B8G8R8_USCALED:
        return "B8G8R8_USCALED";
    case VK_FORMAT_B8G8R8_SSCALED:
        return "B8G8R8_SSCALED";
    case VK_FORMAT_B8G8R8_UINT:
        return "B8G8R8_UINT";
    case VK_FORMAT_B8G8R8_SINT:
        return "B8G8R8_SINT";
    case VK_FORMAT_B8G8R8_SRGB:
        return "B8G8R8_SRGB";
    case VK_FORMAT_R8G8B8A8_UNORM:
        return "R8G8B8A8_UNORM";
    case VK_FORMAT_R8G8B8A8_SNORM:
        return "R8G8B8A8_SNORM";
    case VK_FORMAT_R8G8B8A8_USCALED:
        return "R8G8B8A8_USCALED";
    case VK_FORMAT_R8G8B8A8_SSCALED:
        return "R8G8B8A8_SSCALED";
    case VK_FORMAT_R8G8B8A8_UINT:
        return "R8G8B8A8_UINT";
    case VK_FORMAT_R8G8B8A8_SINT:
        return "R8G8B8A8_SINT";
    case VK_FORMAT_R8G8B8A8_SRGB:
        return "R8G8B8A8_SRGB";
    case VK_FORMAT_B8G8R8A8_UNORM:
        return "B8G8R8A8_UNORM";
    case VK_FORMAT_B8G8R8A8_SNORM:
        return "B8G8R8A8_SNORM";
    case VK_FORMAT_B8G8R8A8_USCALED:
        return "B8G8R8A8_USCALED";
    case VK_FORMAT_B8G8R8A8_SSCALED:
        return "B8G8R8A8_SSCALED";
    case VK_FORMAT_B8G8R8A8_UINT:
        return "B8G8R8A8_UINT";
    case VK_FORMAT_B8G8R8A8_SINT:
        return "B8G8R8A8_SINT";
    case VK_FORMAT_B8G8R8A8_SRGB:
        return "B8G8R8A8_SRGB";
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        return "A8B8G8R8_UNORM_PACK32";
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        return "A8B8G8R8_SNORM_PACK32";
    case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
        return "A8B8G8R8_USCALED_PACK32";
    case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
        return "A8B8G8R8_SSCALED_PACK32";
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:
        return "A8B8G8R8_UINT_PACK32";
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:
        return "A8B8G8R8_SINT_PACK32";
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        return "A8B8G8R8_SRGB_PACK32";
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        return "A2R10G10B10_UNORM_PACK32";
    case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
        return "A2R10G10B10_SNORM_PACK32";
    case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
        return "A2R10G10B10_USCALED_PACK32";
    case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
        return "A2R10G10B10_SSCALED_PACK32";
    case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        return "A2R10G10B10_UINT_PACK32";
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        return "A2R10G10B10_SINT_PACK32";
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        return "A2B10G10R10_UNORM_PACK32";
    case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
        return "A2B10G10R10_SNORM_PACK32";
    case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
        return "A2B10G10R10_USCALED_PACK32";
    case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
        return "A2B10G10R10_SSCALED_PACK32";
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
        return "A2B10G10R10_UINT_PACK32";
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
        return "A2B10G10R10_SINT_PACK32";
    case VK_FORMAT_R16_UNORM:
        return "R16_UNORM";
    case VK_FORMAT_R16_SNORM:
        return "R16_SNORM";
    case VK_FORMAT_R16_USCALED:
        return "R16_USCALED";
    case VK_FORMAT_R16_SSCALED:
        return "R16_SSCALED";
    case VK_FORMAT_R16_UINT:
        return "R16_UINT";
    case VK_FORMAT_R16_SINT:
        return "R16_SINT";
    case VK_FORMAT_R16_SFLOAT:
        return "R16_SFLOAT";
    case VK_FORMAT_R16G16_UNORM:
        return "R16G16_UNORM";
    case VK_FORMAT_R16G16_SNORM:
        return "R16G16_SNORM";
    case VK_FORMAT_R16G16_USCALED:
        return "R16G16_USCALED";
    case VK_FORMAT_R16G16_SSCALED:
        return "R16G16_SSCALED";
    case VK_FORMAT_R16G16_UINT:
        return "R16G16_UINT";
    case VK_FORMAT_R16G16_SINT:
        return "R16G16_SINT";
    case VK_FORMAT_R16G16_SFLOAT:
        return "R16G16_SFLOAT";
    case VK_FORMAT_R16G16B16_UNORM:
        return "R16G16B16_UNORM";
    case VK_FORMAT_R16G16B16_SNORM:
        return "R16G16B16_SNORM";
    case VK_FORMAT_R16G16B16_USCALED:
        return "R16G16B16_USCALED";
    case VK_FORMAT_R16G16B16_SSCALED:
        return "R16G16B16_SSCALED";
    case VK_FORMAT_R16G16B16_UINT:
        return "R16G16B16_UINT";
    case VK_FORMAT_R16G16B16_SINT:
        return "R16G16B16_SINT";
    case VK_FORMAT_R16G16B16_SFLOAT:
        return "R16G16B16_SFLOAT";
    case VK_FORMAT_R16G16B16A16_UNORM:
        return "R16G16B16A16_UNORM";
    case VK_FORMAT_R16G16B16A16_SNORM:
        return "R16G16B16A16_SNORM";
    case VK_FORMAT_R16G16B16A16_USCALED:
        return "R16G16B16A16_USCALED";
    case VK_FORMAT_R16G16B16A16_SSCALED:
        return "R16G16B16A16_SSCALED";
    case VK_FORMAT_R16G16B16A16_UINT:
        return "R16G16B16A16_UINT";
    case VK_FORMAT_R16G16B16A16_SINT:
        return "R16G16B16A16_SINT";
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return "R16G16B16A16_SFLOAT";
    case VK_FORMAT_R32_UINT:
        return "R32_UINT";
    case VK_FORMAT_R32_SINT:
        return "R32_SINT";
    case VK_FORMAT_R32_SFLOAT:
        return "R32_SFLOAT";
    case VK_FORMAT_R32G32_UINT:
        return "R32G32_UINT";
    case VK_FORMAT_R32G32_SINT:
        return "R32G32_SINT";
    case VK_FORMAT_R32G32_SFLOAT:
        return "R32G32_SFLOAT";
    case VK_FORMAT_R32G32B32_UINT:
        return "R32G32B32_UINT";
    case VK_FORMAT_R32G32B32_SINT:
        return "R32G32B32_SINT";
    case VK_FORMAT_R32G32B32_SFLOAT:
        return "R32G32B32_SFLOAT";
    case VK_FORMAT_R32G32B32A32_UINT:
        return "R32G32B32A32_UINT";
    case VK_FORMAT_R32G32B32A32_SINT:
        return "R32G32B32A32_SINT";
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return "R32G32B32A32_SFLOAT";
    case VK_FORMAT_R64_UINT:
        return "R64_UINT";
    case VK_FORMAT_R64_SINT:
        return "R64_SINT";
    case VK_FORMAT_R64_SFLOAT:
        return "R64_SFLOAT";
    case VK_FORMAT_R64G64_UINT:
        return "R64G64_UINT";
    case VK_FORMAT_R64G64_SINT:
        return "R64G64_SINT";
    case VK_FORMAT_R64G64_SFLOAT:
        return "R64G64_SFLOAT";
    case VK_FORMAT_R64G64B64_UINT:
        return "R64G64B64_UINT";
    case VK_FORMAT_R64G64B64_SINT:
        return "R64G64B64_SINT";
    case VK_FORMAT_R64G64B64_SFLOAT:
        return "R64G64B64_SFLOAT";
    case VK_FORMAT_R64G64B64A64_UINT:
        return "R64G64B64A64_UINT";
    case VK_FORMAT_R64G64B64A64_SINT:
        return "R64G64B64A64_SINT";
    case VK_FORMAT_R64G64B64A64_SFLOAT:
        return "R64G64B64A64_SFLOAT";
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        return "B10G11R11_UFLOAT_PACK32";
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
        return "E5B9G9R9_UFLOAT_PACK32";
    case VK_FORMAT_D16_UNORM:
        return "D16_UNORM";
    case VK_FORMAT_X8_D24_UNORM_PACK32:
        return "X8_D24_UNORM_PACK32";
    case VK_FORMAT_D32_SFLOAT:
        return "D32_SFLOAT";
    case VK_FORMAT_S8_UINT:
        return "S8_UINT";
    case VK_FORMAT_D16_UNORM_S8_UINT:
        return "D16_UNORM_S8_UINT";
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return "D24_UNORM_S8_UINT";
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return "D32_SFLOAT_S8_UINT";
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        return "BC1_RGB_UNORM_BLOCK";
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        return "BC1_RGB_SRGB_BLOCK";
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        return "BC1_RGBA_UNORM_BLOCK";
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
        return "BC1_RGBA_SRGB_BLOCK";
    case VK_FORMAT_BC2_UNORM_BLOCK:
        return "BC2_UNORM_BLOCK";
    case VK_FORMAT_BC2_SRGB_BLOCK:
        return "BC2_SRGB_BLOCK";
    case VK_FORMAT_BC3_UNORM_BLOCK:
        return "BC3_UNORM_BLOCK";
    case VK_FORMAT_BC3_SRGB_BLOCK:
        return "BC3_SRGB_BLOCK";
    case VK_FORMAT_BC4_UNORM_BLOCK:
        return "BC4_UNORM_BLOCK";
    case VK_FORMAT_BC4_SNORM_BLOCK:
        return "BC4_SNORM_BLOCK";
    case VK_FORMAT_BC5_UNORM_BLOCK:
        return "BC5_UNORM_BLOCK";
    case VK_FORMAT_BC5_SNORM_BLOCK:
        return "BC5_SNORM_BLOCK";
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        return "BC6H_UFLOAT_BLOCK";
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        return "BC6H_SFLOAT_BLOCK";
    case VK_FORMAT_BC7_UNORM_BLOCK:
        return "BC7_UNORM_BLOCK";
    case VK_FORMAT_BC7_SRGB_BLOCK:
        return "BC7_SRGB_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
        return "ETC2_R8G8B8_UNORM_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
        return "ETC2_R8G8B8_SRGB_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
        return "ETC2_R8G8B8A1_UNORM_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
        return "ETC2_R8G8B8A1_SRGB_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
        return "ETC2_R8G8B8A8_UNORM_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
        return "ETC2_R8G8B8A8_SRGB_BLOCK";
    case VK_FORMAT_EAC_R11_UNORM_BLOCK:
        return "EAC_R11_UNORM_BLOCK";
    case VK_FORMAT_EAC_R11_SNORM_BLOCK:
        return "EAC_R11_SNORM_BLOCK";
    case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
        return "EAC_R11G11_UNORM_BLOCK";
    case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
        return "EAC_R11G11_SNORM_BLOCK";
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
        return "ASTC_4x4_UNORM_BLOCK";
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
        return "ASTC_4x4_SRGB_BLOCK";
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
        return "ASTC_5x4_UNORM_BLOCK";
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
        return "ASTC_5x4_SRGB_BLOCK";
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
        return "ASTC_5x5_UNORM_BLOCK";
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
        return "ASTC_5x5_SRGB_BLOCK";
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
        return "ASTC_6x5_UNORM_BLOCK";
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
        return "ASTC_6x5_SRGB_BLOCK";
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
        return "ASTC_6x6_UNORM_BLOCK";
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
        return "ASTC_6x6_SRGB_BLOCK";
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
        return "ASTC_8x5_UNORM_BLOCK";
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
        return "ASTC_8x5_SRGB_BLOCK";
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
        return "ASTC_8x6_UNORM_BLOCK";
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
        return "ASTC_8x6_SRGB_BLOCK";
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
        return "ASTC_8x8_UNORM_BLOCK";
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
        return "ASTC_8x8_SRGB_BLOCK";
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
        return "ASTC_10x5_UNORM_BLOCK";
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
        return "ASTC_10x5_SRGB_BLOCK";
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
        return "ASTC_10x6_UNORM_BLOCK";
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
        return "ASTC_10x6_SRGB_BLOCK";
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
        return "ASTC_10x8_UNORM_BLOCK";
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
        return "ASTC_10x8_SRGB_BLOCK";
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
        return "ASTC_10x10_UNORM_BLOCK";
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
        return "ASTC_10x10_SRGB_BLOCK";
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
        return "ASTC_12x10_UNORM_BLOCK";
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
        return "ASTC_12x10_SRGB_BLOCK";
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
        return "ASTC_12x12_UNORM_BLOCK";
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
        return "ASTC_12x12_SRGB_BLOCK";
    default:
        return "Unknown VK_FORMAT";
    }
}
