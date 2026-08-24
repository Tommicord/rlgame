#include "rlgame.client/render/triangle_test.h"
#include "rlgame.base/game/game_renderer_subsystem.h"
#include "rlgame.base/cvulkan/cvulkan_surface.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#if defined(_WIN32)
LRESULT CALLBACK
WindowProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage (0);
        return 0;
    default:
        return DefWindowProc (hWnd, uMsg, wParam, lParam);
    }
}
#endif

int
main (int argc, char** argv)
{
    static size_t heapSize = 1024 * 1024 * 128;
    if (R_CSTL_HeapInit (heapSize) != R_CSTL_OK)
    {
        fprintf (stderr, "Failed to initialize heap\n");
        return 1;
    }
    if (R_CSTL_LogInit () != R_CSTL_OK)
    {
        fprintf (stderr, "Failed to initialize logging\n");
        R_CSTL_HeapShutdown ();
        return 1;
    }
#if defined(_WIN32)
    HINSTANCE hInstance = GetModuleHandle (NULL);

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "TriangleTestWindow";
    wc.hbrBackground = (HBRUSH)GetStockObject (BLACK_BRUSH);
    wc.hCursor = LoadCursor (NULL, IDC_ARROW);

    if (!RegisterClass (&wc))
    {
        R_CSTL_LOG_ERROR ("Failed to register window class");
        R_CSTL_LogShutdown ();
        R_CSTL_HeapShutdown ();
        return 1;
    }

    HWND hWnd = CreateWindowEx (
        0,
        "TriangleTestWindow",
        "Triangle Test",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        800,
        600,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (!hWnd)
    {
        R_CSTL_LOG_ERROR ("Failed to create window");
        R_CSTL_LogShutdown ();
        R_CSTL_HeapShutdown ();
        return 1;
    }

    ShowWindow (hWnd, SW_SHOW);
    UpdateWindow (hWnd);

#elif defined(__linux__)
    Display* pDisplay = XOpenDisplay (NULL);
    if (!pDisplay)
    {
        R_CSTL_LOG_ERROR ("Failed to open X display");
        R_CSTL_LogShutdown ();
        R_CSTL_HeapShutdown ();
        return 1;
    }

    int    screen = DefaultScreen (pDisplay);
    Window root = RootWindow (pDisplay, screen);

    XSetWindowAttributes swa = {0};
    swa.event_mask = ExposureMask | KeyPressMask;
    Window window = XCreateWindow (
        pDisplay,
        root,
        0,
        0,
        800,
        600,
        0,
        CopyFromParent,
        InputOutput,
        CopyFromParent,
        CWEventMask,
        &swa);

    XMapWindow (pDisplay, window);
    XStoreName (pDisplay, window, "Triangle Test");

#else
    R_CSTL_LOG_ERROR ("Unsupported platform");
    R_CSTL_LogShutdown ();
    R_CSTL_HeapShutdown ();
    return 1;
#endif
    struct R_TriangleTest_Context triangleContext;
    memset (&triangleContext, 0, sizeof (triangleContext));

    struct R_Game_PipelineContextCreateInfo pipelineCreateInfo = {0};
    pipelineCreateInfo.pApplicationName = "rlgame triangle test";
#if defined(_WIN32)
    pipelineCreateInfo.hInstance = hInstance;
    pipelineCreateInfo.hWnd = hWnd;
#elif defined(__linux__)
    pipelineCreateInfo.pDisplay = pDisplay;
    pipelineCreateInfo.window = window;
#endif

    enum R_GameError result = R_TriangleTestInitialize (&triangleContext, &pipelineCreateInfo);
    if (result != R_GAME_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to initialize triangle test: %d", result);
#if defined(_WIN32)
        DestroyWindow (hWnd);
#elif defined(__linux__)
        XCloseDisplay (pDisplay);
#endif
        R_CSTL_LogShutdown ();
        R_CSTL_HeapShutdown ();
        return 1;
    }
    struct R_GameRendererSubsystem* rendererSubsystem = NULL;
    memset (&rendererSubsystem, 0, sizeof (rendererSubsystem));

    rendererSubsystem = R_GameRenderer_NewSubsystem (rendererSubsystem);
    if (!rendererSubsystem)
    {
        R_CSTL_LOG_ERROR ("Failed to create renderer subsystem");
        R_TriangleTestCleanup (&triangleContext);
#if defined(_WIN32)
        DestroyWindow (hWnd);
#elif defined(__linux__)
        XCloseDisplay (pDisplay);
#endif
        R_CSTL_LogShutdown ();
        R_CSTL_HeapShutdown ();
        return 1;
    }
    result = R_GameRenderer_SetPipelineContext (rendererSubsystem, &triangleContext.pipelineContext);
    if (result != R_GAME_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to set pipeline context: %d", result);
        R_GameRenderer_DeleteSubsystem (rendererSubsystem);
        R_TriangleTestCleanup (&triangleContext);
#if defined(_WIN32)
        DestroyWindow (hWnd);
#elif defined(__linux__)
        XCloseDisplay (pDisplay);
#endif
        R_CSTL_LogShutdown ();
        R_CSTL_HeapShutdown ();
        return 1;
    }
    result = R_GameRenderer_SubsystemStart (rendererSubsystem);
    if (result != R_GAME_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to start renderer subsystem: %d", result);
        R_GameRenderer_DeleteSubsystem (rendererSubsystem);
        R_TriangleTestCleanup (&triangleContext);
#if defined(_WIN32)
        DestroyWindow (hWnd);
#elif defined(__linux__)
        XCloseDisplay (pDisplay);
#endif
        R_CSTL_LogShutdown ();
        R_CSTL_HeapShutdown ();
        return 1;
    }
    result = R_TriangleTestRegisterWithRenderer (&triangleContext, rendererSubsystem);
    if (result != R_GAME_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to register triangle test with renderer: %d", result);
        R_GameRenderer_SubsystemStop (rendererSubsystem);
        R_GameRenderer_DeleteSubsystem (rendererSubsystem);
        R_TriangleTestCleanup (&triangleContext);
#if defined(_WIN32)
        DestroyWindow (hWnd);
#elif defined(__linux__)
        XCloseDisplay (pDisplay);
#endif
        R_CSTL_LogShutdown ();
        R_CSTL_HeapShutdown ();
        return 1;
    }
    bool      running = true;
    int       frameCount = 0;
    const int maxFrames = 1000; // Run for 100 frames then exit

    while (running && frameCount < maxFrames)
    {
#if defined(_WIN32)
        MSG msg;
        while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                running = false;
                break;
            }
            TranslateMessage (&msg);
            DispatchMessage (&msg);
        }
#elif defined(__linux__)
        XEvent event;
        while (XPending (pDisplay))
        {
            XNextEvent (pDisplay, &event);
            if (event.type == KeyPress)
            {
                running = false;
                break;
            }
        }
#endif
        result = R_TriangleTestRenderFrame (&triangleContext);
        if (result != R_GAME_OK)
        {
            R_CSTL_LOG_ERROR ("Failed to render frame: %d", result);
            running = false;
        }

        frameCount++;
    }

    R_GameRenderer_SubsystemStop (rendererSubsystem);
    R_GameRenderer_DeleteSubsystem (rendererSubsystem);
    R_TriangleTestCleanup (&triangleContext);

#if defined(_WIN32)
    DestroyWindow (hWnd);
#elif defined(__linux__)
    XCloseDisplay (pDisplay);
#endif
    R_CSTL_LogShutdown ();
    R_CSTL_HeapShutdown ();

    return 0;
}
