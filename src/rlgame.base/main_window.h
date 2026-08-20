#pragma once

#include <stdint.h>
#include <stdbool.h>

struct R_ApplicationInfo;

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define R_WIN32_HWND      HWND
#define R_WIN32_HINSTANCE HINSTANCE

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
 * @brief Converts a UTF-8 string to a wide character string (Unicode)
 * @param pInput Input UTF-8 string
 * @param ppOut Output pointer to wide character string (allocated on heap)
 */
void R_UnicodeFromString (const char* pInput, wchar_t** ppOut);

/**
 * @brief Centers the window on its current monitor
 * @param hwnd Window handle to center
 */
void R_WindowCenter (HWND hwnd);

/**
 * @brief Gets the global window handle
 * @return Current window handle (may be NULL)
 */
HWND R_GetWindowHandle (void);

/**
 * @brief Initializes the Win32 main window
 * @param hInstance Application instance handle
 * @param pApplicationInfo Application information structure
 * @param nCmdShow Show window command (SW_SHOW, SW_HIDE, etc.)
 * @return 1 on success, 0 on failure
 */
int R_InitWinMain (HINSTANCE hInstance, struct R_ApplicationInfo* pApplicationInfo, int nCmdShow);

/**
 * @brief Sets the window to fullscreen mode
 * @param hwnd Window handle
 * @param fullscreen true to enable fullscreen, false to disable
 */
void R_WindowSetFullscreen (HWND hwnd, bool fullscreen);

/**
 * @brief Sets the window to borderless mode
 * @param hwnd Window handle
 * @param borderless true to enable borderless, false to disable
 */
void R_WindowSetBorderless (HWND hwnd, bool borderless);

/**
 * @brief Sets the window to resizable or fixed size
 * @param hwnd Window handle
 * @param resizable true to enable resizing, false to disable
 */
void R_WindowSetResizable (HWND hwnd, bool resizable);

/**
 * @brief Minimizes the window
 * @param hwnd Window handle
 */
void R_WindowMinimize (HWND hwnd);

/**
 * @brief Maximizes the window
 * @param hwnd Window handle
 */
void R_WindowMaximize (HWND hwnd);

/**
 * @brief Restores the window from minimized or maximized state
 * @param hwnd Window handle
 */
void R_WindowRestore (HWND hwnd);

/**
 * @brief Hides the window
 * @param hwnd Window handle
 */
void R_WindowHide (HWND hwnd);

/**
 * @brief Shows the window
 * @param hwnd Window handle
 */
void R_WindowShow (HWND hwnd);

/**
 * @brief Gets the window client area dimensions
 * @param hwnd Window handle
 * @param pWidth Output pointer for width
 * @param pHeight Output pointer for height
 */
void R_WindowGetClientSize (HWND hwnd, int* pWidth, int* pHeight);

/**
 * @brief Gets the window dimensions (including borders)
 * @param hwnd Window handle
 * @param pWidth Output pointer for width
 * @param pHeight Output pointer for height
 */
void R_WindowGetWindowSize (HWND hwnd, int* pWidth, int* pHeight);

/**
 * @brief Gets the window position
 * @param hwnd Window handle
 * @param pX Output pointer for X position
 * @param pY Output pointer for Y position
 */
void R_WindowGetPosition (HWND hwnd, int* pX, int* pY);

/**
 * @brief Sets the window position
 * @param hwnd Window handle
 * @param x X position
 * @param y Y position
 */
void R_WindowSetPosition (HWND hwnd, int x, int y);

/**
 * @brief Sets the window size
 * @param hwnd Window handle
 * @param width Window width
 * @param height Window height
 */
void R_WindowSetSize (HWND hwnd, int width, int height);

/**
 * @brief Sets the window title
 * @param hwnd Window handle
 * @param pTitle Window title string (UTF-8)
 */
void R_WindowSetTitle (HWND hwnd, const char* pTitle);

/**
 * @brief Checks if the window is currently fullscreen
 * @param hwnd Window handle
 * @return true if fullscreen, false otherwise
 */
bool R_WindowIsFullscreen (HWND hwnd);

/**
 * @brief Checks if the window is currently minimized
 * @param hwnd Window handle
 * @return true if minimized, false otherwise
 */
bool R_WindowIsMinimized (HWND hwnd);

/**
 * @brief Checks if the window is currently maximized
 * @param hwnd Window handle
 * @return true if maximized, false otherwise
 */
bool R_WindowIsMaximized (HWND hwnd);

/**
 * @brief Checks if the window is currently visible
 * @param hwnd Window handle
 * @return true if visible, false otherwise
 */
bool R_WindowIsVisible (HWND hwnd);

#endif // defined(_WIN32)

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
R_WaylandWindow R_InitWaylandWindow (struct R_ApplicationInfo* pApplicationInfo);

/**
 * @brief Gets the Wayland display handle
 * @return Display handle on success, NULL on failure
 */
R_WaylandDisplay R_GetWaylandDisplay (void);

/**
 * @brief Sets the Wayland window to fullscreen mode
 * @param window Window handle
 * @param fullscreen true to enable fullscreen, false to disable
 */
void R_WaylandWindowSetFullscreen (R_WaylandWindow window, bool fullscreen);

/**
 * @brief Sets the Wayland window title
 * @param window Window handle
 * @param pTitle Window title string (UTF-8)
 */
void R_WaylandWindowSetTitle (R_WaylandWindow window, const char* pTitle);

/**
 * @brief Gets the Wayland window dimensions
 * @param window Window handle
 * @param pWidth Output pointer for width
 * @param pHeight Output pointer for height
 */
void R_WaylandWindowGetSize (R_WaylandWindow window, int* pWidth, int* pHeight);

/**
 * @brief Destroys the Wayland window
 * @param window Window handle
 */
void R_DestroyWaylandWindow (R_WaylandWindow window);

#elif defined(__ANDROID__)

#include <android/native_window.h>

/**
 * @brief Initializes the Android native window
 * @param pWindow Native window pointer from Android
 * @return true on success, false on failure
 */
bool R_InitAndroidWindow (ANativeWindow* pWindow);

/**
 * @brief Gets the Android native window
 * @return Native window pointer
 */
ANativeWindow* R_GetAndroidWindow (void);

/**
 * @brief Gets the Android window dimensions
 * @param pWidth Output pointer for width
 * @param pHeight Output pointer for height
 */
void R_AndroidWindowGetSize (int* pWidth, int* pHeight);

/**
 * @brief Destroys the Android window
 */
void R_DestroyAndroidWindow (void);

#endif // defined(__ANDROID__)
