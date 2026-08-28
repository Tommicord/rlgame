#include "rlgame.base/main_window.h"
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

static uint16_t s_initialWidth = 784;
static uint16_t s_initialHeight = 512;
static enum R_WindowBackend g_currentBackend = R_WINDOW_BACKEND_NONE;

static struct 
{
    Display* display;
    Window window;
    int screen;
    Atom wmDeleteMessage;
} g_x11State = {NULL, 0, 0, 0};

static struct 
{
    xcb_connection_t* connection;
    xcb_window_t window;
    xcb_screen_t* screen;
} g_xcbState = {NULL, 0, NULL};

// Helper function to check if Wayland is available
static bool 
R_IsWaylandAvailable (void)
{
    // Check if WAYLAND_DISPLAY is set
    const char* waylandDisplay = getenv ("WAYLAND_DISPLAY");
    if (waylandDisplay && strlen (waylandDisplay) > 0) {
        return true;
    }
    
    // Try to connect to Wayland display
    struct wl_display* display = wl_display_connect (NULL);
    if (display) {
        wl_display_disconnect (display);
        return true;
    }
    
    return false;
}

// Helper function to check if X11 is available
static bool 
R_IsX11Available (void)
{
    // Try to connect to X11 display
    Display* display = XOpenDisplay (NULL);
    if (display) {
        XCloseDisplay (display);
        return true;
    }
    return false;
}

// Helper function to check if XCB is available
static bool 
R_IsXCBAvailable (void)
{
    // Try to connect to XCB
    xcb_connection_t* connection = xcb_connect (NULL, NULL);
    if (connection && !xcb_connection_has_error (connection)) {
        xcb_disconnect (connection);
        return true;
    }
    if (connection) {
        xcb_disconnect (connection);
    }
    return false;
}

// Helper function to detect GPU capabilities using Vulkan
static bool 
R_DetectVulkanCapabilities (struct R_GPUCapabilities* pCapabilities)
{
    if (!pCapabilities) return false;
    VkInstance instance = VK_NULL_HANDLE;
    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    
    VkResult result = vkCreateInstance (&createInfo, NULL, &instance);
    if (result != VK_SUCCESS) {
        R_CSTL_LOG_ERROR ("Failed to create Vulkan instance for GPU detection: %d", result);
        return false;
    }
    uint32_t deviceCount = 0;
    result = vkEnumeratePhysicalDevices (instance, &deviceCount, NULL);
    if (result != VK_SUCCESS || deviceCount == 0) {
        R_CSTL_LOG_ERROR ("No Vulkan physical devices found");
        vkDestroyInstance (instance, NULL);
        return false;
    }
    VkPhysicalDevice* devices = (VkPhysicalDevice*)R_CSTL_HeapAlloc (sizeof (VkPhysicalDevice) * deviceCount);
    if (!devices) {
        vkDestroyInstance (instance, NULL);
        return false;
    }
    
    result = vkEnumeratePhysicalDevices (instance, &deviceCount, devices);
    if (result != VK_SUCCESS) {
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
    pCapabilities->gpuName = deviceProperties.deviceName;
    
    // Determine if GPU is "modern" based on various criteria
    // Modern GPUs typically have:
    // - Vulkan 1.0+ (we already have Vulkan support)
    // - Dedicated GPU (not integrated)
    // - Recent driver version
    // - Good feature support
    
    bool isDiscrete = (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
    bool isIntegrated = (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);
    
    // Check for modern Intel integrated GPUs (HD Graphics 4000+ or Iris/Iris Pro)
    bool isModernIntel = false;
    if (isIntegrated && strstr (deviceProperties.deviceName, "Intel")) {
        // Check for HD Graphics 4000+ or Iris/Iris Pro
        if (strstr (deviceProperties.deviceName, "HD Graphics") || 
            strstr (deviceProperties.deviceName, "Iris") ||
            strstr (deviceProperties.deviceName, "UHD") ||
            strstr (deviceProperties.deviceName, "Arc")) {
            isModernIntel = true;
        }
    }
    
    // Check for AMD modern GPUs (GCN 1.0+)
    bool isModernAMD = false;
    if (strstr (deviceProperties.deviceName, "AMD") || strstr (deviceProperties.deviceName, "Radeon")) {
        // Most AMD GPUs with Vulkan support are reasonably modern
        isModernAMD = true;
    }
    
    // Check for NVIDIA modern GPUs (Kepler+)
    bool isModernNVIDIA = false;
    if (strstr (deviceProperties.deviceName, "NVIDIA") || strstr (deviceProperties.deviceName, "GeForce")) {
        // Most NVIDIA GPUs with Vulkan support are reasonably modern
        isModernNVIDIA = true;
    }
    
    // Determine if GPU is modern
    pCapabilities->isModernGPU = isDiscrete || isModernIntel || isModernAMD || isModernNVIDIA;
    
    R_CSTL_LOG_INFO ("GPU Detection:");
    R_CSTL_LOG_INFO ("  GPU Name: %s", deviceProperties.deviceName);
    R_CSTL_LOG_INFO ("  GPU Type: %s", 
        isDiscrete ? "Discrete" : (isIntegrated ? "Integrated" : "Other"));
    R_CSTL_LOG_INFO ("  Vulkan Version: %u.%u.%u", 
        VK_VERSION_MAJOR (vulkanVersion),
        VK_VERSION_MINOR (vulkanVersion),
        VK_VERSION_PATCH (vulkanVersion));
    R_CSTL_LOG_INFO ("  Modern GPU: %s", pCapabilities->isModernGPU ? "Yes" : "No");
    
    R_CSTL_HeapFree (devices);
    vkDestroyInstance (instance, NULL);
    
    return true;
}

R_ENTRY_API enum R_WindowBackend 
R_DetectGPUCapabilities (struct R_GPUCapabilities* pCapabilities)
{
    if (!pCapabilities) {
        return R_WINDOW_BACKEND_NONE;
    }
    
    // Initialize capabilities
    memset (pCapabilities, 0, sizeof (struct R_GPUCapabilities));
    
    // Check platform availability
    pCapabilities->hasWaylandSupport = R_IsWaylandAvailable ();
    pCapabilities->hasX11Support = R_IsX11Available ();
    pCapabilities->hasXCBSupport = R_IsXCBAvailable ();
    
    R_CSTL_LOG_INFO ("Platform Availability:");
    R_CSTL_LOG_INFO ("  Wayland: %s", pCapabilities->hasWaylandSupport ? "Available" : "Not available");
    R_CSTL_LOG_INFO ("  X11: %s", pCapabilities->hasX11Support ? "Available" : "Not available");
    R_CSTL_LOG_INFO ("  XCB: %s", pCapabilities->hasXCBSupport ? "Available" : "Not available");
    
    // Detect GPU capabilities
    bool gpuDetectionSuccess = R_DetectVulkanCapabilities (pCapabilities);
    if (!gpuDetectionSuccess) {
        R_CSTL_LOG_WARN ("GPU detection failed, assuming legacy GPU");
        pCapabilities->isModernGPU = false;
    }
    
    // Choose backend based on GPU modernity and platform availability
    enum R_WindowBackend chosenBackend = R_WINDOW_BACKEND_NONE;
    
    if (pCapabilities->isModernGPU && pCapabilities->hasWaylandSupport) {
        // Modern GPU with Wayland support - use Wayland
        chosenBackend = R_WINDOW_BACKEND_WAYLAND;
        R_CSTL_LOG_INFO ("Chosen backend: Wayland (modern GPU)");
    } else if (pCapabilities->hasX11Support) {
        // Legacy GPU or no Wayland - use X11
        chosenBackend = R_WINDOW_BACKEND_X11;
        R_CSTL_LOG_INFO ("Chosen backend: X11 (legacy GPU or no Wayland)");
    } else if (pCapabilities->hasXCBSupport) {
        // Fallback to XCB
        chosenBackend = R_WINDOW_BACKEND_XCB;
        R_CSTL_LOG_INFO ("Chosen backend: XCB (fallback)");
    } else {
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
R_GetWindowHandle (void)
{
    return g_hwnd;
}

LRESULT CALLBACK
R_WindowProc (R_WIN32_HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
R_WindowCenter (R_WIN32_HWND hwnd)
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
R_InitWinMain (R_WIN32_HINSTANCE hInstance, struct R_ApplicationInfo* pApplicationInfo, int nCmdShow)
{
    const char* CLASS_NAME = "GameWindowClass";
    WNDCLASSA   wc = {0};
    wc.lpfnWndProc = R_WindowProc;
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
    R_WindowCenter (hwnd);
    if (!hwnd) goto r_fail_init;
    ShowWindow (hwnd, nCmdShow);
    return 1;
r_fail_init:
    R_CSTL_LOG_ERROR ("R_InitWinMain: Failed to initialize WinMain");
    return 0;
}

R_ENTRY_API void
R_WindowSetFullscreen (R_WIN32_HWND hwnd, bool fullscreen)
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
R_WindowSetBorderless (R_WIN32_HWND hwnd, bool borderless)
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
R_WindowSetResizable (R_WIN32_HWND hwnd, bool resizable)
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
R_WindowMinimize (R_WIN32_HWND hwnd)
{
    if (hwnd) ShowWindow (hwnd, SW_MINIMIZE);
}

R_ENTRY_API void
R_WindowMaximize (R_WIN32_HWND hwnd)
{
    if (hwnd) ShowWindow (hwnd, SW_MAXIMIZE);
}

R_ENTRY_API void
R_WindowRestore (R_WIN32_HWND hwnd)
{
    if (hwnd) ShowWindow (hwnd, SW_RESTORE);
}

R_ENTRY_API void
R_WindowHide (R_WIN32_HWND hwnd)
{
    if (hwnd) ShowWindow (hwnd, SW_HIDE);
}

R_ENTRY_API void
R_WindowShow (R_WIN32_HWND hwnd)
{
    if (hwnd) ShowWindow (hwnd, SW_SHOW);
}

R_ENTRY_API void
R_WindowGetClientSize (R_WIN32_HWND hwnd, int* pWidth, int* pHeight)
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
R_WindowGetWindowSize (R_WIN32_HWND hwnd, int* pWidth, int* pHeight)
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
R_WindowGetPosition (R_WIN32_HWND hwnd, int* pX, int* pY)
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
R_WindowSetPosition (R_WIN32_HWND hwnd, int x, int y)
{
    if (hwnd) SetWindowPos (hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

R_ENTRY_API void
R_WindowSetSize (R_WIN32_HWND hwnd, int width, int height)
{
    if (hwnd) SetWindowPos (hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
}

R_ENTRY_API void
R_WindowSetTitle (R_WIN32_HWND hwnd, const char* pTitle)
{
    if (!hwnd || !pTitle) return;
    SetWindowTextA (hwnd, pTitle);
}

R_ENTRY_API bool
R_WindowIsFullscreen (R_WIN32_HWND hwnd)
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
R_WindowIsMinimized (R_WIN32_HWND hwnd)
{
    if (!hwnd) return false;
    return IsIconic (hwnd) != 0;
}

R_ENTRY_API bool
R_WindowIsMaximized (R_WIN32_HWND hwnd)
{
    if (!hwnd) return false;
    return IsZoomed (hwnd) != 0;
}

R_ENTRY_API bool
R_WindowIsVisible (R_WIN32_HWND hwnd)
{
    if (!hwnd) return false;
    return IsWindowVisible (hwnd) != 0;
}

#elif defined(__linux__)

#include <wayland-client.h>
#include <xdg-shell.h>
#include <X11/Xlib.h>
#include <xcb/xcb.h>
#include <vulkan/vulkan.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

struct R_WaylandWindowState
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
};

static struct R_WaylandWindowState* g_waylandState = NULL;

static void
R_RegistryGlobal (
    void*               pData,
    struct wl_registry* pRegistry,
    const uint32_t      name,
    const char*         pInterface,
    const uint32_t      version)
{
    struct R_WaylandWindowState* pState = (struct R_WaylandWindowState*)pData;

    if (strcmp (pInterface, wl_compositor_interface.name) == 0)
    {
        // Use the exact version the compositor advertises to avoid Intel driver bugs
        uint32_t bindVersion = version;
        if (version < 1) {
            R_CSTL_LOG_ERROR ("Compositor version %u is too low, minimum required is 1", version);
            return;
        }
        pState->compositor = wl_registry_bind (pRegistry, name, &wl_compositor_interface, bindVersion);
        if (!pState->compositor) {
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
        if (version < 1) {
            R_CSTL_LOG_ERROR ("XDG WM base version %u is too low, minimum required is 1", version);
            return;
        }
        pState->xdgWmBase = wl_registry_bind (pRegistry, name, &xdg_wm_base_interface, bindVersion);
        if (!pState->xdgWmBase) {
            R_CSTL_LOG_ERROR ("Failed to bind XDG WM base");
            return;
        }
        pState->xdgWmBaseBound = true;
        R_CSTL_LOG_INFO ("Bound XDG WM base with version %u", bindVersion);
    }
}

static void
R_RegistryGlobalRemove (void* pData, struct wl_registry* pRegistry, const uint32_t name)
{
    (void)pData;
    (void)pRegistry;
    (void)name;
}

static const struct wl_registry_listener g_registryListener = {R_RegistryGlobal, R_RegistryGlobalRemove};

static void
xdg_wm_base_ping (void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial)
{
    (void)data;
    xdg_wm_base_pong (xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener g_xdgWmBaseListener = {
    xdg_wm_base_ping
};

static void
xdg_surface_configure (void* data, struct xdg_surface* xdg_surface, uint32_t serial)
{
    struct R_WaylandWindowState* pState = (struct R_WaylandWindowState*)data;
    xdg_surface_ack_configure (xdg_surface, serial);
    pState->configured = true;
}

static const struct xdg_surface_listener xdg_surface_listener = {
    xdg_surface_configure
};

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
    struct R_WaylandWindowState* pState = (struct R_WaylandWindowState*)data;
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

static const struct xdg_toplevel_listener g_xdgTopLevelListener = {
    xdg_toplevel_configure,
    xdg_toplevel_close,
    xdg_toplevel_configure_bounds,
    xdg_toplevel_wm_capabilities
};

R_WaylandWindow
R_InitWaylandWindow (struct R_ApplicationInfo* pApplicationInfo)
{
    if (!pApplicationInfo)
    {
        R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Invalid application info");
        return NULL;
    }
    struct R_WaylandWindowState* pState
        = (struct R_WaylandWindowState*)R_CSTL_HeapAlloc (sizeof (struct R_WaylandWindowState));
    if (!pState)
    {
        R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to allocate state");
        goto r_cleanup_none;
    }
    memset(pState, 0, sizeof(struct R_WaylandWindowState));
    pState->width = s_initialWidth;
    pState->height = s_initialHeight;

    pState->display = wl_display_connect (NULL);
    if (!pState->display)
    {
        R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to connect to Wayland display");
        goto r_cleanup_state;
    }

    pState->registry = wl_display_get_registry (pState->display);
    if (!pState->registry)
    {
        R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to get registry");
        goto r_cleanup_display;
    }

    wl_registry_add_listener (pState->registry, &g_registryListener, pState);
    
    if (wl_display_roundtrip (pState->display) < 0) {
        R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to complete initial Wayland roundtrip");
        goto r_cleanup_registry;
    }

    if (!pState->compositor)
    {
        R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to bind compositor");
        goto r_cleanup_registry;
    }

    if (!pState->xdgWmBase)
    {
        R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to bind XDG WM base");
        goto r_cleanup_compositor;
    }
    
    R_CSTL_LOG_INFO ("Wayland registry roundtrip completed successfully");
    xdg_wm_base_add_listener (pState->xdgWmBase, &g_xdgWmBaseListener, NULL);
    
    pState->surface = wl_compositor_create_surface (pState->compositor);
    if (!pState->surface)
    {
        R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to create surface");
        goto r_cleanup_xdg_wm_base;
    }
    R_CSTL_LOG_INFO ("Wayland surface created successfully");

    pState->xdgSurface = xdg_wm_base_get_xdg_surface (pState->xdgWmBase, pState->surface);
    if (!pState->xdgSurface)
    {
        R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to create XDG surface");
        goto r_cleanup_surface;
    }
    R_CSTL_LOG_INFO ("XDG surface created successfully");

    xdg_surface_add_listener (pState->xdgSurface, &xdg_surface_listener, pState);
    pState->xdgToplevel = xdg_surface_get_toplevel (pState->xdgSurface);
    if (!pState->xdgToplevel)
    {
        R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to create XDG toplevel");
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
    wl_display_flush(pState->display);

    while (!pState->configured) {
        if (wl_display_dispatch(pState->display) == -1) {
            R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to dispatch Wayland events");
            goto r_cleanup_xdg_toplevel;
        }
    }
    wl_surface_commit (pState->surface);
    wl_display_flush(pState->display);

    if (wl_display_roundtrip(pState->display) < 0) {
        R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to complete final Wayland roundtrip");
        goto r_cleanup_xdg_toplevel;
    }
    wl_display_flush(pState->display);

    g_waylandState = pState;
    R_CSTL_LOG_INFO ("Wayland with default size %dx%d initialized", pState->width, pState->height);
    return (R_WaylandWindow)pState;
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

R_ENTRY_API R_WaylandDisplay
R_GetWaylandDisplay (void)
{
    if (g_waylandState) return (R_WaylandDisplay)g_waylandState->display;
    return NULL;
}

R_ENTRY_API void
R_WindowSetFullscreen (R_WaylandWindow window, const bool fullscreen)
{
    struct R_WaylandWindowState* state = (struct R_WaylandWindowState*)window;
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
R_WindowSetTitle (R_WaylandWindow window, const char* pTitle)
{
    struct R_WaylandWindowState* state = (struct R_WaylandWindowState*)window;
    if (state)
    {
        if (pTitle && window)
            xdg_toplevel_set_title (state->xdgToplevel, pTitle);
    }
}

R_ENTRY_API void
R_WindowGetSize (R_WaylandWindow window, int* pWidth, int* pHeight)
{
    struct R_WaylandWindowState* state = (struct R_WaylandWindowState*)window;
    if (!state)
    {
        if (pWidth) *pWidth = 0;
        if (pHeight) *pHeight = 0;
        return;
    }
    if (pWidth)
        *pWidth = state->width;
    if (pHeight)
        *pHeight = state->height;
}

R_ENTRY_API struct wl_surface*
R_WaylandWindowGetSurface (R_WaylandWindow window)
{
    struct R_WaylandWindowState* state = (struct R_WaylandWindowState*)window;
    if (!state) return NULL;
    return state->surface;
}

R_ENTRY_API struct wl_display*
R_WaylandWindowGetDisplay (R_WaylandWindow window)
{
    struct R_WaylandWindowState* state = (struct R_WaylandWindowState*)window;
    if (!state) return NULL;
    return state->display;
}

R_ENTRY_API void
R_WaylandWindowWaitForConfig (R_WaylandWindow window, int* pWidth, int* pHeight)
{
    struct R_WaylandWindowState* state = (struct R_WaylandWindowState*)window;
    if (!state) return;

    // Flush any pending events
    wl_display_flush(state->display);
    
    // Perform a roundtrip to ensure all events are processed
    if (wl_display_roundtrip (state->display) < 0) {
        R_CSTL_LOG_ERROR ("R_WaylandWindowWaitForConfig: Failed to complete Wayland roundtrip");
        if (pWidth) *pWidth = 0;
        if (pHeight) *pHeight = 0;
        return;
    }
    
    // Additional flush after roundtrip
    wl_display_flush(state->display);

    state->width = s_initialWidth;
    state->height = s_initialHeight;
    
    if (pWidth)
        *pWidth = state->width;
    if (pHeight)
        *pHeight = state->height;
    
    R_CSTL_LOG_INFO ("R_WaylandWindowWaitForConfig: Window size set to %dx%d", state->width, state->height);
}

R_ENTRY_API void
R_DestroyWaylandWindow (R_WaylandWindow window)
{
    struct R_WaylandWindowState* state = (struct R_WaylandWindowState*)window;
    if (!state) return;

    if (state->xdgToplevel)
        xdg_toplevel_destroy (state->xdgToplevel);
    if (state->xdgSurface)
        xdg_surface_destroy (state->xdgSurface);
    if (state->xdgWmBase)
        xdg_wm_base_destroy (state->xdgWmBase);
    if (state->surface)
        wl_surface_destroy (state->surface);
    if (state->compositor)
        wl_compositor_destroy (state->compositor);
    if (state->registry)
        wl_registry_destroy (state->registry);
    if (state->display)
        wl_display_disconnect (state->display);
    R_CSTL_HeapFree (state);
}

// X11 Window Implementation

R_ENTRY_API R_X11Window 
R_InitX11Window (struct R_ApplicationInfo* pApplicationInfo)
{
    if (!pApplicationInfo) {
        R_CSTL_LOG_ERROR ("R_InitX11Window: Invalid application info");
        return 0;
    }

    // Open connection to X server
    g_x11State.display = XOpenDisplay (NULL);
    if (!g_x11State.display) {
        R_CSTL_LOG_ERROR ("R_InitX11Window: Failed to open X display");
        return 0;
    }

    g_x11State.screen = DefaultScreen (g_x11State.display);

    // Create window
    g_x11State.window = XCreateSimpleWindow (
        g_x11State.display,
        RootWindow (g_x11State.display, g_x11State.screen),
        0, 0,
        s_initialWidth, s_initialHeight,
        1,
        BlackPixel (g_x11State.display, g_x11State.screen),
        WhitePixel (g_x11State.display, g_x11State.screen));

    if (!g_x11State.window) {
        R_CSTL_LOG_ERROR ("R_InitX11Window: Failed to create X11 window");
        XCloseDisplay (g_x11State.display);
        g_x11State.display = NULL;
        return 0;
    }

    // Set window title
    const char* pAppName = R_CSTL_StringData (pApplicationInfo->pApplicationName);
    if (pAppName) {
        XStoreName (g_x11State.display, g_x11State.window, pAppName);
    }

    // Set up WM_DELETE_MESSAGE protocol
    g_x11State.wmDeleteMessage = XInternAtom (g_x11State.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols (g_x11State.display, g_x11State.window, &g_x11State.wmDeleteMessage, 1);

    // Select input events
    XSelectInput (g_x11State.display, g_x11State.window, 
        ExposureMask | KeyPressMask | KeyReleaseMask | 
        ButtonPressMask | ButtonReleaseMask | 
        StructureNotifyMask);

    // Map (show) window
    XMapWindow (g_x11State.display, g_x11State.window);

    // Flush to ensure requests are sent
    XFlush (g_x11State.display);

    R_CSTL_LOG_INFO ("X11 window created successfully: %dx%d", s_initialWidth, s_initialHeight);
    return g_x11State.window;
}

R_ENTRY_API R_X11Display 
R_GetX11Display (void)
{
    return g_x11State.display;
}

R_ENTRY_API void 
R_X11WindowSetFullscreen (R_X11Window window, bool fullscreen)
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

    XSendEvent (g_x11State.display, DefaultRootWindow (g_x11State.display), False,
        SubstructureNotifyMask | SubstructureRedirectMask, &xev);
    XFlush (g_x11State.display);
}

R_ENTRY_API void 
R_X11WindowSetTitle (R_X11Window window, const char* pTitle)
{
    (void)window; // We use the global state
    
    if (!g_x11State.display || !pTitle) return;
    
    XStoreName (g_x11State.display, g_x11State.window, pTitle);
    XFlush (g_x11State.display);
}

R_ENTRY_API void 
R_X11WindowGetSize (R_X11Window window, int* pWidth, int* pHeight)
{
    (void)window; // We use the global state
    
    if (!g_x11State.display) {
        if (pWidth) *pWidth = 0;
        if (pHeight) *pHeight = 0;
        return;
    }

    XWindowAttributes attributes;
    if (XGetWindowAttributes (g_x11State.display, g_x11State.window, &attributes)) {
        if (pWidth) *pWidth = attributes.width;
        if (pHeight) *pHeight = attributes.height;
    } else {
        if (pWidth) *pWidth = 0;
        if (pHeight) *pHeight = 0;
    }
}

R_ENTRY_API Display* 
R_X11WindowGetDisplay (R_X11Window window)
{
    (void)window; // We use the global state
    return g_x11State.display;
}

R_ENTRY_API void 
R_DestroyX11Window (R_X11Window window)
{
    (void)window; // We use the global state
    
    if (g_x11State.window) {
        XDestroyWindow (g_x11State.display, g_x11State.window);
        g_x11State.window = 0;
    }
    
    if (g_x11State.display) {
        XCloseDisplay (g_x11State.display);
        g_x11State.display = NULL;
    }
    
    R_CSTL_LOG_INFO ("X11 window destroyed");
}

// XCB Window Implementation

R_ENTRY_API R_XCBWindow 
R_InitXCBWindow (struct R_ApplicationInfo* pApplicationInfo)
{
    if (!pApplicationInfo) {
        R_CSTL_LOG_ERROR ("R_InitXCBWindow: Invalid application info");
        return 0;
    }

    // Open connection to X server
    g_xcbState.connection = xcb_connect (NULL, NULL);
    if (!g_xcbState.connection || xcb_connection_has_error (g_xcbState.connection)) {
        R_CSTL_LOG_ERROR ("R_InitXCBWindow: Failed to connect to X server via XCB");
        if (g_xcbState.connection) {
            xcb_disconnect (g_xcbState.connection);
            g_xcbState.connection = NULL;
        }
        return 0;
    }

    // Get screen
    const xcb_setup_t* setup = xcb_get_setup (g_xcbState.connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator (setup);
    g_xcbState.screen = iter.data;

    // Create window
    g_xcbState.window = xcb_generate_id (g_xcbState.connection);
    
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t values[2] = {
        g_xcbState.screen->white_pixel,
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | 
        XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_BUTTON_PRESS | 
        XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_STRUCTURE_NOTIFY
    };

    xcb_create_window (
        g_xcbState.connection,
        XCB_COPY_FROM_PARENT,
        g_xcbState.window,
        g_xcbState.screen->root,
        0, 0,
        s_initialWidth, s_initialHeight,
        1,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        g_xcbState.screen->root_visual,
        mask, values);

    // Set window title
    const char* pAppName = R_CSTL_StringData (pApplicationInfo->pApplicationName);
    if (pAppName) {
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

    R_CSTL_LOG_INFO ("XCB window created successfully: %dx%d", s_initialWidth, s_initialHeight);
    return g_xcbState.window;
}

R_ENTRY_API R_XCBConnection 
R_GetXCBConnection (void)
{
    return g_xcbState.connection;
}

R_ENTRY_API void 
R_XCBWindowSetFullscreen (R_XCBWindow window, bool fullscreen)
{
    (void)window; // We use the global state
    
    if (!g_xcbState.connection) return;

    xcb_intern_atom_cookie_t wm_state_cookie = xcb_intern_atom (
        g_xcbState.connection, 0, strlen ("_NET_WM_STATE"), "_NET_WM_STATE");
    xcb_intern_atom_cookie_t wm_fullscreen_cookie = xcb_intern_atom (
        g_xcbState.connection, 0, strlen ("_NET_WM_STATE_FULLSCREEN"), "_NET_WM_STATE_FULLSCREEN");

    xcb_intern_atom_reply_t* wm_state_reply = xcb_intern_atom_reply (
        g_xcbState.connection, wm_state_cookie, NULL);
    xcb_intern_atom_reply_t* wm_fullscreen_reply = xcb_intern_atom_reply (
        g_xcbState.connection, wm_fullscreen_cookie, NULL);

    if (wm_state_reply && wm_fullscreen_reply) {
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
    
    if (!g_xcbState.connection) {
        if (pWidth) *pWidth = 0;
        if (pHeight) *pHeight = 0;
        return;
    }

    xcb_get_geometry_cookie_t cookie = xcb_get_geometry (g_xcbState.connection, g_xcbState.window);
    xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply (g_xcbState.connection, cookie, NULL);

    if (reply) {
        if (pWidth) *pWidth = reply->width;
        if (pHeight) *pHeight = reply->height;
        free (reply);
    } else {
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
R_DestroyXCBWindow (R_XCBWindow window)
{
    (void)window; // We use the global state
    
    if (g_xcbState.window) {
        xcb_destroy_window (g_xcbState.connection, g_xcbState.window);
        g_xcbState.window = 0;
    }
    
    if (g_xcbState.connection) {
        xcb_disconnect (g_xcbState.connection);
        g_xcbState.connection = NULL;
    }
    
    R_CSTL_LOG_INFO ("XCB window destroyed");
}

#elif defined(__ANDROID__)

#include "rlgame.base/main_window.h"
#include "rlgame.base/cstl/cstl_log.h"
#include <android/native_window.h>

static ANativeWindow* g_androidWindow = NULL;

R_ENTRY_API bool
R_InitAndroidWindow (ANativeWindow* pWindow)
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
R_GetAndroidWindow (void)
{
    return g_androidWindow;
}

R_ENTRY_API void
R_AndroidWindowGetSize (int* pWidth, int* pHeight)
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
R_DestroyAndroidWindow (void)
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
