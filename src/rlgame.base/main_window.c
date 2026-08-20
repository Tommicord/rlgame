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
        R_WindowSetBorderless (hwnd, true);
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
        if (GetWindowRect (hwnd, &rc)
            && GetMonitorInfo (MonitorFromWindow (hwnd, MONITOR_DEFAULTTONEAREST), &mi))
        {
                return rc.left == mi.rcMonitor.left && rc.top == mi.rcMonitor.top
                       && rc.right == mi.rcMonitor.right && rc.bottom == mi.rcMonitor.bottom;
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

#include "rlgame.base/main_window.h"
#include "rlgame.base/main.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_log.h"

#include <wayland-client.h>
#include <xdg-shell.h>
#include <string.h>

struct R_WaylandWindowState
{
                struct wl_display*    display;
                struct wl_registry*   registry;
                struct wl_compositor* compositor;
                struct xdg_wm_base*   xdg_wm_base;
                struct wl_surface*    surface;
                struct xdg_surface*   xdg_surface;
                struct xdg_toplevel*  xdg_toplevel;
                int                   width;
                int                   height;
                bool                  configured;
                bool                  compositor_bound;
                bool                  xdg_wm_base_bound;
};

static struct R_WaylandWindowState* g_waylandState = NULL;

static void
registry_global (
    void*               data,
    struct wl_registry* registry,
    uint32_t            name,
    const char*         interface,
    uint32_t            version)
{
        struct R_WaylandWindowState* state = (struct R_WaylandWindowState*)data;

        if (strcmp (interface, wl_compositor_interface.name) == 0)
        {
                state->compositor = wl_registry_bind (registry, name, &wl_compositor_interface, version);
                state->compositor_bound = true;
        }
        else if (strcmp (interface, xdg_wm_base_interface.name) == 0)
        {
                state->xdg_wm_base = wl_registry_bind (registry, name, &xdg_wm_base_interface, version);
                state->xdg_wm_base_bound = true;
        }
}

static void
registry_global_remove (void* data, struct wl_registry* registry, uint32_t name)
{
        (void)data;
        (void)registry;
        (void)name;
}

static const struct wl_registry_listener registry_listener = {registry_global, registry_global_remove};

static void
xdg_wm_base_ping (void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial)
{
        (void)data;
        xdg_wm_base_pong (xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {xdg_wm_base_ping};

static void
xdg_surface_configure (void* data, struct xdg_surface* xdg_surface, uint32_t serial)
{
        struct R_WaylandWindowState* state = (struct R_WaylandWindowState*)data;
        xdg_surface_ack_configure (xdg_surface, serial);
        state->configured = true;
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
        struct R_WaylandWindowState* state = (struct R_WaylandWindowState*)data;
        if (width > 0) state->width = width;
        if (height > 0) state->height = height;
}

static void
xdg_toplevel_close (void* data, struct xdg_toplevel* xdg_toplevel)
{
        (void)data;
        (void)xdg_toplevel;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener
    = {xdg_toplevel_configure, xdg_toplevel_close};

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

        wl_registry_add_listener (state->registry, &registry_listener, state);
        wl_display_roundtrip (state->display);

        if (!state->compositor)
        {
                R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to bind compositor");
                goto r_cleanup_registry;
        }

        if (!state->xdg_wm_base)
        {
                R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to bind XDG WM base");
                goto r_cleanup_compositor;
        }

        xdg_wm_base_add_listener (state->xdg_wm_base, &xdg_wm_base_listener, NULL);

        state->surface = wl_compositor_create_surface (state->compositor);
        if (!state->surface)
        {
                R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to create surface");
                goto r_cleanup_xdg_wm_base;
        }

        state->xdg_surface = xdg_wm_base_get_xdg_surface (state->xdg_wm_base, state->surface);
        if (!state->xdg_surface)
        {
                R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to create XDG surface");
                goto r_cleanup_surface;
        }

        xdg_surface_add_listener (state->xdg_surface, &xdg_surface_listener, state);

        state->xdg_toplevel = xdg_surface_get_toplevel (state->xdg_surface);
        if (!state->xdg_toplevel)
        {
                R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to create XDG toplevel");
                goto r_cleanup_xdg_surface;
        }

        xdg_toplevel_add_listener (state->xdg_toplevel, &xdg_toplevel_listener, state);

        const char* pAppName = R_CSTL_StringData (pApplicationInfo->pApplicationName);
        if (pAppName)
        {
                xdg_toplevel_set_app_id (state->xdg_toplevel, pAppName);
        }

        state->width = 800;
        state->height = 600;
        state->configured = false;
        state->compositor_bound = false;
        state->xdg_wm_base_bound = false;

        wl_surface_commit (state->surface);

        while (!state->configured && wl_display_dispatch (state->display) != -1)
        {
        }

        g_waylandState = state;
        R_CSTL_LOG_INFO ("R_InitWaylandWindow: Wayland window initialized successfully");
        return (R_WaylandWindow)state;

r_cleanup_xdg_toplevel:
        xdg_surface_destroy (state->xdg_surface);
r_cleanup_xdg_surface:
        wl_surface_destroy (state->surface);
r_cleanup_surface:
        xdg_wm_base_destroy (state->xdg_wm_base);
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
R_WaylandWindowSetFullscreen (R_WaylandWindow window, bool fullscreen)
{
        struct R_WaylandWindowState* state = (struct R_WaylandWindowState*)window;
        if (!state || !state->xdg_toplevel)
        {
                R_CSTL_LOG_ERROR ("R_WaylandWindowSetFullscreen: Invalid window state");
                return;
        }

        if (fullscreen)
        {
                xdg_toplevel_set_fullscreen (state->xdg_toplevel, NULL);
        }
        else
        {
                xdg_toplevel_unset_fullscreen (state->xdg_toplevel);
        }
}

R_ENTRY_API void
R_WaylandWindowSetTitle (R_WaylandWindow window, const char* pTitle)
{
        struct R_WaylandWindowState* state = (struct R_WaylandWindowState*)window;
        if (!state || !state->xdg_toplevel || !pTitle)
        {
                R_CSTL_LOG_ERROR ("R_WaylandWindowSetTitle: Invalid parameters");
                return;
        }

        xdg_toplevel_set_title (state->xdg_toplevel, pTitle);
}

R_ENTRY_API void
R_WaylandWindowGetSize (R_WaylandWindow window, int* pWidth, int* pHeight)
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

R_ENTRY_API void
R_DestroyWaylandWindow (R_WaylandWindow window)
{
        struct R_WaylandWindowState* state = (struct R_WaylandWindowState*)window;
        if (!state) return;

        if (state->xdg_toplevel) xdg_toplevel_destroy (state->xdg_toplevel);
        if (state->xdg_surface) xdg_surface_destroy (state->xdg_surface);
        if (state->xdg_wm_base) xdg_wm_base_destroy (state->xdg_wm_base);
        if (state->surface) wl_surface_destroy (state->surface);
        if (state->compositor) wl_compositor_destroy (state->compositor);
        if (state->registry) wl_registry_destroy (state->registry);
        if (state->display) wl_display_disconnect (state->display);

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
