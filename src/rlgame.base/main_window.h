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
R_ENTRY_API void R_WindowCenter (HWND hwnd);

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
R_ENTRY_API void R_WindowSetFullscreen (R_WIN32_HWND hwnd, bool fullscreen);

/**
 * @brief Sets the window to borderless mode
 * @param hwnd Window handle
 * @param borderless true to enable borderless, false to disable
 */
R_ENTRY_API void R_WindowSetBorderless (R_WIN32_HWND hwnd, bool borderless);

/**
 * @brief Sets the window to resizable or fixed size
 * @param hwnd Window handle
 * @param resizable true to enable resizing, false to disable
 */
R_ENTRY_API void R_WindowSetResizable (R_WIN32_HWND hwnd, bool resizable);

/**
 * @brief Minimizes the window
 * @param hwnd Window handle
 */
R_ENTRY_API void R_WindowMinimize (R_WIN32_HWND hwnd);

/**
 * @brief Maximizes the window
 * @param hwnd Window handle
 */
R_ENTRY_API void R_WindowMaximize (R_WIN32_HWND hwnd);

/**
 * @brief Restores the window from minimized or maximized state
 * @param hwnd Window handle
 */
R_ENTRY_API void R_WindowRestore (R_WIN32_HWND hwnd);

/**
 * @brief Hides the window
 * @param hwnd Window handle
 */
R_ENTRY_API void R_WindowHide (R_WIN32_HWND hwnd);

/**
 * @brief Shows the window
 * @param hwnd Window handle
 */
R_ENTRY_API void R_WindowShow (R_WIN32_HWND hwnd);

/**
 * @brief Gets the window client area dimensions
 * @param hwnd Window handle
 * @param pWidth Output pointer for width
 * @param pHeight Output pointer for height
 */
R_ENTRY_API void R_WindowGetClientSize (R_WIN32_HWND hwnd, int* pWidth, int* pHeight);

/**
 * @brief Gets the window dimensions (including borders)
 * @param hwnd Window handle
 * @param pWidth Output pointer for width
 * @param pHeight Output pointer for height
 */
R_ENTRY_API void R_WindowGetWindowSize (R_WIN32_HWND hwnd, int* pWidth, int* pHeight);

/**
 * @brief Gets the window position
 * @param hwnd Window handle
 * @param pX Output pointer for X position
 * @param pY Output pointer for Y position
 */
R_ENTRY_API void R_WindowGetPosition (R_WIN32_HWND hwnd, int* pX, int* pY);

/**
 * @brief Sets the window position
 * @param hwnd Window handle
 * @param x X position
 * @param y Y position
 */
R_ENTRY_API void R_WindowSetPosition (R_WIN32_HWND hwnd, int x, int y);

/**
 * @brief Sets the window size
 * @param hwnd Window handle
 * @param width Window width
 * @param height Window height
 */
R_ENTRY_API void R_WindowSetSize (R_WIN32_HWND hwnd, int width, int height);

/**
 * @brief Sets the window title
 * @param hwnd Window handle
 * @param pTitle Window title string (UTF-8)
 */
R_ENTRY_API void R_WindowSetTitle (R_WIN32_HWND hwnd, const char* pTitle);

/**
 * @brief Checks if the window is currently fullscreen
 * @param hwnd Window handle
 * @return true if fullscreen, false otherwise
 */
R_ENTRY_API bool R_WindowIsFullscreen (R_WIN32_HWND hwnd);

/**
 * @brief Checks if the window is currently minimized
 * @param hwnd Window handle
 * @return true if minimized, false otherwise
 */
R_ENTRY_API bool R_WindowIsMinimized (R_WIN32_HWND hwnd);

/**
 * @brief Checks if the window is currently maximized
 * @param hwnd Window handle
 * @return true if maximized, false otherwise
 */
R_ENTRY_API bool R_WindowIsMaximized (R_WIN32_HWND hwnd);

/**
 * @brief Checks if the window is currently visible
 * @param hwnd Window handle
 * @return true if visible, false otherwise
 */
R_ENTRY_API bool R_WindowIsVisible (R_WIN32_HWND hwnd);

#elif defined(__linux__)

/**
 * @brief Wayland window handle type
 */
typedef void* R_WaylandWindow;

/**
 * @brief Wayland display handle type
 */
typedef void* R_WaylandDisplay;

/**
 * @brief Initializes the Wayland window
 * @param pApplicationInfo Application information structure
 * @return Window handle on success, NULL on failure
 */
R_ENTRY_API R_WaylandWindow R_InitWindow (struct R_ApplicationInfo* pApplicationInfo);

/**
 * @brief Gets the Wayland display handle
 * @return Display handle on success, NULL on failure
 */
R_ENTRY_API R_WaylandDisplay R_GetWaylandDisplay (void);

/**
 * @brief Sets the Wayland window to fullscreen mode
 * @param window Window handle
 * @param fullscreen true to enable fullscreen, false to disable
 */
R_ENTRY_API void R_WindowSetFullscreen (R_WaylandWindow window, bool fullscreen);

/**
 * @brief Sets the Wayland window title
 * @param window Window handle
 * @param pTitle Window title string (UTF-8)
 */
R_ENTRY_API void R_WindowSetTitle (R_WaylandWindow window, const char* pTitle);

/**
 * @brief Gets the Wayland window dimensions
 * @param window Window handle
 * @param pWidth Output pointer for width
 * @param pHeight Output pointer for height
 */
R_ENTRY_API void R_WindowGetSize (R_WaylandWindow window, int* pWidth, int* pHeight);

/**
 * @brief Destroys the Wayland window
 * @param window Window handle
 */
R_ENTRY_API void R_DestroyWindow (R_WaylandWindow window);

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
