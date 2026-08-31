#include "rlgame.base/main_platform_handle.h"
#include "rlgame.base/main.h"
#include "rlgame.base/main_platform.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_log.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <vulkan/vulkan.h>

static uint16_t                   s_initialWidth = 784;
static uint16_t                   s_initialHeight = 512;
static enum r_window_handle_backend g_currentBackend = R_WINDOW_BACKEND_NONE;

static struct
{
        Display* display;
        Window   window;
        int      screen;
        Atom     wmDeleteMessage;
        uint16_t styleFlags; ///< Window decoration flags (bit-packed)
} g_x11State = {NULL, 0, 0, 0, 0};

static struct
{
        xcb_connection_t* connection;
        xcb_window_t      window;
        xcb_screen_t*     screen;
        uint16_t          styleFlags; ///< Window decoration flags (bit-packed)
} g_xcbState = {NULL, 0, NULL, 0};

// Helper function to check if Wayland is available
static bool
r_is_wayland_available (void)
{
    // Check if WAYLAND_DISPLAY is set
    const char* waylandDisplay = getenv ("WAYLAND_DISPLAY");
    if (waylandDisplay && strlen (waylandDisplay) > 0)
    {
        return true;
    }

    // Try to connect to Wayland display
    struct wl_display* display = wl_display_connect (NULL);
    if (display)
    {
        wl_display_disconnect (display);
        return true;
    }

    return false;
}

// Helper function to check if X11 is available
static bool
r_is_x11_available (void)
{
    // Try to connect to X11 display
    Display* display = XOpenDisplay (NULL);
    if (display)
    {
        XCloseDisplay (display);
        return true;
    }
    return false;
}

// Helper function to check if XCB is available
static bool
r_isXCBAvailable (void)
{
    // Try to connect to XCB
    xcb_connection_t* connection = xcb_connect (NULL, NULL);
    if (connection && !xcb_connection_has_error (connection))
    {
        xcb_disconnect (connection);
        return true;
    }
    if (connection)
    {
        xcb_disconnect (connection);
    }
    return false;
}

// Helper function to detect GPU capabilities using Vulkan
static bool
r_detect_vulkan_capabilities (struct r_capabilities* pCapabilities)
{
    if (!pCapabilities) return false;
    VkInstance           instance = VK_NULL_HANDLE;
    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    VkResult result = vkCreateInstance (&createInfo, NULL, &instance);
    if (result != VK_SUCCESS)
    {
        R_CSTL_LOG_ERROR ("Failed to create Vulkan instance for GPU detection: %d", result);
        return false;
    }
    uint32_t deviceCount = 0;
    result = vkEnumeratePhysicalDevices (instance, &deviceCount, NULL);
    if (result != VK_SUCCESS || deviceCount == 0)
    {
        R_CSTL_LOG_ERROR ("No Vulkan physical devices found");
        vkDestroyInstance (instance, NULL);
        return false;
    }
    VkPhysicalDevice* devices = (VkPhysicalDevice*)R_CSTL_HeapAlloc (sizeof (VkPhysicalDevice) * deviceCount);
    if (!devices)
    {
        vkDestroyInstance (instance, NULL);
        return false;
    }

    result = vkEnumeratePhysicalDevices (instance, &deviceCount, devices);
    if (result != VK_SUCCESS)
    {
        R_CSTL_HeapFree (devices);
        vkDestroyInstance (instance, NULL);
        return false;
    }

    // Get properties of the first device
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties (devices[0], &deviceProperties);

    // Extract Vulkan version
    uint32_t vulkanVersion = deviceProperties.apiVersion;
    pCapabilities->vulkanVersion = vulkanVersion;

    // Store GPU name
    pCapabilities->pVendor = deviceProperties.deviceName;

    // Determine if GPU is "modern" based on various criteria
    // Modern GPUs typically have:
    // - Vulkan 1.0+ (we already have Vulkan support)
    // - Dedicated GPU (not integrated)
    // - Recent driver version
    // - Good feature support
    bool isDiscrete = (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
    bool isIntegrated = (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);

    // Check for modern Intel integrated GPUs
    // Modern Intel integrated GPUs (good Vulkan/Wayland support):
    // - UHD Graphics 600+ (Gemini Lake, 2017+)
    // - Iris Plus Graphics 640/650/655 (Kaby Lake, 2017+)
    // - Iris Xe Graphics (Tiger Lake 11th gen+, 2020+)
    // - UHD Graphics Xe (11th gen+, 2020+)
    // - Arc Graphics (discrete, 2022+)
    // Older GPUs to EXCLUDE:
    // - HD Graphics 4000-630 (Ivy Bridge through Skylake, 2012-2015)
    // - HD Graphics 500-505 (Braswell/Cherry Trail, 2015) - too weak
    bool isModernIntel = false;
    if (isIntegrated && strstr (deviceProperties.deviceName, "Intel"))
    {
        const char* name = deviceProperties.deviceName;

        // Check for explicitly modern Intel GPUs
        if (strstr (name, "Iris Xe") || strstr (name, "Iris Plus") || strstr (name, "Arc"))
        {
            isModernIntel = true;
        }
        // Check for UHD Graphics 600+ (but not HD Graphics)
        else if (strstr (name, "UHD Graphics"))
        {
            // Extract model number after "UHD Graphics"
            const char* modelStr = strstr (name, "UHD Graphics");
            if (modelStr)
            {
                modelStr += strlen ("UHD Graphics");
                while (*modelStr == ' ' || *modelStr == 'G')
                    modelStr++;
                int modelNum = atoi (modelStr);
                // UHD Graphics 600+ are modern (Gemini Lake 2017+, Tiger Lake 2020+)
                if (modelNum >= 600)
                {
                    isModernIntel = true;
                }
            }
        }
        // Check for HD Graphics - only 600+ (Kaby Lake 7th gen+) might be acceptable
        // But HD Graphics 530 (Skylake 6th gen) is too old
        else if (strstr (name, "HD Graphics"))
        {
            const char* modelStr = strstr (name, "HD Graphics");
            if (modelStr)
            {
                modelStr += strlen ("HD Graphics");
                while (*modelStr == ' ' || *modelStr == 'P' || *modelStr == 'G')
                    modelStr++;
                int modelNum = atoi (modelStr);
                // HD Graphics 600+ (Kaby Lake 7th gen+) - borderline but might work
                // HD Graphics 500-505 (Apollo Lake) - too weak
                // HD Graphics 530 (Skylake) - too old
                if (modelNum >= 600)
                {
                    isModernIntel = true;
                }
            }
        }
    }

    // Check for AMD modern GPUs (GCN 1.0+)
    bool isModernAMD = false;
    if (strstr (deviceProperties.deviceName, "AMD") || strstr (deviceProperties.deviceName, "Radeon"))
    {
        // Most AMD GPUs with Vulkan support are reasonably modern
        isModernAMD = true;
    }

    // Check for NVIDIA modern GPUs (Kepler+)
    bool isModernNVIDIA = false;
    if (strstr (deviceProperties.deviceName, "NVIDIA") || strstr (deviceProperties.deviceName, "GeForce"))
    {
        // Most NVIDIA GPUs with Vulkan support are reasonably modern
        isModernNVIDIA = true;
    }

    // Determine if GPU is modern
    pCapabilities->looksModernGpu = isDiscrete || isModernIntel || isModernAMD || isModernNVIDIA;

    R_CSTL_LOG_INFO ("GPU Detection:");
    R_CSTL_LOG_INFO ("  GPU Name: %s", deviceProperties.deviceName);
    R_CSTL_LOG_INFO ("  GPU Type: %s", isDiscrete ? "Discrete" : (isIntegrated ? "Integrated" : "Other"));
    R_CSTL_LOG_INFO (
        "  Vulkan Version: %u.%u.%u",
        VK_VERSION_MAJOR (vulkanVersion),
        VK_VERSION_MINOR (vulkanVersion),
        VK_VERSION_PATCH (vulkanVersion));
    R_CSTL_LOG_INFO ("  Looks Modern GPU: %s", pCapabilities->looksModernGpu ? "Yes" : "No");

    R_CSTL_HeapFree (devices);
    vkDestroyInstance (instance, NULL);

    return true;
}

R_ENTRY_API enum r_window_handle_backend
r_detect_capabilities (struct r_capabilities* pCapabilities)
{
    if (!pCapabilities)
    {
        return R_WINDOW_BACKEND_NONE;
    }

    // Initialize capabilities
    memset (pCapabilities, 0, sizeof (struct r_capabilities));

    // Check platform availability
    pCapabilities->hasWaylandSupport = r_is_wayland_available ();
    pCapabilities->hasX11Support = r_is_x11_available ();
    pCapabilities->hasXCBSupport = r_isXCBAvailable ();

    R_CSTL_LOG_INFO ("Platform Availability:");
    R_CSTL_LOG_INFO ("  Wayland: %s", pCapabilities->hasWaylandSupport ? "Available" : "Not available");
    R_CSTL_LOG_INFO ("  X11: %s", pCapabilities->hasX11Support ? "Available" : "Not available");
    R_CSTL_LOG_INFO ("  XCB: %s", pCapabilities->hasXCBSupport ? "Available" : "Not available");

    // Detect GPU capabilities
    bool gpuDetectionSuccess = r_detect_vulkan_capabilities (pCapabilities);
    if (!gpuDetectionSuccess)
    {
        R_CSTL_LOG_WARN ("GPU detection failed, assuming legacy GPU");
        pCapabilities->looksModernGpu = false;
    }

    // Choose backend based on GPU modernity and platform availability
    enum r_window_handle_backend chosenBackend = R_WINDOW_BACKEND_NONE;

    if (pCapabilities->looksModernGpu && pCapabilities->hasWaylandSupport)
    {
        // Modern GPU with Wayland support - use Wayland
        chosenBackend = R_WINDOW_BACKEND_WAYLAND;
        R_CSTL_LOG_INFO ("Chosen backend: Wayland (modern GPU)");
    }
    else if (pCapabilities->hasX11Support)
    {
        // Legacy GPU or no Wayland, here use X11
        chosenBackend = R_WINDOW_BACKEND_X11;
        R_CSTL_LOG_INFO ("Chosen backend: X11 (legacy GPU or no Wayland)");
    }
    else if (pCapabilities->hasXCBSupport)
    {
        // Fallback to XCB
        chosenBackend = R_WINDOW_BACKEND_XCB;
        R_CSTL_LOG_INFO ("Chosen backend: XCB (fallback)");
    }
    else
    {
        // No suitable backend found
        R_CSTL_LOG_ERROR ("No suitable window backend found");
        chosenBackend = R_WINDOW_BACKEND_NONE;
    }
    g_currentBackend = chosenBackend;
    return chosenBackend;
}

#if defined(_WIN32)
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

static R_WIN32_HWND g_hwnd = NULL;

R_ENTRY_API HWND
r_get_window_handle (void)
{
    return g_hwnd;
}

LRESULT CALLBACK
r_window_handle_proc (R_WIN32_HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        g_hwnd = hwnd;
        return 0;
    case WM_DESTROY:
        g_hwnd = NULL;
        PostQuitMessage (0);
        return 0;
    default:
        return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
}

R_ENTRY_API void
r_window_handle_center (R_WIN32_HWND hwnd)
{
    RECT rc;
    GetWindowRect (hwnd, &rc);
    HMONITOR hMonitor = MonitorFromWindow (hwnd, MONITOR_DEFAULTTONEAREST);

    MONITORINFO mi = {sizeof (mi)};
    if (GetMonitorInfo (hMonitor, &mi))
    {
        int monitorWidth = mi.rcWork.right - mi.rcWork.left;
        int monitorHeight = mi.rcWork.bottom - mi.rcWork.top;

        int windowWidth = rc.right - rc.left;
        int windowHeight = rc.bottom - rc.top;
        int xPos = mi.rcWork.left + (monitorWidth - windowWidth) / 2;
        int yPos = mi.rcWork.top + (monitorHeight - windowHeight) / 2;
        SetWindowPos (hwnd, HWND_TOP, xPos, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
}

R_ENTRY_API int
r_init_win_main (R_WIN32_HINSTANCE hInstance, struct r_application_info* pApplicationInfo, int nCmdShow)
{
    const char* CLASS_NAME = "GameWindowClass";
    WNDCLASSA   wc = {0};
    wc.lpfnWndProc = r_window_handle_proc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    if (!RegisterClassA (&wc)) goto r_fail_init;
    if (!pApplicationInfo) goto r_fail_init;

    const char* pAppName = R_CSTL_StringData (pApplicationInfo->pApplicationName);

    HWND hwnd = CreateWindowExA (
        0,
        CLASS_NAME,
        pAppName,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        s_initialWidth,
        s_initialHeight,
        NULL,
        NULL,
        hInstance,
        NULL);
    r_window_handle_center (hwnd);
    if (!hwnd) goto r_fail_init;
    ShowWindow (hwnd, nCmdShow);
    return 1;
r_fail_init:
    R_CSTL_LOG_ERROR ("r_init_win_main: Failed to initialize WinMain");
    return 0;
}

R_ENTRY_API void
r_window_handle_set_fullscreen (R_WIN32_HWND hwnd, bool fullscreen)
{
    if (!hwnd) return;

    static WINDOWPLACEMENT wpPrev = {sizeof (WINDOWPLACEMENT)};
    DWORD                  dwStyle = GetWindowLong (hwnd, GWL_STYLE);

    if (fullscreen)
    {
        MONITORINFO mi = {sizeof (MONITORINFO)};
        if (GetWindowPlacement (hwnd, &wpPrev)
            && GetMonitorInfo (MonitorFromWindow (hwnd, MONITOR_DEFAULTTONEAREST), &mi))
        {
            SetWindowLong (hwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos (
                hwnd,
                HWND_TOP,
                mi.rcMonitor.left,
                mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong (hwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement (hwnd, &wpPrev);
        SetWindowPos (
            hwnd,
            NULL,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

R_ENTRY_API void
r_window_handle_set_borderless (R_WIN32_HWND hwnd, bool borderless)
{
    if (!hwnd) return;
    DWORD dwStyle = GetWindowLong (hwnd, GWL_STYLE);
    if (borderless)
    {
        SetWindowLong (
            hwnd,
            GWL_STYLE,
            dwStyle & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU));
    }
    else
    {
        SetWindowLong (hwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
    }
    SetWindowPos (hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

R_ENTRY_API void
r_window_handle_set_resizable (R_WIN32_HWND hwnd, bool resizable)
{
    if (!hwnd) return;
    DWORD dwStyle = GetWindowLong (hwnd, GWL_STYLE);
    if (resizable)
    {
        SetWindowLong (hwnd, GWL_STYLE, dwStyle | WS_THICKFRAME | WS_MAXIMIZEBOX);
    }
    else
    {
        SetWindowLong (hwnd, GWL_STYLE, dwStyle & ~(WS_THICKFRAME | WS_MAXIMIZEBOX));
    }
    SetWindowPos (hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

R_ENTRY_API void
r_window_handle_minimize (R_WIN32_HWND hwnd)
{
    if (hwnd) ShowWindow (hwnd, SW_MINIMIZE);
}

R_ENTRY_API void
r_window_handle_maximize (R_WIN32_HWND hwnd)
{
    if (hwnd) ShowWindow (hwnd, SW_MAXIMIZE);
}

R_ENTRY_API void
r_window_handle_restore (R_WIN32_HWND hwnd)
{
    if (hwnd) ShowWindow (hwnd, SW_RESTORE);
}

R_ENTRY_API void
r_window_handle_hide (R_WIN32_HWND hwnd)
{
    if (hwnd) ShowWindow (hwnd, SW_HIDE);
}

R_ENTRY_API void
r_window_handle_show (R_WIN32_HWND hwnd)
{
    if (hwnd) ShowWindow (hwnd, SW_SHOW);
}

R_ENTRY_API void
r_window_handle_get_client_size (R_WIN32_HWND hwnd, int* pWidth, int* pHeight)
{
    if (!hwnd)
    {
        if (pWidth) *pWidth = 0;
        if (pHeight) *pHeight = 0;
        return;
    }

    RECT rc;
    if (GetClientRect (hwnd, &rc))
    {
        if (pWidth) *pWidth = rc.right - rc.left;
        if (pHeight) *pHeight = rc.bottom - rc.top;
    }
    else
    {
        if (pWidth) *pWidth = 0;
        if (pHeight) *pHeight = 0;
    }
}

R_ENTRY_API void
r_window_handle_get_window_size (R_WIN32_HWND hwnd, int* pWidth, int* pHeight)
{
    if (!hwnd)
    {
        if (pWidth) *pWidth = 0;
        if (pHeight) *pHeight = 0;
        return;
    }

    RECT rc;
    if (GetWindowRect (hwnd, &rc))
    {
        if (pWidth) *pWidth = rc.right - rc.left;
        if (pHeight) *pHeight = rc.bottom - rc.top;
    }
    else
    {
        if (pWidth) *pWidth = 0;
        if (pHeight) *pHeight = 0;
    }
}

R_ENTRY_API void
r_window_handle_get_position (R_WIN32_HWND hwnd, int* pX, int* pY)
{
    if (!hwnd)
    {
        if (pX) *pX = 0;
        if (pY) *pY = 0;
        return;
    }

    RECT rc;
    if (GetWindowRect (hwnd, &rc))
    {
        if (pX) *pX = rc.left;
        if (pY) *pY = rc.top;
    }
    else
    {
        if (pX) *pX = 0;
        if (pY) *pY = 0;
    }
}

R_ENTRY_API void
r_window_handle_set_position (R_WIN32_HWND hwnd, int x, int y)
{
    if (hwnd) SetWindowPos (hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

R_ENTRY_API void
r_window_handle_set_size (R_WIN32_HWND hwnd, int width, int height)
{
    if (hwnd) SetWindowPos (hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
}

R_ENTRY_API void
r_window_handle_set_title (R_WIN32_HWND hwnd, const char* pTitle)
{
    if (!hwnd || !pTitle) return;
    SetWindowTextA (hwnd, pTitle);
}

R_ENTRY_API bool
r_window_handle_is_fullscreen (R_WIN32_HWND hwnd)
{
    if (!hwnd) return false;

    RECT        rc;
    MONITORINFO mi = {sizeof (MONITORINFO)};
    if (GetWindowRect (hwnd, &rc) && GetMonitorInfo (MonitorFromWindow (hwnd, MONITOR_DEFAULTTONEAREST), &mi))
    {
        return rc.left == mi.rcMonitor.left && rc.top == mi.rcMonitor.top && rc.right == mi.rcMonitor.right
               && rc.bottom == mi.rcMonitor.bottom;
    }
    return false;
}

R_ENTRY_API bool
r_window_handle_is_minimized (R_WIN32_HWND hwnd)
{
    if (!hwnd) return false;
    return IsIconic (hwnd) != 0;
}

R_ENTRY_API bool
r_window_handle_is_maximized (R_WIN32_HWND hwnd)
{
    if (!hwnd) return false;
    return IsZoomed (hwnd) != 0;
}

R_ENTRY_API bool
r_window_handle_is_visible (R_WIN32_HWND hwnd)
{
    if (!hwnd) return false;
    return IsWindowVisible (hwnd) != 0;
}

#elif defined(__linux__)

#include <wayland-client.h>
#include <xdg-shell.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <xcb/xcb.h>
#include <vulkan/vulkan.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

struct r_wayland_window_state
{
        struct wl_display*    display;
        struct wl_registry*   registry;
        struct wl_compositor* compositor;
        struct xdg_wm_base*   xdgWmBase;
        struct wl_surface*    surface;
        struct xdg_surface*   xdgSurface;
        struct xdg_toplevel*  xdgToplevel;
        int                   width;
        int                   height;
        bool                  configured;
        bool                  compositorBound;
        bool                  xdgWmBaseBound;
        uint16_t              styleFlags; ///< Window decoration flags (bit-packed)
};

static struct r_wayland_window_state* g_waylandState = NULL;

static void
r_registry_global (
    void*               pData,
    struct wl_registry* pRegistry,
    const uint32_t      name,
    const char*         pInterface,
    const uint32_t      version)
{
    struct r_wayland_window_state* pState = (struct r_wayland_window_state*)pData;

    if (strcmp (pInterface, wl_compositor_interface.name) == 0)
    {
        // Use the exact version the compositor advertises to avoid Intel driver bugs
        uint32_t bindVersion = version;
        if (version < 1)
        {
            R_CSTL_LOG_ERROR ("Compositor version %u is too low, minimum required is 1", version);
            return;
        }
        pState->compositor = wl_registry_bind (pRegistry, name, &wl_compositor_interface, bindVersion);
        if (!pState->compositor)
        {
            R_CSTL_LOG_ERROR ("Failed to bind compositor");
            return;
        }
        pState->compositorBound = true;
        R_CSTL_LOG_INFO ("Bound compositor with version %u", bindVersion);
    }
    else if (strcmp (pInterface, xdg_wm_base_interface.name) == 0)
    {
        // Use the exact version the compositor advertises to avoid Intel driver bugs
        uint32_t bindVersion = version;
        if (version < 1)
        {
            R_CSTL_LOG_ERROR ("XDG WM base version %u is too low, minimum required is 1", version);
            return;
        }
        pState->xdgWmBase = wl_registry_bind (pRegistry, name, &xdg_wm_base_interface, bindVersion);
        if (!pState->xdgWmBase)
        {
            R_CSTL_LOG_ERROR ("Failed to bind XDG WM base");
            return;
        }
        pState->xdgWmBaseBound = true;
        R_CSTL_LOG_INFO ("Bound XDG WM base with version %u", bindVersion);
    }
}

static void
r_registry_global_remove (void* pData, struct wl_registry* pRegistry, const uint32_t name)
{
    (void)pData;
    (void)pRegistry;
    (void)name;
}

static const struct wl_registry_listener g_registryListener = {r_registry_global, r_registry_global_remove};

static void
xdg_wm_base_ping (void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial)
{
    (void)data;
    xdg_wm_base_pong (xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener g_xdgWmBaseListener = {xdg_wm_base_ping};

static void
xdg_surface_configure (void* data, struct xdg_surface* xdg_surface, uint32_t serial)
{
    struct r_wayland_window_state* pState = (struct r_wayland_window_state*)data;
    xdg_surface_ack_configure (xdg_surface, serial);
    pState->configured = true;
}

static const struct xdg_surface_listener xdg_surface_listener = {xdg_surface_configure};

static void
xdg_toplevel_configure (
    void*                data,
    struct xdg_toplevel* xdg_toplevel,
    int32_t              width,
    int32_t              height,
    struct wl_array*     states)
{
    (void)xdg_toplevel;
    (void)states;
    struct r_wayland_window_state* pState = (struct r_wayland_window_state*)data;
    if (width > 0) pState->width = width;
    if (height > 0) pState->height = height;
}

static void
xdg_toplevel_close (void* data, struct xdg_toplevel* xdg_toplevel)
{
    (void)data;
    (void)xdg_toplevel;
}

static void
xdg_toplevel_configure_bounds (void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height)
{
    (void)data;
    (void)xdg_toplevel;
    (void)width;
    (void)height;
}

static void
xdg_toplevel_wm_capabilities (void* data, struct xdg_toplevel* xdg_toplevel, struct wl_array* capabilities)
{
    (void)data;
    (void)xdg_toplevel;
    (void)capabilities;
}

static const struct xdg_toplevel_listener g_xdgTopLevelListener
    = {xdg_toplevel_configure,
       xdg_toplevel_close,
       xdg_toplevel_configure_bounds,
       xdg_toplevel_wm_capabilities};

r_wayland_window
r_init_wayland_window (struct r_application_info* pApplicationInfo, const struct r_window_handle_style* pStyle)
{
    if (!pApplicationInfo)
    {
        R_CSTL_LOG_ERROR ("r_init_wayland_window: Invalid application info");
        return NULL;
    }

    // Use default style if none provided
    struct r_window_handle_style defaultStyle = {R_WINDOW_STYLE_DEFAULT};
    if (!pStyle)
    {
        pStyle = &defaultStyle;
    }

    struct r_wayland_window_state* pState
        = (struct r_wayland_window_state*)R_CSTL_HeapAlloc (sizeof (struct r_wayland_window_state));
    if (!pState)
    {
        R_CSTL_LOG_ERROR ("r_init_wayland_window: Failed to allocate state");
        goto r_cleanup_none;
    }
    memset (pState, 0, sizeof (struct r_wayland_window_state));
    pState->width = s_initialWidth;
    pState->height = s_initialHeight;

    // Store style flags for later use
    pState->styleFlags = pStyle->decorationFlags;

    pState->display = wl_display_connect (NULL);
    if (!pState->display)
    {
        R_CSTL_LOG_ERROR ("r_init_wayland_window: Failed to connect to Wayland display");
        goto r_cleanup_state;
    }

    pState->registry = wl_display_get_registry (pState->display);
    if (!pState->registry)
    {
        R_CSTL_LOG_ERROR ("r_init_wayland_window: Failed to get registry");
        goto r_cleanup_display;
    }

    wl_registry_add_listener (pState->registry, &g_registryListener, pState);

    if (wl_display_roundtrip (pState->display) < 0)
    {
        R_CSTL_LOG_ERROR ("r_init_wayland_window: Failed to complete initial Wayland roundtrip");
        goto r_cleanup_registry;
    }

    if (!pState->compositor)
    {
        R_CSTL_LOG_ERROR ("r_init_wayland_window: Failed to bind compositor");
        goto r_cleanup_registry;
    }

    if (!pState->xdgWmBase)
    {
        R_CSTL_LOG_ERROR ("r_init_wayland_window: Failed to bind XDG WM base");
        goto r_cleanup_compositor;
    }

    R_CSTL_LOG_INFO ("Wayland registry roundtrip completed successfully");
    xdg_wm_base_add_listener (pState->xdgWmBase, &g_xdgWmBaseListener, NULL);

    pState->surface = wl_compositor_create_surface (pState->compositor);
    if (!pState->surface)
    {
        R_CSTL_LOG_ERROR ("r_init_wayland_window: Failed to create surface");
        goto r_cleanup_xdg_wm_base;
    }
    R_CSTL_LOG_INFO ("Wayland surface created successfully");

    pState->xdgSurface = xdg_wm_base_get_xdg_surface (pState->xdgWmBase, pState->surface);
    if (!pState->xdgSurface)
    {
        R_CSTL_LOG_ERROR ("r_init_wayland_window: Failed to create XDG surface");
        goto r_cleanup_surface;
    }
    R_CSTL_LOG_INFO ("XDG surface created successfully");

    xdg_surface_add_listener (pState->xdgSurface, &xdg_surface_listener, pState);
    pState->xdgToplevel = xdg_surface_get_toplevel (pState->xdgSurface);
    if (!pState->xdgToplevel)
    {
        R_CSTL_LOG_ERROR ("r_init_wayland_window: Failed to create XDG toplevel");
        goto r_cleanup_xdg_surface;
    }
    R_CSTL_LOG_INFO ("XDG toplevel created successfully");
    xdg_toplevel_add_listener (pState->xdgToplevel, &g_xdgTopLevelListener, pState);

    const char* pAppName = R_CSTL_StringData (pApplicationInfo->pApplicationName);
    if (pAppName)
    {
        xdg_toplevel_set_app_id (pState->xdgToplevel, pAppName);
    }
    xdg_toplevel_set_min_size (pState->xdgToplevel, s_initialWidth, s_initialHeight);
    xdg_surface_set_window_geometry (pState->xdgSurface, 0, 0, s_initialWidth, s_initialHeight);
    wl_surface_commit (pState->surface);
    wl_display_flush (pState->display);

    while (!pState->configured)
    {
        if (wl_display_dispatch (pState->display) == -1)
        {
            R_CSTL_LOG_ERROR ("r_init_wayland_window: Failed to dispatch Wayland events");
            goto r_cleanup_xdg_toplevel;
        }
    }
    wl_surface_commit (pState->surface);
    wl_display_flush (pState->display);

    if (wl_display_roundtrip (pState->display) < 0)
    {
        R_CSTL_LOG_ERROR ("r_init_wayland_window: Failed to complete final Wayland roundtrip");
        goto r_cleanup_xdg_toplevel;
    }
    wl_display_flush (pState->display);

    g_waylandState = pState;

    // Apply window style immediately after window creation
    struct r_window_handle_style currentStyle = {pState->styleFlags};
    union r_window_handle_handle windowHandle;
    windowHandle.waylandWindow = (r_wayland_window)pState;
    r_apply_window_style (R_WINDOW_BACKEND_WAYLAND, &windowHandle, &currentStyle);

    R_CSTL_LOG_INFO ("Wayland with default size %dx%d initialized", pState->width, pState->height);
    return (r_wayland_window)pState;
r_cleanup_xdg_toplevel:
    xdg_toplevel_destroy (pState->xdgToplevel);
r_cleanup_xdg_surface:
    xdg_surface_destroy (pState->xdgSurface);
r_cleanup_surface:
    wl_surface_destroy (pState->surface);
r_cleanup_xdg_wm_base:
    xdg_wm_base_destroy (pState->xdgWmBase);
r_cleanup_compositor:
    wl_compositor_destroy (pState->compositor);
r_cleanup_registry:
    wl_registry_destroy (pState->registry);
r_cleanup_display:
    wl_display_disconnect (pState->display);
r_cleanup_state:
    R_CSTL_HeapFree (pState);
r_cleanup_none:
    return NULL;
}

R_ENTRY_API r_wayland_display
r_get_wayland_display (void)
{
    if (g_waylandState) return (r_wayland_display)g_waylandState->display;
    return NULL;
}

R_ENTRY_API void
r_window_handle_set_fullscreen (r_wayland_window window, const bool fullscreen)
{
    struct r_wayland_window_state* state = (struct r_wayland_window_state*)window;
    if (state && state->xdgToplevel)
    {
        if (fullscreen)
        {
            xdg_toplevel_set_fullscreen (state->xdgToplevel, NULL);
        }
        else
        {
            xdg_toplevel_unset_fullscreen (state->xdgToplevel);
        }
    }
}

R_ENTRY_API void
r_window_handle_set_title (r_wayland_window window, const char* pTitle)
{
    struct r_wayland_window_state* state = (struct r_wayland_window_state*)window;
    if (state && pTitle && window)
        xdg_toplevel_set_title (state->xdgToplevel, pTitle);
}

R_ENTRY_API void
r_window_handle_get_size (r_wayland_window window, int* pWidth, int* pHeight)
{
    struct r_wayland_window_state* state = (struct r_wayland_window_state*)window;
    if (!state)
    {
        if (pWidth) *pWidth = 0;
        if (pHeight) *pHeight = 0;
        return;
    }
    if (pWidth) *pWidth = state->width;
    if (pHeight) *pHeight = state->height;
}

R_ENTRY_API struct wl_surface*
r_wayland_window_get_surface (r_wayland_window window)
{
    struct r_wayland_window_state* state = (struct r_wayland_window_state*)window;
    if (!state) return NULL;
    return state->surface;
}

R_ENTRY_API struct wl_display*
r_wayland_window_get_display (r_wayland_window window)
{
    struct r_wayland_window_state* state = (struct r_wayland_window_state*)window;
    if (!state) return NULL;
    return state->display;
}

R_ENTRY_API void
r_wayland_window_wait_for_settings (r_wayland_window window, int* pWidth, int* pHeight)
{
    struct r_wayland_window_state* state = (struct r_wayland_window_state*)window;
    if (!state) return;

    wl_display_flush (state->display);
    if (wl_display_roundtrip (state->display) < 0)
    {
        R_CSTL_LOG_ERROR ("r_wayland_window_wait_for_settings: Failed to complete Wayland roundtrip");
        if (pWidth) *pWidth = 0;
        if (pHeight) *pHeight = 0;
        return;
    }

    // Additional flush after roundtrip
    wl_display_flush (state->display);

    state->width = s_initialWidth;
    state->height = s_initialHeight;

    if (pWidth) *pWidth = state->width;
    if (pHeight) *pHeight = state->height;

    R_CSTL_LOG_INFO ("r_wayland_window_wait_for_settings: Window size set to %dx%d", state->width, state->height);
}

R_ENTRY_API void
r_destroy_wayland_window (r_wayland_window window)
{
    struct r_wayland_window_state* state = (struct r_wayland_window_state*)window;
    if (!state) return;

    if (state->xdgToplevel) xdg_toplevel_destroy (state->xdgToplevel);
    if (state->xdgSurface) xdg_surface_destroy (state->xdgSurface);
    if (state->xdgWmBase) xdg_wm_base_destroy (state->xdgWmBase);
    if (state->surface) wl_surface_destroy (state->surface);
    if (state->compositor) wl_compositor_destroy (state->compositor);
    if (state->registry) wl_registry_destroy (state->registry);
    if (state->display) wl_display_disconnect (state->display);
    R_CSTL_HeapFree (state);
}

R_ENTRY_API r_x11_window
r_init_x11_window (struct r_application_info* pApplicationInfo, const struct r_window_handle_style* pStyle)
{
    if (!pApplicationInfo)
    {
        R_CSTL_LOG_ERROR ("r_init_x11_window: Invalid application info");
        return 0;
    }

    // Use default style if none provided
    struct r_window_handle_style defaultStyle = {R_WINDOW_STYLE_DEFAULT};
    if (!pStyle)
    {
        pStyle = &defaultStyle;
    }

    // Store style flags for later use
    g_x11State.styleFlags = pStyle->decorationFlags;

    g_x11State.display = XOpenDisplay (NULL);
    if (!g_x11State.display)
    {
        R_CSTL_LOG_ERROR ("r_init_x11_window: Failed to open X display");
        return 0;
    }
    g_x11State.screen = DefaultScreen (g_x11State.display);

    XVisualInfo visualTemplate;
    visualTemplate.screen = g_x11State.screen;
    visualTemplate.depth = 32;
    visualTemplate.class = TrueColor;

    int          visualCount;
    XVisualInfo* pVisualList = XGetVisualInfo (
        g_x11State.display,
        VisualScreenMask | VisualDepthMask | VisualClassMask,
        &visualTemplate,
        &visualCount);

    Visual* pVisual;
    int     depth;
    if (pVisualList && visualCount > 0)
    {
        pVisual = pVisualList[0].visual;
        depth = pVisualList[0].depth;
    }
    else
    {
        pVisual = DefaultVisual (g_x11State.display, g_x11State.screen);
        depth = DefaultDepth (g_x11State.display, g_x11State.screen);
        if (pVisualList) XFree (pVisualList);
        pVisualList = NULL;
    }
    XSetWindowAttributes attrs = {0};
    attrs.colormap = XCreateColormap (
        g_x11State.display,
        RootWindow (g_x11State.display, g_x11State.screen),
        pVisual,
        AllocNone);
    attrs.background_pixel = 0;
    attrs.border_pixel = 0;
    attrs.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask
                       | StructureNotifyMask | FocusChangeMask;
    g_x11State.window = XCreateWindow (
        g_x11State.display,
        RootWindow (g_x11State.display, g_x11State.screen),
        0,
        0,
        s_initialWidth,
        s_initialHeight,
        0,
        depth,
        InputOutput,
        pVisual,
        CWEventMask | CWColormap | CWBorderPixel | CWBackPixel,
        &attrs);

    if (!g_x11State.window)
    {
        R_CSTL_LOG_ERROR ("r_init_x11_window: Failed to create X11 window");
        XCloseDisplay (g_x11State.display);
        g_x11State.display = NULL;
        return 0;
    }
    const char* pAppName = R_CSTL_StringData (pApplicationInfo->pApplicationName);
    if (pAppName)
    {
        XStoreName (g_x11State.display, g_x11State.window, pAppName);
    }
    g_x11State.wmDeleteMessage = XInternAtom (g_x11State.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols (g_x11State.display, g_x11State.window, &g_x11State.wmDeleteMessage, 1);

    XSelectInput (
        g_x11State.display,
        g_x11State.window,
        ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask
            | StructureNotifyMask);
    attrs.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask
                       | StructureNotifyMask | FocusChangeMask;
    XChangeWindowAttributes (g_x11State.display, g_x11State.window, CWEventMask, &attrs);

    XWMHints* wmHints = XAllocWMHints ();
    if (wmHints)
    {
        wmHints->flags = InputHint | StateHint;
        wmHints->input = True;
        wmHints->initial_state = NormalState;
        XSetWMHints (g_x11State.display, g_x11State.window, wmHints);
        XFree (wmHints);
    }
    XMapWindow (g_x11State.display, g_x11State.window);
    XFlush (g_x11State.display);

    XEvent event;
    do
    {
        XNextEvent (g_x11State.display, &event);
    } while (event.type != MapNotify || event.xmap.window != g_x11State.window);

    // Apply window style immediately after window creation
    struct r_window_handle_style currentStyle = {g_x11State.styleFlags};
    union r_window_handle_handle windowHandle;
    windowHandle.x11Window = g_x11State.window;
    r_apply_window_style (R_WINDOW_BACKEND_X11, &windowHandle, &currentStyle);

    R_CSTL_LOG_INFO ("X11 window created and mapped successfully: %dx%d", s_initialWidth, s_initialHeight);
    return g_x11State.window;
}

R_ENTRY_API r_x11_display
r_get_x11_display (void)
{
    return g_x11State.display;
}

R_ENTRY_API void
r_x11_window_set_fullscreen (r_x11_window window, bool fullscreen)
{
    (void)window; // We use the global state
    if (!g_x11State.display) return;

    Atom wmState = XInternAtom (g_x11State.display, "_NET_WM_STATE", False);
    Atom wmFullscreen = XInternAtom (g_x11State.display, "_NET_WM_STATE_FULLSCREEN", False);

    XEvent xev;
    memset (&xev, 0, sizeof (xev));
    xev.type = ClientMessage;
    xev.xclient.window = g_x11State.window;
    xev.xclient.message_type = wmState;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = fullscreen ? 1 : 0;
    xev.xclient.data.l[1] = wmFullscreen;
    xev.xclient.data.l[2] = 0;

    XSendEvent (
        g_x11State.display,
        DefaultRootWindow (g_x11State.display),
        False,
        SubstructureNotifyMask | SubstructureRedirectMask,
        &xev);
    XFlush (g_x11State.display);
}

R_ENTRY_API void
r_x11_window_set_title (r_x11_window window, const char* pTitle)
{
    (void)window; // We use the global state

    if (!g_x11State.display || !pTitle) return;

    XStoreName (g_x11State.display, g_x11State.window, pTitle);
    XFlush (g_x11State.display);
}

R_ENTRY_API void
r_x11_window_get_size (r_x11_window window, int* pWidth, int* pHeight)
{
    (void)window; // We use the global state

    if (!g_x11State.display)
    {
        if (pWidth) *pWidth = 0;
        if (pHeight) *pHeight = 0;
        return;
    }

    XWindowAttributes attributes;
    if (XGetWindowAttributes (g_x11State.display, g_x11State.window, &attributes))
    {
        if (pWidth) *pWidth = attributes.width;
        if (pHeight) *pHeight = attributes.height;
    }
    else
    {
        if (pWidth) *pWidth = 0;
        if (pHeight) *pHeight = 0;
    }
}

R_ENTRY_API Display*
r_x11_window_get_display (r_x11_window window)
{
    (void)window; // We use the global state
    return g_x11State.display;
}

R_ENTRY_API void
r_destroy_x11_window (r_x11_window window)
{
    (void)window; // We use the global state

    if (g_x11State.window)
    {
        XDestroyWindow (g_x11State.display, g_x11State.window);
        g_x11State.window = 0;
    }

    if (g_x11State.display)
    {
        XCloseDisplay (g_x11State.display);
        g_x11State.display = NULL;
    }

    R_CSTL_LOG_INFO ("X11 window destroyed");
}

R_ENTRY_API R_XCBWindow
r_initXCBWindow (struct r_application_info* pApplicationInfo, const struct r_window_handle_style* pStyle)
{
    if (!pApplicationInfo)
    {
        R_CSTL_LOG_ERROR ("r_initXCBWindow: Invalid application info");
        return 0;
    }

    // Use default style if none provided
    struct r_window_handle_style defaultStyle = {R_WINDOW_STYLE_ORANGE};
    if (!pStyle)
    {
        pStyle = &defaultStyle;
    }

    // Store style flags for later use
    g_xcbState.styleFlags = pStyle->decorationFlags;

    // Open connection to X server
    g_xcbState.connection = xcb_connect (NULL, NULL);
    if (!g_xcbState.connection || xcb_connection_has_error (g_xcbState.connection))
    {
        R_CSTL_LOG_ERROR ("r_initXCBWindow: Failed to connect to X server via XCB");
        if (g_xcbState.connection)
        {
            xcb_disconnect (g_xcbState.connection);
            g_xcbState.connection = NULL;
        }
        return 0;
    }

    // Get screen
    const xcb_setup_t*    setup = xcb_get_setup (g_xcbState.connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator (setup);
    g_xcbState.screen = iter.data;

    // Create window
    g_xcbState.window = xcb_generate_id (g_xcbState.connection);

    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t values[2] = {
        g_xcbState.screen->white_pixel,
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE
            | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_STRUCTURE_NOTIFY};

    xcb_create_window (
        g_xcbState.connection,
        XCB_COPY_FROM_PARENT,
        g_xcbState.window,
        g_xcbState.screen->root,
        0,
        0,
        s_initialWidth,
        s_initialHeight,
        1,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        g_xcbState.screen->root_visual,
        mask,
        values);

    // Set window title
    const char* pAppName = R_CSTL_StringData (pApplicationInfo->pApplicationName);
    if (pAppName)
    {
        xcb_change_property (
            g_xcbState.connection,
            XCB_PROP_MODE_REPLACE,
            g_xcbState.window,
            XCB_ATOM_WM_NAME,
            XCB_ATOM_STRING,
            8,
            strlen (pAppName),
            pAppName);
    }

    // Map (show) window
    xcb_map_window (g_xcbState.connection, g_xcbState.window);
    xcb_flush (g_xcbState.connection);

    // Apply window style immediately after window creation
    struct r_window_handle_style currentStyle = {g_xcbState.styleFlags};
    union r_window_handle_handle windowHandle;
    windowHandle.xcbWindow = g_xcbState.window;
    r_apply_window_style (R_WINDOW_BACKEND_XCB, &windowHandle, &currentStyle);

    R_CSTL_LOG_INFO ("XCB window created successfully: %dx%d", s_initialWidth, s_initialHeight);
    return g_xcbState.window;
}

R_ENTRY_API R_XCBConnection
r_getXCBConnection (void)
{
    return g_xcbState.connection;
}

R_ENTRY_API void
R_XCBWindowSetFullscreen (R_XCBWindow window, bool fullscreen)
{
    (void)window; // We use the global state

    if (!g_xcbState.connection) return;

    xcb_intern_atom_cookie_t wm_state_cookie
        = xcb_intern_atom (g_xcbState.connection, 0, strlen ("_NET_WM_STATE"), "_NET_WM_STATE");
    xcb_intern_atom_cookie_t wm_fullscreen_cookie = xcb_intern_atom (
        g_xcbState.connection,
        0,
        strlen ("_NET_WM_STATE_FULLSCREEN"),
        "_NET_WM_STATE_FULLSCREEN");

    xcb_intern_atom_reply_t* wm_state_reply
        = xcb_intern_atom_reply (g_xcbState.connection, wm_state_cookie, NULL);
    xcb_intern_atom_reply_t* wm_fullscreen_reply
        = xcb_intern_atom_reply (g_xcbState.connection, wm_fullscreen_cookie, NULL);

    if (wm_state_reply && wm_fullscreen_reply)
    {
        xcb_client_message_event_t event;
        memset (&event, 0, sizeof (event));
        event.response_type = XCB_CLIENT_MESSAGE;
        event.window = g_xcbState.window;
        event.type = wm_state_reply->atom;
        event.format = 32;
        event.data.data32[0] = fullscreen ? 1 : 0;
        event.data.data32[1] = wm_fullscreen_reply->atom;
        event.data.data32[2] = 0;

        xcb_send_event (
            g_xcbState.connection,
            0,
            g_xcbState.screen->root,
            XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
            (char*)&event);
        xcb_flush (g_xcbState.connection);

        free (wm_state_reply);
        free (wm_fullscreen_reply);
    }
}

R_ENTRY_API void
R_XCBWindowSetTitle (R_XCBWindow window, const char* pTitle)
{
    (void)window; // We use the global state

    if (!g_xcbState.connection || !pTitle) return;

    xcb_change_property (
        g_xcbState.connection,
        XCB_PROP_MODE_REPLACE,
        g_xcbState.window,
        XCB_ATOM_WM_NAME,
        XCB_ATOM_STRING,
        8,
        strlen (pTitle),
        pTitle);
    xcb_flush (g_xcbState.connection);
}

R_ENTRY_API void
R_XCBWindowGetSize (R_XCBWindow window, int* pWidth, int* pHeight)
{
    (void)window; // We use the global state

    if (!g_xcbState.connection)
    {
        if (pWidth) *pWidth = 0;
        if (pHeight) *pHeight = 0;
        return;
    }

    xcb_get_geometry_cookie_t cookie = xcb_get_geometry (g_xcbState.connection, g_xcbState.window);
    xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply (g_xcbState.connection, cookie, NULL);

    if (reply)
    {
        if (pWidth) *pWidth = reply->width;
        if (pHeight) *pHeight = reply->height;
        free (reply);
    }
    else
    {
        if (pWidth) *pWidth = 0;
        if (pHeight) *pHeight = 0;
    }
}

R_ENTRY_API xcb_connection_t*
R_XCBWindowGetConnection (R_XCBWindow window)
{
    (void)window; // We use the global state
    return g_xcbState.connection;
}

R_ENTRY_API void
r_destroyXCBWindow (R_XCBWindow window)
{
    (void)window; // We use the global state

    if (g_xcbState.window)
    {
        xcb_destroy_window (g_xcbState.connection, g_xcbState.window);
        g_xcbState.window = 0;
    }

    if (g_xcbState.connection)
    {
        xcb_disconnect (g_xcbState.connection);
        g_xcbState.connection = NULL;
    }

    R_CSTL_LOG_INFO ("XCB window destroyed");
}

// Common window style functions

/**
 * @brief Maps color theme to RGB values
 * @param colorTheme Color theme to map
 * @param pRed Output pointer for red component
 * @param pGreen Output pointer for green component
 * @param pBlue Output pointer for blue component
 */
static void
r_map_color_theme_toRGB (uint16_t colorTheme, uint16_t* pRed, uint16_t* pGreen, uint16_t* pBlue)
{
    switch (colorTheme)
    {
    case R_WINDOW_COLOR_ORANGE:
        *pRed = 0xFF00;
        *pGreen = 0xA500;
        *pBlue = 0x0000;
        break;
    case R_WINDOW_COLOR_BLUE:
        *pRed = 0x0000;
        *pGreen = 0x0000;
        *pBlue = 0xFFFF;
        break;
    case R_WINDOW_COLOR_GREEN:
        *pRed = 0x0000;
        *pGreen = 0x8000;
        *pBlue = 0x0000;
        break;
    case R_WINDOW_COLOR_RED:
        *pRed = 0xFF00;
        *pGreen = 0x0000;
        *pBlue = 0x0000;
        break;
    case R_WINDOW_COLOR_PURPLE:
        *pRed = 0x8000;
        *pGreen = 0x0000;
        *pBlue = 0x8000;
        break;
    default:
        *pRed = 0;
        *pGreen = 0;
        *pBlue = 0;
        break;
    }
}

/**
 * @brief Applies X11 window manager color hints
 * @param display X11 display
 * @param window X11 window
 * @param red Red color component
 * @param green Green color component
 * @param blue Blue color component
 */
static void
r_apply_x11_color_hints (Display* display, Window window, uint16_t red, uint16_t green, uint16_t blue)
{
    // KDE/Plasma supports _KDE_NET_WM_FRAME_STRUT and related atoms
    Atom kdeColorAtom = XInternAtom (display, "_KDE_NET_WM_FRAME_STRUT", False);
    if (kdeColorAtom != None)
    {
        // Try to set frame color hints for KDE
        unsigned long frameColor[4] = {red, green, blue, 0};
        XChangeProperty (
            display,
            window,
            kdeColorAtom,
            XInternAtom (display, "CARDINAL", False),
            32,
            PropModeReplace,
            (unsigned char*)frameColor,
            4);
    }

    // GNOME/Mutter supports _GTK_THEME_VARIANT and related hints
    Atom gtkThemeAtom = XInternAtom (display, "_GTK_THEME_VARIANT", False);
    if (gtkThemeAtom != None)
    {
        const char* themeVariant = "dark";
        XChangeProperty (
            display,
            window,
            gtkThemeAtom,
            XInternAtom (display, "STRING", False),
            8,
            PropModeReplace,
            (unsigned char*)themeVariant,
            strlen (themeVariant));
    }
    Atom windowRoleAtom = XInternAtom (display, "WM_WINDOW_ROLE", False);
    if (windowRoleAtom != None)
    {
        const char* windowRole = "custom-themed-window";
        XChangeProperty (
            display,
            window,
            windowRoleAtom,
            XInternAtom (display, "STRING", False),
            8,
            PropModeReplace,
            (unsigned char*)windowRole,
            strlen (windowRole));
    }
    Atom xappDecorAtom = XInternAtom (display, "_XAPP_DECORATION_ENABLED", False);
    if (xappDecorAtom != None)
    {
        unsigned long enabled = 1;
        XChangeProperty (
            display,
            window,
            xappDecorAtom,
            XInternAtom (display, "CARDINAL", False),
            32,
            PropModeReplace,
            (unsigned char*)&enabled,
            1);
    }
}

/**
 * @brief Applies XCB window manager color hints
 * @param connection XCB connection
 * @param window XCB window
 * @param red Red color component
 * @param green Green color component
 * @param blue Blue color component
 */
static void
r_applyXCBColorHints (xcb_connection_t* connection, xcb_window_t window, uint16_t red, uint16_t green, uint16_t blue)
{
    // Get standard atoms first
    xcb_intern_atom_cookie_t cardinalCookie
        = xcb_intern_atom (connection, 0, strlen ("CARDINAL"), "CARDINAL");
    xcb_intern_atom_reply_t* cardinalReply
        = xcb_intern_atom_reply (connection, cardinalCookie, NULL);
    xcb_intern_atom_cookie_t stringCookie
        = xcb_intern_atom (connection, 0, strlen ("STRING"), "STRING");
    xcb_intern_atom_reply_t* stringReply
        = xcb_intern_atom_reply (connection, stringCookie, NULL);

    if (!cardinalReply || !stringReply)
    {
        if (cardinalReply) free (cardinalReply);
        if (stringReply) free (stringReply);
        return;
    }

    // KDE/Plasma supports _KDE_NET_WM_FRAME_STRUT and related atoms
    xcb_intern_atom_cookie_t kdeCookie
        = xcb_intern_atom (connection, 0, strlen ("_KDE_NET_WM_FRAME_STRUT"), "_KDE_NET_WM_FRAME_STRUT");
    xcb_intern_atom_reply_t* kdeReply
        = xcb_intern_atom_reply (connection, kdeCookie, NULL);
    if (kdeReply)
    {
        const uint32_t frameColor[4] = {red, green, blue, 0};
        xcb_change_property (
            connection,
            XCB_PROP_MODE_REPLACE,
            window,
            kdeReply->atom,
            cardinalReply->atom,
            32,
            4,
            (unsigned char*)frameColor);
        free (kdeReply);
    }

    // GNOME/Mutter supports _GTK_THEME_VARIANT and related hints
    xcb_intern_atom_cookie_t gtkCookie
        = xcb_intern_atom (connection, 0, strlen ("_GTK_THEME_VARIANT"), "_GTK_THEME_VARIANT");
    xcb_intern_atom_reply_t* gtkReply
        = xcb_intern_atom_reply (connection, gtkCookie, NULL);
    if (gtkReply)
    {
        const char* themeVariant = "dark";
        xcb_change_property (
            connection,
            XCB_PROP_MODE_REPLACE,
            window,
            gtkReply->atom,
            stringReply->atom,
            8,
            strlen (themeVariant),
            (unsigned char*)themeVariant);
        free (gtkReply);
    }

    // Try to set custom window role for theme styling
    xcb_intern_atom_cookie_t roleCookie
        = xcb_intern_atom (connection, 0, strlen ("WM_WINDOW_ROLE"), "WM_WINDOW_ROLE");
    xcb_intern_atom_reply_t* roleReply
        = xcb_intern_atom_reply (connection, roleCookie, NULL);
    if (roleReply)
    {
        const char* windowRole = "custom-themed-window";
        xcb_change_property (
            connection,
            XCB_PROP_MODE_REPLACE,
            window,
            roleReply->atom,
            stringReply->atom,
            8,
            strlen (windowRole),
            (unsigned char*)windowRole);
        free (roleReply);
    }

    // Set XApp decoration hints for modern Linux desktops
    xcb_intern_atom_cookie_t xappCookie
        = xcb_intern_atom (connection, 0, strlen ("_XAPP_DECORATION_ENABLED"), "_XAPP_DECORATION_ENABLED");
    xcb_intern_atom_reply_t* xappReply
        = xcb_intern_atom_reply (connection, xappCookie, NULL);
    if (xappReply)
    {
        uint32_t enabled = 1;
        xcb_change_property (
            connection,
            XCB_PROP_MODE_REPLACE,
            window,
            xappReply->atom,
            cardinalReply->atom,
            32,
            1,
            (unsigned char*)&enabled);
        free (xappReply);
    }

    // Clean up atom replies
    free (cardinalReply);
    free (stringReply);
}

/**
 * @brief Sets X11 window decorations based on style flags
 * @param display X11 display
 * @param window X11 window
 * @param decorationFlags Decoration flags
 * @param isBorderless Whether window should be borderless
 */
static void
r_set_x11_window_decorations (Display* display, Window window, uint8_t decorationFlags, bool isBorderless)
{
    Atom wmHints = XInternAtom (display, "_MOTIF_WM_HINTS", False);
    if (wmHints == None) return;

    struct
    {
            unsigned long flags;
            unsigned long functions;
            unsigned long styles;
            long          inputMode;
            unsigned long status;
    } hints = {0};

    hints.flags = 2; // MWM_HINTS_DECORATIONS

    if (isBorderless)
    {
        hints.styles = 0;
    }
    else
    {
        hints.styles = 0;
        if (r_has_decoration_flag (decorationFlags, R_WINDOW_DECORATION_BORDER))
            hints.styles |= (1 << 1); // MWM_DECOR_BORDER
        if (r_has_decoration_flag (decorationFlags, R_WINDOW_DECORATION_TITLEBAR))
            hints.styles |= (1 << 3); // MWM_DECOR_TITLE
        if (r_has_decoration_flag (decorationFlags, R_WINDOW_DECORATION_MINIMIZE))
            hints.styles |= (1 << 2); // MWM_DECOR_MINIMIZE
        if (r_has_decoration_flag (decorationFlags, R_WINDOW_DECORATION_MAXIMIZE))
            hints.styles |= (1 << 4); // MWM_DECOR_MAXIMIZE
        if (r_has_decoration_flag (decorationFlags, R_WINDOW_DECORATION_CLOSE))
            hints.styles |= (1 << 5); // MWM_DECOR_MENU (close button)
    }

    XChangeProperty (
        display,
        window,
        wmHints,
        wmHints,
        32,
        PropModeReplace,
        (unsigned char*)&hints,
        5);
    XFlush (display);
}

/**
 * @brief Sets XCB window decorations based on style flags
 * @param connection XCB connection
 * @param window XCB window
 * @param decorationFlags Decoration flags
 * @param isBorderless Whether window should be borderless
 */
static void
r_setXCBWindowDecorations (xcb_connection_t* connection, xcb_window_t window, uint8_t decorationFlags, bool isBorderless)
{
    xcb_intern_atom_cookie_t motifCookie
        = xcb_intern_atom (connection, 0, strlen ("_MOTIF_WM_HINTS"), "_MOTIF_WM_HINTS");
    xcb_intern_atom_reply_t* motifReply
        = xcb_intern_atom_reply (connection, motifCookie, NULL);
    if (!motifReply) return;

    struct
    {
            unsigned long flags;
            unsigned long functions;
            unsigned long styles;
            long          inputMode;
            unsigned long status;
    } hints = {0};

    hints.flags = 2; // MWM_HINTS_DECORATIONS

    if (isBorderless)
    {
        hints.styles = 0; // No styles
    }
    else
    {
        hints.styles = 0; // Start with no styles
        if (r_has_decoration_flag (decorationFlags, R_WINDOW_DECORATION_BORDER))
            hints.styles |= (1 << 1); // MWM_DECOR_BORDER
        if (r_has_decoration_flag (decorationFlags, R_WINDOW_DECORATION_TITLEBAR))
            hints.styles |= (1 << 3); // MWM_DECOR_TITLE
        if (r_has_decoration_flag (decorationFlags, R_WINDOW_DECORATION_MINIMIZE))
            hints.styles |= (1 << 2); // MWM_DECOR_MINIMIZE
        if (r_has_decoration_flag (decorationFlags, R_WINDOW_DECORATION_MAXIMIZE))
            hints.styles |= (1 << 4); // MWM_DECOR_MAXIMIZE
        if (r_has_decoration_flag (decorationFlags, R_WINDOW_DECORATION_CLOSE))
            hints.styles |= (1 << 5); // MWM_DECOR_MENU (close button)
    }
    xcb_change_property (
        connection,
        XCB_PROP_MODE_REPLACE,
        window,
        motifReply->atom,
        motifReply->atom,
        32,
        5,
        (unsigned char*)&hints);
    xcb_flush (connection);
    free (motifReply);
}

R_ENTRY_API void
r_apply_window_style (
    enum r_window_handle_backend        backend,
    union r_window_handle_handle*       pWindowHandle,
    const struct r_window_handle_style* pStyle)
{
    if (!pStyle || !pWindowHandle) return;

    uint8_t decorationFlags = r_get_decoration_flags (pStyle->decorationFlags);
    uint16_t colorTheme = r_get_color_theme (pStyle->decorationFlags);
    bool isBorderless = !r_has_decoration_flag (pStyle->decorationFlags, R_WINDOW_DECORATION_BORDER);

    switch (backend)
    {
    case R_WINDOW_BACKEND_WAYLAND:
        // Wayland style is handled by the compositor, but we can set properties
        if (isBorderless)
        {
            // For Wayland, borderless is typically handled by the compositor
            // We might need to use specific protocols for custom styles
            // Some compositors support xdg-decoration protocol for borderless windows
        }
        if (g_waylandState && g_waylandState->xdgToplevel)
        {
            // Check if decoration protocol is available and apply flags
            // This is compositor-specific and may not work on all Wayland compositors
            if (r_has_decoration_flag (pStyle->decorationFlags, R_WINDOW_DECORATION_MINIMIZE))
            {
                // Request minimize capability (if supported by compositor)
            }
            if (r_has_decoration_flag (pStyle->decorationFlags, R_WINDOW_DECORATION_MAXIMIZE))
            {
                // Request maximize capability (if supported by compositor)
            }
            if (r_has_decoration_flag (pStyle->decorationFlags, R_WINDOW_DECORATION_CLOSE))
            {
                // Request close capability (if supported by compositor)
            }
        }
        break;
    case R_WINDOW_BACKEND_X11:
        r_set_x11_window_decorations (g_x11State.display, g_x11State.window, decorationFlags, isBorderless);

        if (colorTheme != R_WINDOW_COLOR_DEFAULT)
        {
            uint16_t red, green, blue;
            r_map_color_theme_toRGB (colorTheme, &red, &green, &blue);

            XColor color;
            Colormap colormap = DefaultColormap (g_x11State.display, g_x11State.screen);
            color.red = red;
            color.green = green;
            color.blue = blue;

            XAllocColor (g_x11State.display, colormap, &color);
            XSetWindowBackground (g_x11State.display, g_x11State.window, color.pixel);

            r_apply_x11_color_hints (g_x11State.display, g_x11State.window, red, green, blue);
            XClearWindow (g_x11State.display, g_x11State.window);
        }
        break;

    case R_WINDOW_BACKEND_XCB:
        r_setXCBWindowDecorations (g_xcbState.connection, g_xcbState.window, decorationFlags, isBorderless);

        // Apply color theme by setting window background color and decoration hints
        if (colorTheme != R_WINDOW_COLOR_DEFAULT)
        {
            uint16_t red, green, blue;
            r_map_color_theme_toRGB (colorTheme, &red, &green, &blue);

            xcb_screen_iterator_t screenIter = xcb_setup_roots_iterator (xcb_get_setup (g_xcbState.connection));
            xcb_colormap_t colormap = screenIter.data->default_colormap;

            xcb_alloc_color_cookie_t colorCookie = xcb_alloc_color (g_xcbState.connection, colormap, red, green, blue);
            xcb_alloc_color_reply_t* colorReply = xcb_alloc_color_reply (g_xcbState.connection, colorCookie, NULL);

            if (colorReply)
            {
                xcb_change_window_attributes (
                    g_xcbState.connection,
                    g_xcbState.window,
                    XCB_CW_BACK_PIXEL,
                    &colorReply->pixel);
                free (colorReply);
            }

            r_applyXCBColorHints (g_xcbState.connection, g_xcbState.window, red, green, blue);

            // Force window refresh to apply background color
            xcb_clear_area (g_xcbState.connection, 1, g_xcbState.window, 0, 0, screenIter.data->width_in_pixels, screenIter.data->height_in_pixels);
        }
        break;

    default:
        break;
    }
}

R_ENTRY_API void
r_set_window_title (
    enum r_window_handle_backend      backend,
    union r_window_handle_handle*     pWindowHandle,
    const struct r_application_info* pApplicationInfo,
    const char*                     pCustomTitle)
{
    if (!pApplicationInfo) return;

    // Determine the title to use
    const char* title = pCustomTitle;
    if (!title)
    {
        title = R_CSTL_StringData (pApplicationInfo->pApplicationName);
    }

    if (!title) return;

    // Check if title is empty using proper string length check
    size_t titleLength = 0;
    if (pCustomTitle)
    {
        titleLength = strlen (pCustomTitle);
    }
    else if (pApplicationInfo->pApplicationName)
    {
        titleLength = R_CSTL_StringLength (pApplicationInfo->pApplicationName);
    }

    if (titleLength == 0) return;

    switch (backend)
    {
    case R_WINDOW_BACKEND_WAYLAND:
        r_window_handle_set_title (pWindowHandle->waylandWindow, title);
        break;

    case R_WINDOW_BACKEND_X11:
        r_x11_window_set_title (pWindowHandle->x11Window, title);
        break;

    case R_WINDOW_BACKEND_XCB:
        R_XCBWindowSetTitle (pWindowHandle->xcbWindow, title);
        break;

    default:
        break;
    }
}

R_ENTRY_API void
r_process_window_events (union r_window_handle_handle* pWindowHandle)
{
    switch (g_currentBackend)
    {
    case R_WINDOW_BACKEND_WAYLAND:
        if (g_waylandState && g_waylandState->display)
        {
            wl_display_dispatch (g_waylandState->display);
        }
        break;
    case R_WINDOW_BACKEND_X11:
        if (g_x11State.display)
        {
            XEvent event;
            while (XPending (g_x11State.display))
            {
                XNextEvent (g_x11State.display, &event);
                if (event.type == ClientMessage)
                {
                    if (event.xclient.data.l[0] == g_x11State.wmDeleteMessage)
                    {
                        R_CSTL_LOG_INFO ("X11 window close requested");
                    }
                }
            }
        }
        break;
    case R_WINDOW_BACKEND_XCB:
        if (g_xcbState.connection)
        {
            xcb_generic_event_t* event;
            while ((event = xcb_poll_for_event (g_xcbState.connection)))
            {
                free (event);
            }
        }
        break;

    default:
        break;
    }
}

#elif defined(__ANDROID__)

#include "rlgame.base/main_platform_handle.h"
#include "rlgame.base/cstl/cstl_log.h"
#include <android/native_window.h>

static ANativeWindow* g_androidWindow = NULL;

R_ENTRY_API bool
r_init_android_window (ANativeWindow* pWindow)
{
#if defined(R_DEVMODE)
    if (!pWindow)
    {
        return false;
    }
#endif
    g_androidWindow = pWindow;
    return true;
}

R_ENTRY_API ANativeWindow*
r_get_android_window (void)
{
    return g_androidWindow;
}

R_ENTRY_API void
r_android_window_get_size (int* pWidth, int* pHeight)
{
    if (g_androidWindow)
    {
        if (pWidth) *pWidth = ANativeWindow_getWidth (g_androidWindow);
        if (pHeight) *pHeight = ANativeWindow_getHeight (g_androidWindow);
    }
    else
    {
        if (pWidth) *pWidth = 0;
        if (pHeight) *pHeight = 0;
    }
}

R_ENTRY_API void
r_destroy_android_window (void)
{
#if defined(R_DEVMODE)
    if (g_androidWindow)
    {
#endif
        ANativeWindow_release (g_androidWindow);
#if defined(R_DEVMODE)
    }
#endif
}

#endif // defined(__ANDROID__)
