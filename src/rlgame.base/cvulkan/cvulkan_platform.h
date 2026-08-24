#pragma once

#include <vulkan/vulkan.h>
#if defined(_WIN32) || defined(_WIN64)
#define R_CVULKAN_PLATFORM_WINDOWS 1
#elif defined(__linux__)
#define R_CVULKAN_PLATFORM_LINUX 1
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_MAC
#define R_CVULKAN_PLATFORM_MACOS 1
#endif
#elif defined(__ANDROID__)
#define R_CVULKAN_PLATFORM_ANDROID 1
#endif

#if defined(R_DEVMODE)
#define R_CVULKAN_DEBUG
#endif

#if defined(R_CVULKAN_DEBUG)
#include <assert.h>
#define R_CVULKAN_ASSERT(condition)        assert (condition)
#define R_CVULKAN_ALWAYS_ASSERT(condition) assert (condition)
#else
#define R_CVULKAN_ASSERT(condition)        ((void)0)
#define R_CVULKAN_ALWAYS_ASSERT(condition) ((void)0)
#endif

#if defined(R_COMPILER_MSVC)
#define R_CVULKAN_API_ATTR __forceinline
#elif defined(R_COMPILER_GCC) || defined(R_COMPILER_CLANG)
#define R_CVULKAN_API_ATTR __attribute__ ((always_inline)) inline
#else
#define R_CVULKAN_API_ATTR inline
#endif

#if defined(_WIN32)
#ifdef R_CVULKAN_BUILDING_DLL
#define R_CVULKAN_API __declspec (dllexport)
#else
#define R_CVULKAN_API __declspec (dllimport)
#endif
#else
#define R_CVULKAN_API
#endif

#if defined(R_CVULKAN_DEBUG)
#include <stdbool.h>
#define R_CVULKAN_DEBUG_FIELD                 bool booted;
#define R_CVULKAN_IS_INITIALIZED_RETURN(pObj) ((pObj)->booted)

#define R_CVULKAN_VALIDATE_PARAM(ptr)                                                                        \
    do                                                                                                       \
    {                                                                                                        \
        R_CVULKAN_ASSERT (ptr);                                                                              \
        if (!(ptr))                                                                                          \
        {                                                                                                    \
            return R_CVULKAN_ERROR_NULL_POINTER;                                                             \
        }                                                                                                    \
    } while (0)

#define R_CVULKAN_VALIDATE_PARAM_BOOTED(obj)                                                                 \
    do                                                                                                       \
    {                                                                                                        \
        R_CVULKAN_ASSERT ((obj));                                                                            \
        R_CVULKAN_ASSERT ((obj)->booted);                                                                    \
        if (!(obj) || !(obj)->booted)                                                                        \
        {                                                                                                    \
            return R_CVULKAN_ERROR_NOT_INITIALIZED;                                                          \
        }                                                                                                    \
    } while (0)

#define R_CVULKAN_VALIDATE_COMMAND_BUFFER(cmdBuf)                                                            \
    do                                                                                                       \
    {                                                                                                        \
        R_CVULKAN_ASSERT ((cmdBuf));                                                                         \
        R_CVULKAN_ASSERT ((cmdBuf)->booted);                                                                 \
        R_CVULKAN_ASSERT ((cmdBuf)->record);                                                                 \
        if (!(cmdBuf) || !(cmdBuf)->booted || !(cmdBuf)->record)                                             \
        {                                                                                                    \
            return R_CVULKAN_ERROR_NOT_INITIALIZED;                                                          \
        }                                                                                                    \
    } while (0)

#define R_CVULKAN_VALIDATE_GETTER(ptr)                                                                       \
    do                                                                                                       \
    {                                                                                                        \
        R_CVULKAN_ASSERT ((ptr));                                                                            \
    } while (0)

#else
#define R_CVULKAN_DEBUG_FIELD
#define R_CVULKAN_IS_INITIALIZED_RETURN(pObj) (1)
#define R_CVULKAN_VALIDATE_PARAM(ptr)                                                                        \
    do                                                                                                       \
    {                                                                                                        \
        if (!(ptr))                                                                                          \
        {                                                                                                    \
            return R_CVULKAN_ERROR_NULL_POINTER;                                                             \
        }                                                                                                    \
    } while (0)
#define R_CVULKAN_VALIDATE_PARAM_BOOTED(obj)      ((void)0)
#define R_CVULKAN_VALIDATE_COMMAND_BUFFER(cmdBuf) ((void)0)
#define R_CVULKAN_VALIDATE_GETTER(ptr)            ((void)0)
#endif

/**
 * @brief R_CVulkan wrapper error codes
 */
enum R_CVulkanError
{
    R_CVULKAN_OK = 0, /**< Success */
    R_CVULKAN_ERROR_FAILED = -1, /**< General failure */
    R_CVULKAN_ERROR_OUT_OF_MEMORY = -2, /**< Memory allocation failed */
    R_CVULKAN_ERROR_INVALID_ARGUMENT = -3, /**< Invalid function argument */
    R_CVULKAN_ERROR_NULL_POINTER = -4, /**< Null pointer passed */
    R_CVULKAN_ERROR_NOT_INITIALIZED = -5, /**< Vulkan not initialized */
    R_CVULKAN_ERROR_ALREADY_INITIALIZED = -6, /**< Already initialized */
    R_CVULKAN_ERROR_INSTANCE_CREATE_FAILED = -7, /**< Failed to create Vulkan instance */
    R_CVULKAN_ERROR_DEVICE_CREATE_FAILED = -8, /**< Failed to create Vulkan device */
    R_CVULKAN_ERROR_EXTENSION_NOT_FOUND = -9, /**< Required extension not found */
    R_CVULKAN_ERROR_LAYER_NOT_FOUND = -10, /**< Required layer not found */
    R_CVULKAN_ERROR_PHYSICAL_DEVICE_NOT_FOUND = -11, /**< No suitable physical device */
    R_CVULKAN_ERROR_QUEUE_FAMILY_NOT_FOUND = -12, /**< Required queue family not found */
    R_CVULKAN_ERROR_SURFACE_CREATE_FAILED = -13, /**< Failed to create surface */
    R_CVULKAN_ERROR_SWAPCHAIN_CREATE_FAILED = -14, /**< Failed to create swapchain */
    R_CVULKAN_ERROR_SWAPCHAIN_OUT_OF_DATE = -15, /**< Swapchain is out of date, needs recreation */
    R_CVULKAN_ERROR_SWAPCHAIN_SUBOPTIMAL = -16, /**< Swapchain is suboptimal, can recreate */
    R_CVULKAN_ERROR_MAP_MEMORY_FAILED = -17, /**< Failed to map memory */
    R_CVULKAN_ERROR_UNMAP_MEMORY_FAILED = -18, /**< Failed to unmap memory */
    R_CVULKAN_ERROR_FENCE_WAIT_FAILED = -19, /**< Failed to wait for fence */
    R_CVULKAN_ERROR_FENCE_RESET_FAILED = -20, /**< Failed to reset fence */
    R_CVULKAN_ERROR_BUFFER_CREATE_FAILED = -21, /**< Failed to create buffer */
    R_CVULKAN_ERROR_IMAGE_CREATE_FAILED = -22, /**< Failed to create image */
    R_CVULKAN_ERROR_MEMORY_ALLOCATE_FAILED = -23, /**< Failed to allocate memory */
    R_CVULKAN_ERROR_COMMAND_POOL_CREATE_FAILED = -24, /**< Failed to create command pool */
    R_CVULKAN_ERROR_COMMAND_BUFFER_BEGIN_FAILED = -25, /**< Failed to begin command buffer */
    R_CVULKAN_ERROR_COMMAND_BUFFER_END_FAILED = -26, /**< Failed to end command buffer */
    R_CVULKAN_ERROR_RENDER_PASS_CREATE_FAILED = -27, /**< Failed to create render pass */
    R_CVULKAN_ERROR_FRAMEBUFFER_CREATE_FAILED = -28, /**< Failed to create framebuffer */
    R_CVULKAN_ERROR_SHADER_MODULE_CREATE_FAILED = -29, /**< Failed to create shader module */
    R_CVULKAN_ERROR_PIPELINE_CREATE_FAILED = -30, /**< Failed to create pipeline */
    R_CVULKAN_ERROR_DESCRIPTOR_POOL_CREATE_FAILED = -31, /**< Failed to create descriptor pool */
    R_CVULKAN_ERROR_DESCRIPTOR_SET_LAYOUT_CREATE_FAILED = -32, /**< Failed to create descriptor set layout */
    R_CVULKAN_ERROR_SAMPLER_CREATE_FAILED = -33, /**< Failed to create sampler */
    R_CVULKAN_ERROR_DEVICE_LOST = -34, /**< Device lost, must exit immediately */
    R_CVULKAN_ERROR_SURFACE_LOST = -35, /**< Surface lost, needs recreation */
    R_CVULKAN_ERROR_OUT_OF_DATE_KHR = -36, /**< Out of date (window resize) */
    R_CVULKAN_ERROR_FEATURE_NOT_PRESENT = -37, /**< Required feature not present */
    R_CVULKAN_ERROR_INCOMPATIBLE_DRIVER = -38, /**< Incompatible driver */
    R_CVULKAN_ERROR_SURFACE_NOT_PRESENT = -39, /**< Surface not present */
    R_CVULKAN_ERROR_INCOMPLETE = -40, /**< Incomplete operation */
    R_CVULKAN_ERROR_UNKNOWN = -99 /**< Unknown error */
};

/**
 * @brief Get human-readable error message for an R_CVulkan error code
 * @param error The R_CVulkan error code
 * @return Static string describing the error, or "Unknown error" if not recognized
 */
R_CVULKAN_API const char* R_CVulkanErrorToString (enum R_CVulkanError error);

/**
 * @brief Get human-readable string for error category
 * @param category The error category
 * @return Static string describing the error category
 */
R_CVULKAN_API const char* R_CVulkanErrorCategoryToString (enum R_CVulkanErrorCategory category);

/**
 * @brief Convert Vulkan result to R_CVulkan errorcode
 * @param result Vulkan result code
 * @return Corresponding R_CVulkan error code
 */
R_CVULKAN_API enum R_CVulkanError R_CVulkan_ResultToError (const VkResult result);

/**
 * @brief Get human-readable string for Vulkan result code
 * @param result Vulkan result code
 * @return Static string describing the Vulkan result
 */
R_CVULKAN_API const char* R_CVulkan_ResultToString (const VkResult result);

/**
 * @brief Get human-readable string for Vulkan format
 * @param format Vulkan format
 * @return Static string describing the Vulkan format
 */
R_CVULKAN_API const char* R_CVulkan_FormatToString (const VkFormat format);
