#pragma once

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <inttypes.h>

#include "rlgame.base/cvulkan/cvulkan_platform.h"

#if defined(R_CVULKAN_PLATFORM_LINUX)
#include <wayland-client.h>
#endif

struct R_CVulkan_Instance;

/**
 * @file cvulkan_surface.h
 * @brief Vulkan surface wrapper for presentation
 *
 * This module provides a safe wrapper for VkSurfaceKHR, which represents
 * a window or display surface for rendering. The surface is platform-specific
 * and is created from native window handles.
 * The instance must be created separately using R_CVulkan_NewInstance before creating a surface.
 */

/**
 * @brief Configuration parameters for surface creation
 */
struct R_CVulkan_SurfaceCreateInfo
{
        const struct R_CVulkan_Instance* pInstance; /**< Vulkan instance (required) */
#if defined(R_CVULKAN_PLATFORM_WINDOWS)
        HINSTANCE hInstance; /**< Windows instance handle */
        HWND      hWnd; /**< Windows window handle */
#elif defined(R_CVULKAN_PLATFORM_LINUX)
        struct wl_display* pDisplay; /**< Wayland display connection */
        struct wl_surface* pSurface; /**< Wayland surface */
#elif defined(R_CVULKAN_PLATFORM_ANDROID)
        ANativeWindow* pWindow; /**< Android native window */
#elif defined(R_CVULKAN_PLATFORM_MACOS)
        void* pNSWindow; /**< macOS NSWindow pointer */
#endif
};

/**
 * @brief Safe wrapper for VkSurfaceKHR
 */
struct R_CVulkan_Surface
{
        VkSurfaceKHR handle; /**< Raw Vulkan surface handle */
        VkInstance   instance; /**< Associated Vulkan instance */
        R_CVULKAN_DEBUG_FIELD
};

/**
 * @brief Initialize a surface from native window handle
 * @param pSurface Pointer to surface to initialize
 * @param pCreateInfo Surface creation parameters (must include valid instance)
 * @return R_CVULKAN_OK on success, error code otherwise
 *
 * This function creates a Vulkan surface from a native window handle.
 * The instance must be created separately using R_CVulkan_NewInstance before calling this function.
 *
 * Common errors:
 * - R_CVULKAN_ERROR_NOT_INITIALIZED: Instance not initialized
 * - R_CVULKAN_ERROR_SURFACE_CREATE_FAILED: Failed to create surface (check window handles)
 */
R_CVULKAN_API enum R_CVulkanError R_CVulkan_NewSurface (
    struct R_CVulkan_Surface*                 pSurface,
    const struct R_CVulkan_SurfaceCreateInfo* pCreateInfo);

/**
 * @brief Delete a surface and destroy the Vulkan object
 * @param pSurface Pointer to surface to delete
 */
R_CVULKAN_API void R_CVulkan_DeleteSurface (struct R_CVulkan_Surface* pSurface);

/**
 * @brief Get the raw Vulkan surface handle
 * @param pSurface Pointer to surface
 * @return Vulkan surface handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkSurfaceKHR R_CVulkan_SurfaceGetHandle (const struct R_CVulkan_Surface* pSurface);

/**
 * @brief Get the associated Vulkan instance
 * @param pSurface Pointer to surface
 * @return Vulkan instance handle, or VK_NULL_HANDLE if not initialized
 */
R_CVULKAN_API VkInstance R_CVulkan_SurfaceGetInstance (const struct R_CVulkan_Surface* pSurface);

/**
 * @brief Check if the surface is initialized
 * @param pSurface Pointer to surface
 * @return 1 if initialized, 0 otherwise
 */
R_CVULKAN_API int R_CVulkan_SurfaceIsInitialized (const struct R_CVulkan_Surface* pSurface);
