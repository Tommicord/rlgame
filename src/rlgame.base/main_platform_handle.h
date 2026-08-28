#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "rlgame.base/main_platform.h"

struct R_ApplicationInfo;

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef HWND      R_WIN32_HWND;
typedef HINSTANCE R_WIN32_HINSTANCE;

/**
 * @brief Window procedure callback for handling Windows messages
 * @param hwnd Window handle
 * @param uMsg Message identifier
 * @param wParam Additional message information
 * @param lParam Additional message information
 * @return Result of message processing
 */
LRESULT CALLBACK WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/**
 * @brief Centers the window on its current monitor
 * @param hwnd Window handle to center
 */
R_ENTRY_API void R_WindowHandleCenter (HWND hwnd);

/**
 * @brief Gets the global window handle
 * @return Current window handle (may be NULL)
 */
R_ENTRY_API HWND R_GetWindowHandle (void);

/**
 * @brief Initializes the Win32 main window
 * @param hInstance Application instance handle
 * @param pApplicationInfo Application information structure
 * @param nCmdShow Show window command (SW_SHOW, SW_HIDE, etc.)
 * @return 1 on success, 0 on failure
 */
R_ENTRY_API int
R_InitWinMain (R_WIN32_HINSTANCE hInstance, struct R_ApplicationInfo* pApplicationInfo, int nCmdShow);

/**
 * @brief Sets the window to fullscreen mode
 * @param hwnd Window handle
 * @param fullscreen true to enable fullscreen, false to disable
 */
R_ENTRY_API void R_WindowHandleSetFullscreen (R_WIN32_HWND hwnd, bool fullscreen);

/**
 * @brief Sets the window to borderless mode
 * @param hwnd Window handle
 * @param borderless true to enable borderless, false to disable
 */
R_ENTRY_API void R_WindowHandleSetBorderless (R_WIN32_HWND hwnd, bool borderless);

/**
 * @brief Sets the window to resizable or fixed size
 * @param hwnd Window handle
 * @param resizable true to enable resizing, false to disable
 */
R_ENTRY_API void R_WindowHandleSetResizable (R_WIN32_HWND hwnd, bool resizable);

/**
 * @brief Minimizes the window
 * @param hwnd Window handle
 */
R_ENTRY_API void R_WindowHandleMinimize (R_WIN32_HWND hwnd);

/**
 * @brief Maximizes the window
 * @param hwnd Window handle
 */
R_ENTRY_API void R_WindowHandleMaximize (R_WIN32_HWND hwnd);

/**
 * @brief Restores the window from minimized or maximized state
 * @param hwnd Window handle
 */
R_ENTRY_API void R_WindowHandleRestore (R_WIN32_HWND hwnd);

/**
 * @brief Hides the window
 * @param hwnd Window handle
 */
R_ENTRY_API void R_WindowHandleHide (R_WIN32_HWND hwnd);

/**
 * @brief Shows the window
 * @param hwnd Window handle
 */
R_ENTRY_API void R_WindowHandleShow (R_WIN32_HWND hwnd);

/**
 * @brief Gets the window client area dimensions
 * @param hwnd Window handle
 * @param pWidth Output pointer for width
 * @param pHeight Output pointer for height
 */
R_ENTRY_API void R_WindowHandleGetClientSize (R_WIN32_HWND hwnd, int* pWidth, int* pHeight);

/**
 * @brief Gets the window dimensions (including borders)
 * @param hwnd Window handle
 * @param pWidth Output pointer for width
 * @param pHeight Output pointer for height
 */
R_ENTRY_API void R_WindowHandleGetWindowSize (R_WIN32_HWND hwnd, int* pWidth, int* pHeight);

/**
 * @brief Gets the window position
 * @param hwnd Window handle
 * @param pX Output pointer for X position
 * @param pY Output pointer for Y position
 */
R_ENTRY_API void R_WindowHandleGetPosition (R_WIN32_HWND hwnd, int* pX, int* pY);

/**
 * @brief Sets the window position
 * @param hwnd Window handle
 * @param x X position
 * @param y Y position
 */
R_ENTRY_API void R_WindowHandleSetPosition (R_WIN32_HWND hwnd, int x, int y);

/**
 * @brief Sets the window size
 * @param hwnd Window handle
 * @param width Window width
 * @param height Window height
 */
R_ENTRY_API void R_WindowHandleSetSize (R_WIN32_HWND hwnd, int width, int height);

/**
 * @brief Sets the window title
 * @param hwnd Window handle
 * @param pTitle Window title string (UTF-8)
 */
R_ENTRY_API void R_WindowHandleSetTitle (R_WIN32_HWND hwnd, const char* pTitle);

/**
 * @brief Checks if the window is currently fullscreen
 * @param hwnd Window handle
 * @return true if fullscreen, false otherwise
 */
R_ENTRY_API bool R_WindowHandleIsFullscreen (R_WIN32_HWND hwnd);

/**
 * @brief Checks if the window is currently minimized
 * @param hwnd Window handle
 * @return true if minimized, false otherwise
 */
R_ENTRY_API bool R_WindowHandleIsMinimized (R_WIN32_HWND hwnd);

/**
 * @brief Checks if the window is currently maximized
 * @param hwnd Window handle
 * @return true if maximized, false otherwise
 */
R_ENTRY_API bool R_WindowHandleIsMaximized (R_WIN32_HWND hwnd);

/**
 * @brief Checks if the window is currently visible
 * @param hwnd Window handle
 * @return true if visible, false otherwise
 */
R_ENTRY_API bool R_WindowHandleIsVisible (R_WIN32_HWND hwnd);

#elif defined(__linux__)

#include <wayland-client.h>
#include <X11/Xlib.h>
#include <xcb/xcb.h>

/**
 * @brief Window decoration style flags (bit-packed)
 *
 * These flags use bit packing to efficiently store window decoration options
 * in a single uint8_t instead of wasting space with multiple booleans or integers.
 */
enum R_WindowDecorationFlags
{
    R_WINDOW_DECORATION_NONE = 0x00,          ///< No decorations
    R_WINDOW_DECORATION_BORDER = 0x01,         ///< Show window border
    R_WINDOW_DECORATION_TITLEBAR = 0x02,       ///< Show title bar
    R_WINDOW_DECORATION_MINIMIZE = 0x04,       ///< Show minimize button
    R_WINDOW_DECORATION_MAXIMIZE = 0x08,       ///< Show maximize button
    R_WINDOW_DECORATION_CLOSE = 0x10,          ///< Show close button
    R_WINDOW_DECORATION_RESIZE = 0x20,         ///< Enable window resizing
    R_WINDOW_DECORATION_MENU = 0x40,          ///< Show window menu
    R_WINDOW_DECORATION_ALL = 0x7F            ///< All decorations enabled
};

/**
 * @brief Window decoration color theme (bit-packed)
 *
 * Color themes for window decorations (menu bar, borders, etc.)
 * Stored in upper bits of decoration flags (bits 8-10)
 */
enum R_WindowDecorationColor
{
    R_WINDOW_COLOR_DEFAULT = 0x00,            ///< Default system color (bits 8-10: 000)
    R_WINDOW_COLOR_ORANGE = 0x0100,           ///< Orange theme (bits 8-10: 001)
    R_WINDOW_COLOR_BLUE = 0x0200,             ///< Blue theme (bits 8-10: 010)
    R_WINDOW_COLOR_GREEN = 0x0300,            ///< Green theme (bits 8-10: 011)
    R_WINDOW_COLOR_RED = 0x0400,              ///< Red theme (bits 8-10: 100)
    R_WINDOW_COLOR_PURPLE = 0x0500,           ///< Purple theme (bits 8-10: 101)
    R_WINDOW_COLOR_CUSTOM = 0x0600            ///< Custom color (bits 8-10: 110)
};

/**
 * @brief Window backend type enumeration
 */
enum R_WindowHandleBackend
{
    R_WINDOW_BACKEND_NONE = 0,
    R_WINDOW_BACKEND_WAYLAND = 1,
    R_WINDOW_BACKEND_X11 = 2,
    R_WINDOW_BACKEND_XCB = 3
};

/**
 * @brief Window handle style structure with bit-packed flags
 *
 * This structure uses bit packing to efficiently store window decoration options
 * without wasting space with multiple booleans or integers.
 */
struct R_WindowHandleStyle
{
    uint16_t decorationFlags; ///< Combined decoration flags and color theme (bit-packed)
};

/**
 * @brief Predefined window decoration styles (bit-packed constants)
 *
 * These constants provide commonly used window decoration configurations
 * using the bit-packed flag system for space efficiency.
 */
#define R_WINDOW_STYLE_DEFAULT ((R_WINDOW_DECORATION_ALL) | (R_WINDOW_COLOR_DEFAULT))
#define R_WINDOW_STYLE_ORANGE ((R_WINDOW_DECORATION_ALL) | (R_WINDOW_COLOR_ORANGE))
#define R_WINDOW_STYLE_BLUE ((R_WINDOW_DECORATION_ALL) | (R_WINDOW_COLOR_BLUE))
#define R_WINDOW_STYLE_GREEN ((R_WINDOW_DECORATION_ALL) | (R_WINDOW_COLOR_GREEN))
#define R_WINDOW_STYLE_RED ((R_WINDOW_DECORATION_ALL) | (R_WINDOW_COLOR_RED))
#define R_WINDOW_STYLE_PURPLE ((R_WINDOW_DECORATION_ALL) | (R_WINDOW_COLOR_PURPLE))
#define R_WINDOW_STYLE_BORDERLESS ((R_WINDOW_DECORATION_NONE) | (R_WINDOW_COLOR_DEFAULT))
#define R_WINDOW_STYLE_MINIMAL ((R_WINDOW_DECORATION_BORDER | R_WINDOW_DECORATION_TITLEBAR) | (R_WINDOW_COLOR_DEFAULT))
#define R_WINDOW_STYLE_ORANGE_BORDERLESS ((R_WINDOW_DECORATION_NONE) | (R_WINDOW_COLOR_ORANGE))

/**
 * @brief Helper function to create a window style from decoration flags and color
 * @param decorationFlags Decoration flags (bitwise OR of R_WindowDecorationFlags)
 * @param colorTheme Color theme (R_WindowDecorationColor)
 * @return Combined decoration flags value
 */
static inline uint16_t R_CreateWindowStyle (uint8_t decorationFlags, uint16_t colorTheme)
{
    return (uint16_t)decorationFlags | colorTheme;
}

/**
 * @brief Helper function to extract decoration flags from style
 * @param styleFlags Combined style flags
 * @return Decoration flags (lower 8 bits)
 */
static inline uint8_t R_GetDecorationFlags (uint16_t styleFlags)
{
    return (uint8_t)(styleFlags & 0xFF);
}

/**
 * @brief Helper function to extract color theme from style
 * @param styleFlags Combined style flags
 * @return Color theme (bits 8-10)
 */
static inline uint16_t R_GetColorTheme (uint16_t styleFlags)
{
    return styleFlags & 0x0700;
}

/**
 * @brief Helper function to check if a specific decoration flag is set
 * @param styleFlags Combined style flags
 * @param flag Flag to check (R_WindowDecorationFlags)
 * @return true if flag is set, false otherwise
 */
static inline bool R_HasDecorationFlag (uint16_t styleFlags, uint8_t flag)
{
    return (styleFlags & flag) != 0;
}

/**
 * @brief Helper function to set a decoration flag
 * @param pStyle Pointer to style structure
 * @param flag Flag to set (R_WindowDecorationFlags)
 */
static inline void R_SetDecorationFlag (struct R_WindowHandleStyle* pStyle, uint8_t flag)
{
    if (pStyle)
    {
        pStyle->decorationFlags |= flag;
    }
}

/**
 * @brief Helper function to clear a decoration flag
 * @param pStyle Pointer to style structure
 * @param flag Flag to clear (R_WindowDecorationFlags)
 */
static inline void R_ClearDecorationFlag (struct R_WindowHandleStyle* pStyle, uint8_t flag)
{
    if (pStyle)
    {
        pStyle->decorationFlags &= ~flag;
    }
}

/**
 * @brief Helper function to set the color theme
 * @param pStyle Pointer to style structure
 * @param colorTheme Color theme to set (R_WindowDecorationColor)
 */
static inline void R_SetColorTheme (struct R_WindowHandleStyle* pStyle, uint16_t colorTheme)
{
    if (pStyle)
    {
        pStyle->decorationFlags = (pStyle->decorationFlags & 0xFF) | colorTheme;
    }
}

/**
 * @brief GPU capability information
 */
struct R_Capabilities
{
        bool        hasWaylandSupport;
        bool        hasX11Support;
        bool        hasXCBSupport;
        bool        looksModernGpu;
        uint32_t    vulkanVersion;
        const char* pVendor;
        const char* pDriverVersion;
};

/**
 * @brief Wayland window handle type
 */
typedef void* R_WaylandWindow;

/**
 * @brief Wayland display handle type
 */
typedef void* R_WaylandDisplay;

/**
 * @brief X11 window handle type
 */
typedef Window R_X11Window;

/**
 * @brief X11 display handle type
 */
typedef Display* R_X11Display;

/**
 * @brief XCB window handle type
 */
typedef xcb_window_t R_XCBWindow;

/**
 * @brief XCB connection handle type
 */
typedef xcb_connection_t* R_XCBConnection;

/**
 * @brief Generic window handle union
 */
union R_WindowHandleHandle
{
        R_WaylandWindow waylandWindow;
        R_X11Window     x11Window;
        R_XCBWindow     xcbWindow;
};

/**
 * @brief Generic display handle union
 */
union R_DisplayHandle
{
        R_WaylandDisplay waylandDisplay;
        R_X11Display     x11Display;
        R_XCBConnection  xcbConnection;
};

/**
 * @brief Detects GPU capabilities and chooses appropriate backend
 * @param pCapabilities Output structure for GPU capabilities
 * @return Chosen window backend type
 */
R_ENTRY_API enum R_WindowHandleBackend R_DetectCapabilities (struct R_Capabilities* pCapabilities);

/**
 * @brief Initializes the Wayland window
 * @param pApplicationInfo Application information structure
 * @param pStyle Window style style (can be NULL for default)
 * @return Window handle on success, NULL on failure
 */
R_ENTRY_API R_WaylandWindow
R_InitWaylandWindow (struct R_ApplicationInfo* pApplicationInfo, const struct R_WindowHandleStyle* pStyle);

/**
 * @brief Initializes the X11 window
 * @param pApplicationInfo Application information structure
 * @param pStyle Window style style (can be NULL for default)
 * @return Window handle on success, 0 on failure
 */
R_ENTRY_API R_X11Window
R_InitX11Window (struct R_ApplicationInfo* pApplicationInfo, const struct R_WindowHandleStyle* pStyle);

/**
 * @brief Initializes the XCB window
 * @param pApplicationInfo Application information structure
 * @param pStyle Window style style (can be NULL for default)
 * @return Window handle on success, 0 on failure
 */
R_ENTRY_API R_XCBWindow
R_InitXCBWindow (struct R_ApplicationInfo* pApplicationInfo, const struct R_WindowHandleStyle* pStyle);

/**
 * @brief Applies window style to an existing window
 * @param backend Window backend type
 * @param pWindowHandle Generic window handle
 * @param pStyle Window style style
 */
R_ENTRY_API void R_ApplyWindowStyle (
    enum R_WindowHandleBackend        backend,
    union R_WindowHandleHandle*       pWindowHandle,
    const struct R_WindowHandleStyle* pStyle);

/**
 * @brief Sets window title with proper app name handling
 * @param backend Window backend type
 * @param pWindowHandle Generic window handle
 * @param pApplicationInfo Application information for app name
 * @param pCustomTitle Optional custom title (overrides app name if provided)
 */
R_ENTRY_API void R_SetWindowTitle (
    enum R_WindowHandleBackend      backend,
    union R_WindowHandleHandle*     pWindowHandle,
    const struct R_ApplicationInfo* pApplicationInfo,
    const char*                     pCustomTitle);

/**
 * @brief Gets the Wayland display handle
 * @return Display handle on success, NULL on failure
 */
R_ENTRY_API R_WaylandDisplay R_GetWaylandDisplay (void);

/**
 * @brief Gets the X11 display handle
 * @return Display handle on success, NULL on failure
 */
R_ENTRY_API R_X11Display R_GetX11Display (void);

/**
 * @brief Gets the XCB connection handle
 * @return Connection handle on success, NULL on failure
 */
R_ENTRY_API R_XCBConnection R_GetXCBConnection (void);

/**
 * @brief Sets the Wayland window to fullscreen mode
 * @param window Window handle
 * @param fullscreen true to enable fullscreen, false to disable
 */
R_ENTRY_API void R_WindowHandleSetFullscreen (R_WaylandWindow window, bool fullscreen);

/**
 * @brief Sets the X11 window to fullscreen mode
 * @param window Window handle
 * @param fullscreen true to enable fullscreen, false to disable
 */
R_ENTRY_API void R_X11WindowSetFullscreen (R_X11Window window, bool fullscreen);

/**
 * @brief Sets the XCB window to fullscreen mode
 * @param window Window handle
 * @param fullscreen true to enable fullscreen, false to disable
 */
R_ENTRY_API void R_XCBWindowSetFullscreen (R_XCBWindow window, bool fullscreen);

/**
 * @brief Sets the Wayland window title
 * @param window Window handle
 * @param pTitle Window title string (UTF-8)
 */
R_ENTRY_API void R_WindowHandleSetTitle (R_WaylandWindow window, const char* pTitle);

/**
 * @brief Sets the X11 window title
 * @param window Window handle
 * @param pTitle Window title string (UTF-8)
 */
R_ENTRY_API void R_X11WindowSetTitle (R_X11Window window, const char* pTitle);

/**
 * @brief Sets the XCB window title
 * @param window Window handle
 * @param pTitle Window title string (UTF-8)
 */
R_ENTRY_API void R_XCBWindowSetTitle (R_XCBWindow window, const char* pTitle);

/**
 * @brief Gets the Wayland window dimensions
 * @param window Window handle
 * @param pWidth Output pointer for width
 * @param pHeight Output pointer for height
 */
R_ENTRY_API void R_WindowHandleGetSize (R_WaylandWindow window, int* pWidth, int* pHeight);

/**
 * @brief Gets the X11 window dimensions
 * @param window Window handle
 * @param pWidth Output pointer for width
 * @param pHeight Output pointer for height
 */
R_ENTRY_API void R_X11WindowGetSize (R_X11Window window, int* pWidth, int* pHeight);

/**
 * @brief Gets the XCB window dimensions
 * @param window Window handle
 * @param pWidth Output pointer for width
 * @param pHeight Output pointer for height
 */
R_ENTRY_API void R_XCBWindowGetSize (R_XCBWindow window, int* pWidth, int* pHeight);

/**
 * @brief Gets the Wayland surface from the window
 * @param window Window handle
 * @return Wayland surface pointer, or NULL on failure
 */
R_ENTRY_API struct wl_surface* R_WaylandWindowGetSurface (R_WaylandWindow window);

/**
 * @brief Gets the Wayland display from the window
 * @param window Window handle
 * @return Wayland display pointer, or NULL on failure
 */
R_ENTRY_API struct wl_display* R_WaylandWindowGetDisplay (R_WaylandWindow window);

/**
 * @brief Gets the X11 display from the window
 * @param window Window handle
 * @return X11 display pointer, or NULL on failure
 */
R_ENTRY_API Display* R_X11WindowGetDisplay (R_X11Window window);

/**
 * @brief Gets the XCB connection from the window
 * @param window Window handle
 * @return XCB connection pointer, or NULL on failure
 */
R_ENTRY_API xcb_connection_t* R_XCBWindowGetConnection (R_XCBWindow window);

/**
 * @brief Waits for the Wayland compositor to provide window configuration
 * @param window Window handle
 * @param pWidth Output pointer for width (can be NULL)
 * @param pHeight Output pointer for height (can be NULL)
 */
R_ENTRY_API void R_WaylandWindowWaitForConfig (R_WaylandWindow window, int* pWidth, int* pHeight);

/**
 * @brief Destroys the Wayland window
 * @param window Window handle
 */
R_ENTRY_API void R_DestroyWaylandWindow (R_WaylandWindow window);

/**
 * @brief Destroys the X11 window
 * @param window Window handle
 */
R_ENTRY_API void R_DestroyX11Window (R_X11Window window);

/**
 * @brief Destroys the XCB window
 * @param window Window handle
 */
R_ENTRY_API void R_DestroyXCBWindow (R_XCBWindow window);

/**
 * @brief Processes window events for the current backend
 * Should be called each frame in the main loop
 */
R_ENTRY_API void R_ProcessWindowEvents (union R_WindowHandleHandle* pWindowHandle);

#elif defined(__ANDROID__)

#include <android/native_window.h>

/**
 * @brief Initializes the Android native window
 * @param pWindow Native window pointer from Android
 * @return true on success, false on failure
 */
R_ENTRY_API bool R_InitAndroidWindow (ANativeWindow* pWindow);

/**
 * @brief Gets the Android native window
 * @return Native window pointer
 */
R_ENTRY_API ANativeWindow* R_GetAndroidWindow (void);

/**
 * @brief Gets the Android window dimensions
 * @param pWidth Output pointer for width
 * @param pHeight Output pointer for height
 */
R_ENTRY_API void R_AndroidWindowGetSize (int* pWidth, int* pHeight);

/**
 * @brief Destroys the Android window
 */
R_ENTRY_API void R_DestroyAndroidWindow (void);

#endif // defined(__ANDROID__)
