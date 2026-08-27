#include "rlgame.base/main_window.h"
#include "rlgame.base/main.h"
#include "rlgame.base/main_platform.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_log.h"

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
        800,
        600,
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
#include <string.h>

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
        pState->compositor = wl_registry_bind (pRegistry, name, &wl_compositor_interface, version);
        pState->compositorBound = true;
    }
    else if (strcmp (pInterface, xdg_wm_base_interface.name) == 0)
    {
        pState->xdgWmBase = wl_registry_bind (pRegistry, name, &xdg_wm_base_interface, version);
        pState->xdgWmBaseBound = true;
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
R_BasePing (void* data, struct xdg_wm_base* pXdgWmBase, const uint32_t serial)
{
    (void)data;
    xdg_wm_base_pong (pXdgWmBase, serial);
}

static const struct xdg_wm_base_listener g_xdgWmBaseListener = {R_BasePing};

static void
R_SurfaceConfigure (void* data, struct xdg_surface* pXdgSurface, uint32_t serial)
{
    struct R_WaylandWindowState* pState = (struct R_WaylandWindowState*)data;
    xdg_surface_ack_configure (pXdgSurface, serial);
    pState->configured = true;
}

static const struct xdg_surface_listener xdg_surface_listener = {R_SurfaceConfigure};

static void
xdg_toplevel_configure (
    void*                pData,
    struct xdg_toplevel* pXdgTopLevel,
    int32_t              width,
    int32_t              height,
    struct wl_array*     pStates)
{
    (void)pXdgTopLevel;
    (void)pStates;
    struct R_WaylandWindowState* pState = (struct R_WaylandWindowState*)pData;
    if (width > 0) pState->width = width;
    if (height > 0) pState->height = height;
}

static void
R_TopLevelClose (void* data, struct xdg_toplevel* pXdgTopLevel)
{
    (void)data;
    (void)pXdgTopLevel;
}

static const struct xdg_toplevel_listener g_xdgTopLevelListener
    = {xdg_toplevel_configure, R_TopLevelClose};

R_WaylandWindow
R_InitWaylandWindow (struct R_ApplicationInfo* pApplicationInfo)
{
    if (!pApplicationInfo)
    {
        R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Invalid application info");
        return NULL;
    }
    struct R_WaylandWindowState* state
        = (struct R_WaylandWindowState*)R_CSTL_HeapAlloc (sizeof (struct R_WaylandWindowState));
    if (!state)
    {
        R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to allocate state");
        goto r_cleanup_none;
    }
    state->display = wl_display_connect (NULL);
    if (!state->display)
    {
        R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to connect to Wayland display");
        goto r_cleanup_state;
    }

    state->registry = wl_display_get_registry (state->display);
    if (!state->registry)
    {
        R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to get registry");
        goto r_cleanup_displayr;
    }

    wl_registry_add_listener (state->registry, &g_registryListener, state);
    wl_display_roundtrip (state->display);

    if (!state->compositor)
    {
        R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to bind compositor");
        goto r_cleanup_registry;
    }

    if (!state->xdgWmBase)
    {
        R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to bind XDG WM base");
        goto r_cleanup_compositor;
    }
    xdg_wm_base_add_listener (state->xdgWmBase, &g_xdgWmBaseListener, NULL);
    state->surface = wl_compositor_create_surface (state->compositor);
    if (!state->surface)
    {
        R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to create surface");
        goto r_cleanup_xdg_wm_base;
    }

    state->xdgSurface = xdg_wm_base_get_xdg_surface (state->xdgWmBase, state->surface);
    if (!state->xdgSurface)
    {
        R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to create XDG surface");
        goto r_cleanup_surface;
    }

    xdg_surface_add_listener (state->xdgSurface, &xdg_surface_listener, state);

    state->xdgToplevel = xdg_surface_get_toplevel (state->xdgSurface);
    if (!state->xdgToplevel)
    {
        R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to create XDG toplevel");
        goto r_cleanup_xdg_surface;
    }

    xdg_toplevel_add_listener (state->xdgToplevel, &g_xdgTopLevelListener, state);

    const char* pAppName = R_CSTL_StringData (pApplicationInfo->pApplicationName);
    if (pAppName)
    {
        xdg_toplevel_set_app_id (state->xdgToplevel, pAppName);
    }

    state->width = 800;
    state->height = 600;
    state->configured = false;
    state->compositorBound = false;
    state->xdgWmBaseBound = false;

    wl_surface_commit (state->surface);

    while (!state->configured && wl_display_dispatch (state->display) != -1);
    g_waylandState = state;

    return (R_WaylandWindow)state;
r_cleanup_xdg_toplevel:
    xdg_surface_destroy (state->xdgSurface);
r_cleanup_xdg_surface:
    wl_surface_destroy (state->surface);
r_cleanup_surface:
    xdg_wm_base_destroy (state->xdgWmBase);
r_cleanup_xdg_wm_base:
    wl_compositor_destroy (state->compositor);
r_cleanup_compositor:
    wl_registry_destroy (state->registry);
r_cleanup_registry:
    wl_display_disconnect (state->display);
r_cleanup_displayr:
    R_CSTL_HeapFree (state);
r_cleanup_state:
    return NULL;
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
R_WaylandWindowSetFullscreen (R_WaylandWindow window, const bool fullscreen)
{
    struct R_WaylandWindowState* state = (struct R_WaylandWindowState*)window;
    if (!state || !state->xdgToplevel)
    {
        R_CSTL_LOG_ERROR ("R_WaylandWindowSetFullscreen: Invalid window state");
        return;
    }

    if (fullscreen)
    {
        xdg_toplevel_set_fullscreen (state->xdgToplevel, NULL);
    }
    else
    {
        xdg_toplevel_unset_fullscreen (state->xdgToplevel);
    }
}

R_ENTRY_API void
R_WaylandWindowSetTitle (R_WaylandWindow window, const char* pTitle)
{
    struct R_WaylandWindowState* state = (struct R_WaylandWindowState*)window;
    if (!state || !state->xdgToplevel || !pTitle)
    {
        R_CSTL_LOG_ERROR ("R_WaylandWindowSetTitle: Invalid parameters");
        return;
    }

    xdg_toplevel_set_title (state->xdgToplevel, pTitle);
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

    if (pWidth) *pWidth = state->width;
    if (pHeight) *pHeight = state->height;
}

R_ENTRY_API struct wl_surface*
R_WaylandWindowGetSurface (R_WaylandWindow window)
{
    struct R_WaylandWindowState* state = (struct R_WaylandWindowState*)window;
    if (!state) return NULL;
    return state->surface;
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
    g_waylandState = NULL;
    R_CSTL_LOG_INFO ("R_DestroyWaylandWindow: Wayland window destroyed");
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
