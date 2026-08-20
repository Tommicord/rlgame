#include "rlgame.base/cvulkan/cvulkan_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

R_CVULKAN_API const char*
R_CVulkan_ErrorToString (const enum R_CVulkan_Error error)
{
        switch (error)
        {
        case R_CVULKAN_OK:
                return "Success: Operation completed successfully";
        case R_CVULKAN_ERROR_FAILED:
                return "General failure: An unspecified error occurred. Check logs for more details.";
        case R_CVULKAN_ERROR_OUT_OF_MEMORY:
                return "Out of memory: System or GPU memory exhausted. Close other applications or reduce "
                       "resource usage.";
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

R_CVULKAN_API enum R_CVulkan_ErrorSeverity
R_CVulkan_ErrorGetSeverity (const enum R_CVulkan_Error error)
{
        switch (error)
        {
        case R_CVULKAN_OK:
                return R_CVULKAN_SEVERITY_INFO;
        case R_CVULKAN_ERROR_SWAPCHAIN_SUBOPTIMAL:
        case R_CVULKAN_ERROR_OUT_OF_DATE_KHR:
                return R_CVULKAN_SEVERITY_WARNING;
        case R_CVULKAN_ERROR_FAILED:
        case R_CVULKAN_ERROR_OUT_OF_MEMORY:
        case R_CVULKAN_ERROR_INVALID_ARGUMENT:
        case R_CVULKAN_ERROR_NULL_POINTER:
        case R_CVULKAN_ERROR_NOT_INITIALIZED:
        case R_CVULKAN_ERROR_ALREADY_INITIALIZED:
        case R_CVULKAN_ERROR_INSTANCE_CREATE_FAILED:
        case R_CVULKAN_ERROR_DEVICE_CREATE_FAILED:
        case R_CVULKAN_ERROR_EXTENSION_NOT_FOUND:
        case R_CVULKAN_ERROR_LAYER_NOT_FOUND:
        case R_CVULKAN_ERROR_PHYSICAL_DEVICE_NOT_FOUND:
        case R_CVULKAN_ERROR_QUEUE_FAMILY_NOT_FOUND:
        case R_CVULKAN_ERROR_SURFACE_CREATE_FAILED:
        case R_CVULKAN_ERROR_SWAPCHAIN_CREATE_FAILED:
        case R_CVULKAN_ERROR_MAP_MEMORY_FAILED:
        case R_CVULKAN_ERROR_UNMAP_MEMORY_FAILED:
        case R_CVULKAN_ERROR_FENCE_WAIT_FAILED:
        case R_CVULKAN_ERROR_FENCE_RESET_FAILED:
        case R_CVULKAN_ERROR_BUFFER_CREATE_FAILED:
        case R_CVULKAN_ERROR_IMAGE_CREATE_FAILED:
        case R_CVULKAN_ERROR_MEMORY_ALLOCATE_FAILED:
        case R_CVULKAN_ERROR_COMMAND_POOL_CREATE_FAILED:
        case R_CVULKAN_ERROR_COMMAND_BUFFER_BEGIN_FAILED:
        case R_CVULKAN_ERROR_COMMAND_BUFFER_END_FAILED:
        case R_CVULKAN_ERROR_RENDER_PASS_CREATE_FAILED:
        case R_CVULKAN_ERROR_FRAMEBUFFER_CREATE_FAILED:
        case R_CVULKAN_ERROR_SHADER_MODULE_CREATE_FAILED:
        case R_CVULKAN_ERROR_PIPELINE_CREATE_FAILED:
        case R_CVULKAN_ERROR_DESCRIPTOR_POOL_CREATE_FAILED:
        case R_CVULKAN_ERROR_DESCRIPTOR_SET_LAYOUT_CREATE_FAILED:
        case R_CVULKAN_ERROR_SAMPLER_CREATE_FAILED:
        case R_CVULKAN_ERROR_FEATURE_NOT_PRESENT:
        case R_CVULKAN_ERROR_INCOMPATIBLE_DRIVER:
        case R_CVULKAN_ERROR_SURFACE_NOT_PRESENT:
        case R_CVULKAN_ERROR_UNKNOWN:
                return R_CVULKAN_SEVERITY_ERROR;
        case R_CVULKAN_ERROR_DEVICE_LOST:
        case R_CVULKAN_ERROR_SURFACE_LOST:
        case R_CVULKAN_ERROR_SWAPCHAIN_OUT_OF_DATE:
                return R_CVULKAN_SEVERITY_CRITICAL;
        default:
                return R_CVULKAN_SEVERITY_ERROR;
        }
}

R_CVULKAN_API enum R_CVulkan_ErrorRecovery
R_CVulkan_ErrorGetRecoveryAction (const enum R_CVulkan_Error error)
{
        switch (error)
        {
        case R_CVULKAN_OK:
                return R_CVULKAN_RECOVERY_NONE;
        case R_CVULKAN_ERROR_OUT_OF_MEMORY:
        case R_CVULKAN_ERROR_FENCE_WAIT_FAILED:
        case R_CVULKAN_ERROR_FENCE_RESET_FAILED:
                return R_CVULKAN_RECOVERY_RETRY;
        case R_CVULKAN_ERROR_SWAPCHAIN_OUT_OF_DATE:
        case R_CVULKAN_ERROR_OUT_OF_DATE_KHR:
        case R_CVULKAN_ERROR_SURFACE_LOST:
                return R_CVULKAN_RECOVERY_RESIZE;
        case R_CVULKAN_ERROR_SWAPCHAIN_SUBOPTIMAL:
                return R_CVULKAN_RECOVERY_RECREATE;
        case R_CVULKAN_ERROR_DEVICE_LOST:
                return R_CVULKAN_RECOVERY_EXIT;
        case R_CVULKAN_ERROR_FEATURE_NOT_PRESENT:
        case R_CVULKAN_ERROR_INCOMPATIBLE_DRIVER:
        case R_CVULKAN_ERROR_PHYSICAL_DEVICE_NOT_FOUND:
                return R_CVULKAN_RECOVERY_FALLBACK;
        default:
                return R_CVULKAN_RECOVERY_NONE;
        }
}

R_CVULKAN_API enum R_CVulkan_ErrorCategory
R_CVulkan_ErrorGetCategory (const enum R_CVulkan_Error error)
{
        switch (error)
        {
        case R_CVULKAN_OK:
                return R_CVULKAN_CATEGORY_SUCCESS;
        case R_CVULKAN_ERROR_OUT_OF_MEMORY:
        case R_CVULKAN_ERROR_MAP_MEMORY_FAILED:
        case R_CVULKAN_ERROR_UNMAP_MEMORY_FAILED:
        case R_CVULKAN_ERROR_MEMORY_ALLOCATE_FAILED:
                return R_CVULKAN_CATEGORY_MEMORY;
        case R_CVULKAN_ERROR_NOT_INITIALIZED:
        case R_CVULKAN_ERROR_ALREADY_INITIALIZED:
        case R_CVULKAN_ERROR_INSTANCE_CREATE_FAILED:
        case R_CVULKAN_ERROR_DEVICE_CREATE_FAILED:
        case R_CVULKAN_ERROR_INCOMPATIBLE_DRIVER:
                return R_CVULKAN_CATEGORY_INITIALIZATION;
        case R_CVULKAN_ERROR_SURFACE_CREATE_FAILED:
        case R_CVULKAN_ERROR_SWAPCHAIN_CREATE_FAILED:
        case R_CVULKAN_ERROR_BUFFER_CREATE_FAILED:
        case R_CVULKAN_ERROR_IMAGE_CREATE_FAILED:
        case R_CVULKAN_ERROR_COMMAND_POOL_CREATE_FAILED:
        case R_CVULKAN_ERROR_RENDER_PASS_CREATE_FAILED:
        case R_CVULKAN_ERROR_FRAMEBUFFER_CREATE_FAILED:
        case R_CVULKAN_ERROR_SHADER_MODULE_CREATE_FAILED:
        case R_CVULKAN_ERROR_PIPELINE_CREATE_FAILED:
        case R_CVULKAN_ERROR_DESCRIPTOR_POOL_CREATE_FAILED:
        case R_CVULKAN_ERROR_DESCRIPTOR_SET_LAYOUT_CREATE_FAILED:
        case R_CVULKAN_ERROR_SAMPLER_CREATE_FAILED:
                return R_CVULKAN_CATEGORY_RESOURCE;
        case R_CVULKAN_ERROR_INVALID_ARGUMENT:
        case R_CVULKAN_ERROR_NULL_POINTER:
        case R_CVULKAN_ERROR_PHYSICAL_DEVICE_NOT_FOUND:
        case R_CVULKAN_ERROR_QUEUE_FAMILY_NOT_FOUND:
        case R_CVULKAN_ERROR_FEATURE_NOT_PRESENT:
        case R_CVULKAN_ERROR_SURFACE_NOT_PRESENT:
                return R_CVULKAN_CATEGORY_STATE;
        case R_CVULKAN_ERROR_LAYER_NOT_FOUND:
        case R_CVULKAN_ERROR_EXTENSION_NOT_FOUND:
                return R_CVULKAN_CATEGORY_VALIDATION;
        case R_CVULKAN_ERROR_COMMAND_BUFFER_BEGIN_FAILED:
        case R_CVULKAN_ERROR_COMMAND_BUFFER_END_FAILED:
        case R_CVULKAN_ERROR_FENCE_WAIT_FAILED:
        case R_CVULKAN_ERROR_FENCE_RESET_FAILED:
        case R_CVULKAN_ERROR_SWAPCHAIN_OUT_OF_DATE:
        case R_CVULKAN_ERROR_SWAPCHAIN_SUBOPTIMAL:
        case R_CVULKAN_ERROR_OUT_OF_DATE_KHR:
        case R_CVULKAN_ERROR_SURFACE_LOST:
        case R_CVULKAN_ERROR_DEVICE_LOST:
                return R_CVULKAN_CATEGORY_RUNTIME;
        case R_CVULKAN_ERROR_FAILED:
        case R_CVULKAN_ERROR_UNKNOWN:
        default:
                return R_CVULKAN_CATEGORY_UNKNOWN;
        }
}

R_CVULKAN_API const char*
R_CVulkan_ErrorSeverityToString (const enum R_CVulkan_ErrorSeverity severity)
{
        switch (severity)
        {
        case R_CVULKAN_SEVERITY_INFO:
                return "INFO";
        case R_CVULKAN_SEVERITY_WARNING:
                return "WARNING";
        case R_CVULKAN_SEVERITY_ERROR:
                return "ERROR";
        case R_CVULKAN_SEVERITY_CRITICAL:
                return "CRITICAL";
        default:
                return "UNKNOWN";
        }
}

R_CVULKAN_API const char*
R_CVulkan_ErrorRecoveryToString (const enum R_CVulkan_ErrorRecovery recovery)
{
        switch (recovery)
        {
        case R_CVULKAN_RECOVERY_NONE:
                return "None";
        case R_CVULKAN_RECOVERY_RETRY:
                return "Retry operation";
        case R_CVULKAN_RECOVERY_RECREATE:
                return "Recreate resource";
        case R_CVULKAN_RECOVERY_RESIZE:
                return "Handle window resize";
        case R_CVULKAN_RECOVERY_EXIT:
                return "Exit application";
        case R_CVULKAN_RECOVERY_FALLBACK:
                return "Use fallback path";
        default:
                return "Unknown";
        }
}

R_CVULKAN_API const char*
R_CVulkan_ErrorCategoryToString (const enum R_CVulkan_ErrorCategory category)
{
        switch (category)
        {
        case R_CVULKAN_CATEGORY_SUCCESS:
                return "Success";
        case R_CVULKAN_CATEGORY_MEMORY:
                return "Memory";
        case R_CVULKAN_CATEGORY_INITIALIZATION:
                return "Initialization";
        case R_CVULKAN_CATEGORY_RESOURCE:
                return "Resource";
        case R_CVULKAN_CATEGORY_STATE:
                return "State";
        case R_CVULKAN_CATEGORY_VALIDATION:
                return "Validation";
        case R_CVULKAN_CATEGORY_RUNTIME:
                return "Runtime";
        case R_CVULKAN_CATEGORY_UNKNOWN:
        default:
                return "Unknown";
        }
}

R_CVULKAN_API int
R_CVulkan_ErrorFormatMessage (
    enum R_CVulkan_Error error,
    const char*          file,
    int                  line,
    const char*          function,
    char*                pBuffer,
    size_t               bufferSize)
{
        if (pBuffer == NULL || bufferSize == 0)
        {
                return -1;
        }

        const char* pErrorString = R_CVulkan_ErrorToString (error);
        const char* pSeverityString = R_CVulkan_ErrorSeverityToString (R_CVulkan_ErrorGetSeverity (error));
        const char* pCategoryString = R_CVulkan_ErrorCategoryToString (R_CVulkan_ErrorGetCategory (error));

        int written = 0;

        if (file && line > 0)
        {
                written += snprintf (pBuffer + written, bufferSize - (size_t)written, "[%s:%d] ", file, line);
        }

        if (function)
        {
                written += snprintf (pBuffer + written, bufferSize - (size_t)written, "%s(): ", function);
        }

        written += snprintf (
            pBuffer + written,
            bufferSize - (size_t)written,
            "[%s][%s] %s",
            pSeverityString,
            pCategoryString,
            pErrorString);

        return written;
}

enum R_CVulkan_Error
R_CVulkan_ResultToError (const VkResult result)
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
R_CVulkan_ResultToString (const VkResult result)
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
