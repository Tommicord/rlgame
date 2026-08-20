#include "rlgame.base/main_window.h"
#include "rlgame.base/main.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_log.h"

#if defined(_WIN32)
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

static HWND g_hwnd = NULL;

/**
 * @brief Gets the global window handle
 * @return Current window handle (may be NULL)
 */
HWND
R_GetWindowHandle (void)
{
        return g_hwnd;
}

LRESULT CALLBACK
WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
                return DefWindowProcW (hwnd, uMsg, wParam, lParam);
        }
}

void
R_UnicodeFromString (const char* pInput, wchar_t** ppOut)
{
        int wideLen = MultiByteToWideChar (CP_UTF8, 0, pInput, -1, NULL, 0);
        if (wideLen == 0) return;
        *ppOut = (wchar_t*)R_CSTL_HeapAlloc (wideLen * sizeof (wchar_t));
        if (*ppOut == NULL) return;
        MultiByteToWideChar (CP_UTF8, 0, pInput, -1, *ppOut, wideLen);
}

void
R_WindowCenter (HWND hwnd)
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

int
R_InitWinMain (HINSTANCE hInstance, struct R_ApplicationInfo* pApplicationInfo, int nCmdShow)
{
        const wchar_t CLASS_NAME[] = L"GameWindowClass";
        WNDCLASSW     wc = {0};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;
        if (!RegisterClassW (&wc)) goto r_fail_init;
        if (!pApplicationInfo) goto r_fail_init;
        const char* pAppName = R_CSTL_StringData (pApplicationInfo->pApplicationName);
        if (!pAppName) goto r_fail_init;
        wchar_t* pWideAppName = NULL;
        R_UnicodeFromString (pAppName, &pWideAppName);
        if (!pWideAppName) goto r_fail_init;
        HWND hwnd = CreateWindowExW (
            0,
            CLASS_NAME,
            pWideAppName,
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
        R_CSTL_HeapFree (pWideAppName);
        if (!hwnd) goto r_fail_init;
        ShowWindow (hwnd, nCmdShow);
        return 1;
r_fail_init:
        R_CSTL_LOG_ERROR ("R_InitWinMain: Failed to initialize WinMain");
        return 0;
}

void
R_WindowSetFullscreen (HWND hwnd, bool fullscreen)
{
        if (!hwnd) return;

        static WINDOWPLACEMENT g_wpPrev = {sizeof (WINDOWPLACEMENT)};
        DWORD                  dwStyle = GetWindowLong (hwnd, GWL_STYLE);

        if (fullscreen)
        {
                MONITORINFO mi = {sizeof (MONITORINFO)};
                if (GetWindowPlacement (hwnd, &g_wpPrev)
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
                SetWindowPlacement (hwnd, &g_wpPrev);
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

void
R_WindowSetBorderless (HWND hwnd, bool borderless)
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

void
R_WindowSetResizable (HWND hwnd, bool resizable)
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

void
R_WindowMinimize (HWND hwnd)
{
        if (hwnd) ShowWindow (hwnd, SW_MINIMIZE);
}

void
R_WindowMaximize (HWND hwnd)
{
        if (hwnd) ShowWindow (hwnd, SW_MAXIMIZE);
}

void
R_WindowRestore (HWND hwnd)
{
        if (hwnd) ShowWindow (hwnd, SW_RESTORE);
}

void
R_WindowHide (HWND hwnd)
{
        if (hwnd) ShowWindow (hwnd, SW_HIDE);
}

void
R_WindowShow (HWND hwnd)
{
        if (hwnd) ShowWindow (hwnd, SW_SHOW);
}

void
R_WindowGetClientSize (HWND hwnd, int* pWidth, int* pHeight)
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

void
R_WindowGetWindowSize (HWND hwnd, int* pWidth, int* pHeight)
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

void
R_WindowGetPosition (HWND hwnd, int* pX, int* pY)
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

void
R_WindowSetPosition (HWND hwnd, int x, int y)
{
        if (hwnd) SetWindowPos (hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void
R_WindowSetSize (HWND hwnd, int width, int height)
{
        if (hwnd) SetWindowPos (hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
}

void
R_WindowSetTitle (HWND hwnd, const char* pTitle)
{
        if (!hwnd || !pTitle) return;

        wchar_t* pWideTitle = NULL;
        R_UnicodeFromString (pTitle, &pWideTitle);
        if (pWideTitle)
        {
                SetWindowTextW (hwnd, pWideTitle);
                R_CSTL_HeapFree (pWideTitle);
        }
}

bool
R_WindowIsFullscreen (HWND hwnd)
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

bool
R_WindowIsMinimized (HWND hwnd)
{
        if (!hwnd) return false;
        return IsIconic (hwnd) != 0;
}

bool
R_WindowIsMaximized (HWND hwnd)
{
        if (!hwnd) return false;
        return IsZoomed (hwnd) != 0;
}

bool
R_WindowIsVisible (HWND hwnd)
{
        if (!hwnd) return false;
        return IsWindowVisible (hwnd) != 0;
}

#endif // defined(_WIN32)

#elif defined(__linux__)

#include "rlgame.base/main_window.h"
#include "rlgame.base/main.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_log.h"

#include <wayland-client.h>
#include <xdg-shell.h>

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
                return NULL;
        }

        state->display = wl_display_connect (NULL);
        if (!state->display)
        {
                R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to connect to Wayland display");
                R_CSTL_HeapFree (state);
                return NULL;
        }

        state->registry = wl_display_get_registry (state->display);
        if (!state->registry)
        {
                R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to get registry");
                wl_display_disconnect (state->display);
                R_CSTL_HeapFree (state);
                return NULL;
        }

        wl_registry_add_listener (state->registry, &registry_listener, state);
        wl_display_roundtrip (state->display);

        if (!state->compositor)
        {
                R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to bind compositor");
                wl_registry_destroy (state->registry);
                wl_display_disconnect (state->display);
                R_CSTL_HeapFree (state);
                return NULL;
        }

        if (!state->xdg_wm_base)
        {
                R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to bind XDG WM base");
                wl_compositor_destroy (state->compositor);
                wl_registry_destroy (state->registry);
                wl_display_disconnect (state->display);
                R_CSTL_HeapFree (state);
                return NULL;
        }

        xdg_wm_base_add_listener (state->xdg_wm_base, &xdg_wm_base_listener, NULL);

        state->surface = wl_compositor_create_surface (state->compositor);
        if (!state->surface)
        {
                R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to create surface");
                xdg_wm_base_destroy (state->xdg_wm_base);
                wl_compositor_destroy (state->compositor);
                wl_registry_destroy (state->registry);
                wl_display_disconnect (state->display);
                R_CSTL_HeapFree (state);
                return NULL;
        }

        state->xdg_surface = xdg_wm_base_get_xdg_surface (state->xdg_wm_base, state->surface);
        if (!state->xdg_surface)
        {
                R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to create XDG surface");
                wl_surface_destroy (state->surface);
                xdg_wm_base_destroy (state->xdg_wm_base);
                wl_compositor_destroy (state->compositor);
                wl_registry_destroy (state->registry);
                wl_display_disconnect (state->display);
                R_CSTL_HeapFree (state);
                return NULL;
        }

        xdg_surface_add_listener (state->xdg_surface, &xdg_surface_listener, state);

        state->xdg_toplevel = xdg_surface_get_toplevel (state->xdg_surface);
        if (!state->xdg_toplevel)
        {
                R_CSTL_LOG_ERROR ("R_InitWaylandWindow: Failed to create XDG toplevel");
                xdg_surface_destroy (state->xdg_surface);
                wl_surface_destroy (state->surface);
                xdg_wm_base_destroy (state->xdg_wm_base);
                wl_compositor_destroy (state->compositor);
                wl_registry_destroy (state->registry);
                wl_display_disconnect (state->display);
                R_CSTL_HeapFree (state);
                return NULL;
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
}

R_WaylandDisplay
R_GetWaylandDisplay (void)
{
        if (g_waylandState) return (R_WaylandDisplay)g_waylandState->display;
        return NULL;
}

void
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

void
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

void
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

void
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

bool
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

ANativeWindow*
R_GetAndroidWindow (void)
{
        return g_androidWindow;
}

void
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

void
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
