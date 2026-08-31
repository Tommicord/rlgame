#include "rlgame.base/cvulkan/cvulkan_surface.h"
#include "rlgame.base/cvulkan/cvulkan_instance.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_trace.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

#if defined(R_CVULKAN_PLATFORM_ANDROID)
#include <android/native_window.h>
#endif

#if defined(R_CVULKAN_PLATFORM_LINUX)
#include <X11/Xlib.h>
#include <xcb/xcb.h>
#endif

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_NewSurface (
    struct R_CVulkan_Surface*                 pSurface,
    const struct r_cvulkan_surface_create_info* pCreateInfo)
{
    R_CVULKAN_ASSERT (pSurface);
    R_CVULKAN_ASSERT (pCreateInfo);
    R_CSTL_TRACE_SCOPE_CTX ("instance=%p", pCreateInfo ? pCreateInfo->pInstance : NULL);

    if (pCreateInfo->pInstance == NULL)
    {
        R_CSTL_LOG_ERROR (
            "Instance is NULL. Create instance using R_CVulkan_NewInstance before creating surface.");
        R_CSTL_TRACE_SCOPE_EXIT ();
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }
    VkInstance instance = r_cvulkan_instance_get_handle (pCreateInfo->pInstance);
    if (instance == VK_NULL_HANDLE)
    {
        R_CSTL_LOG_ERROR ("Instance handle is NULL despite being marked as initialized.");
        R_CSTL_TRACE_SCOPE_EXIT ();
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }
    pSurface->instance = instance;
    VkResult result = VK_ERROR_UNKNOWN;
#if defined(R_CVULKAN_PLATFORM_WINDOWS)
    if (!pCreateInfo->hInstance || !pCreateInfo->hWnd)
    {
        R_CSTL_TRACE_SCOPE_EXIT ();
        return R_CVULKAN_ERROR_INVALID_ARGUMENT;
    }

    VkWin32SurfaceCreateInfoKHR surfaceInfo = {0};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hinstance = pCreateInfo->hInstance;
    surfaceInfo.hwnd = pCreateInfo->hWnd;

    result = vkCreateWin32SurfaceKHR (instance, &surfaceInfo, NULL, &pSurface->handle);

#elif defined(R_CVULKAN_PLATFORM_LINUX)
    if (pCreateInfo->linuxBackend == R_CVULKAN_LINUX_BACKEND_WAYLAND)
    {
        if (!pCreateInfo->pDisplay || !pCreateInfo->pSurface)
        {
            R_CSTL_TRACE_SCOPE_EXIT ();
            return R_CVULKAN_ERROR_INVALID_ARGUMENT;
        }

        R_CSTL_LOG_INFO (
            "R_CVulkan_NewSurface: Creating Wayland surface with display=%p, surface=%p",
            (void*)pCreateInfo->pDisplay,
            (void*)pCreateInfo->pSurface);

        VkWaylandSurfaceCreateInfoKHR surfaceInfo = {0};
        surfaceInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        surfaceInfo.display = pCreateInfo->pDisplay;
        surfaceInfo.surface = pCreateInfo->pSurface;

        result = vkCreateWaylandSurfaceKHR (instance, &surfaceInfo, NULL, &pSurface->handle);

        if (result == VK_SUCCESS)
        {
            R_CSTL_LOG_INFO (
                "R_CVulkan_NewSurface: Wayland surface created, handle=%p",
                (void*)pSurface->handle);
        }
        else
        {
            R_CSTL_LOG_ERROR ("R_CVulkan_NewSurface: Failed to create Wayland surface, result=%d", result);
        }
    }
    else if (pCreateInfo->linuxBackend == R_CVULKAN_LINUX_BACKEND_X11)
    {
        if (!pCreateInfo->pX11Display || !pCreateInfo->x11Window)
        {
            R_CSTL_TRACE_SCOPE_EXIT ();
            return R_CVULKAN_ERROR_INVALID_ARGUMENT;
        }

        R_CSTL_LOG_INFO (
            "R_CVulkan_NewSurface: Creating X11 surface with display=%p, window=%lu",
            (void*)pCreateInfo->pX11Display,
            (unsigned long)pCreateInfo->x11Window);

        VkXlibSurfaceCreateInfoKHR surfaceInfo = {0};
        surfaceInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
        surfaceInfo.dpy = pCreateInfo->pX11Display;
        surfaceInfo.window = pCreateInfo->x11Window;

        result = vkCreateXlibSurfaceKHR (instance, &surfaceInfo, NULL, &pSurface->handle);

        if (result == VK_SUCCESS)
        {
            R_CSTL_LOG_INFO ("R_CVulkan_NewSurface: X11 surface created, handle=%p", (void*)pSurface->handle);
        }
        else
        {
            R_CSTL_LOG_ERROR ("R_CVulkan_NewSurface: Failed to create X11 surface, result=%d", result);
        }
    }
    else if (pCreateInfo->linuxBackend == R_CVULKAN_LINUX_BACKEND_XCB)
    {
        if (!pCreateInfo->pXCBConnection || !pCreateInfo->xcbWindow)
        {
            R_CSTL_TRACE_SCOPE_EXIT ();
            return R_CVULKAN_ERROR_INVALID_ARGUMENT;
        }

        R_CSTL_LOG_INFO (
            "R_CVulkan_NewSurface: Creating XCB surface with connection=%p, window=%u",
            (void*)pCreateInfo->pXCBConnection,
            pCreateInfo->xcbWindow);

        VkXcbSurfaceCreateInfoKHR surfaceInfo = {0};
        surfaceInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        surfaceInfo.connection = pCreateInfo->pXCBConnection;
        surfaceInfo.window = pCreateInfo->xcbWindow;

        result = vkCreateXcbSurfaceKHR (instance, &surfaceInfo, NULL, &pSurface->handle);

        if (result == VK_SUCCESS)
        {
            R_CSTL_LOG_INFO ("R_CVulkan_NewSurface: XCB surface created, handle=%p", (void*)pSurface->handle);
        }
        else
        {
            R_CSTL_LOG_ERROR ("R_CVulkan_NewSurface: Failed to create XCB surface, result=%d", result);
        }
    }
    else
    {
        R_CSTL_TRACE_SCOPE_EXIT ();
        return R_CVULKAN_ERROR_INVALID_ARGUMENT;
    }

#elif defined(R_CVULKAN_PLATFORM_ANDROID)
    if (!pCreateInfo->pWindow)
    {
        R_CSTL_TRACE_SCOPE_EXIT ();
        return R_CVULKAN_ERROR_INVALID_ARGUMENT;
    }

    VkAndroidSurfaceCreateInfoKHR surfaceInfo = {0};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.window = pCreateInfo->pWindow;

    result = vkCreateAndroidSurfaceKHR (instance, &surfaceInfo, NULL, &pSurface->handle);

#elif defined(R_CVULKAN_PLATFORM_MACOS)
    if (!pCreateInfo->pNSWindow)
    {
        R_CSTL_TRACE_SCOPE_EXIT ();
        return R_CVULKAN_ERROR_INVALID_ARGUMENT;
    }

    VkMetalSurfaceCreateInfoEXT surfaceInfo = {0};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    surfaceInfo.pLayer = pCreateInfo->pNSWindow;

    result = vkCreateMetalSurfaceEXT (instance, &surfaceInfo, NULL, &pSurface->handle);

#else
    R_CSTL_TRACE_SCOPE_EXIT ();
    return R_CVULKAN_ERROR_FEATURE_NOT_PRESENT;
#endif

    if (result != VK_SUCCESS)
    {
        R_CSTL_TRACE_SCOPE_EXIT ();
        return R_CVULKAN_ERROR_SURFACE_CREATE_FAILED;
    }
    R_CSTL_TRACE_SCOPE_EXIT ();
    return R_CVULKAN_OK;
}

R_CVULKAN_API void
R_CVulkan_DeleteSurface (struct R_CVulkan_Surface* pSurface)
{
    R_CVULKAN_ASSERT (pSurface);
    vkDestroySurfaceKHR (pSurface->instance, pSurface->handle, NULL);
#if defined(R_CVULKAN_DEBUG)
    pSurface->handle = VK_NULL_HANDLE;
    pSurface->instance = VK_NULL_HANDLE;
#endif
}

R_CVULKAN_API VkSurfaceKHR
r_cvulkan_surface_get_handle (const struct R_CVulkan_Surface* pSurface)
{
    R_CVULKAN_ASSERT (pSurface);
    return pSurface->handle;
}

R_CVULKAN_API VkInstance
r_cvulkan_surface_get_instance (const struct R_CVulkan_Surface* pSurface)
{
    R_CVULKAN_ASSERT (pSurface);
    return pSurface->instance;
}