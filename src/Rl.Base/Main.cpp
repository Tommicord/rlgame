#include "Rl.Base/Main.h"
#include "Rl.Base/MainGame.h"
#include "Rl.Base/GameError.h"
#include "Rl.Base/GameInput.h"
#include "Rl.Log/Log.h"

#include <vulkan/vulkan.hpp>

namespace rl
{

const char* gameName = "Real Game";

int         gameInitialWindowWidth  = 900;
int         gameInitialWindowHeight = 600;
int         gameMinWindowWidth      = 512;
int         gameMinWindowHeight     = 384;

#if defined(__ANDROID__)

MainLauncher::CallbackType MainProvider::onLaunch = [](struct android_app* state)
{
        // TODO: Implement the main loop for Android platform
}

#elif defined(_WIN32)

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
        GameInputInstance::handleWin32Message(hwnd, uMsg, wParam, lParam);

        switch (uMsg)
        {
        case WM_DESTROY:
                {
                        PostQuitMessage(0);
                        return 0;
                }
        case WM_GETMINMAXINFO:
                {
                        MINMAXINFO* info       = reinterpret_cast<MINMAXINFO*>(lParam);
                        info->ptMinTrackSize.x = gameMinWindowWidth;
                        info->ptMinTrackSize.y = gameMinWindowHeight;
                        return 0;
                }
        case WM_SIZE:
                {
                        MainGame* game =
                            reinterpret_cast<MainGame*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
                        GameDeviceInstance* device = &game->device;
                        if (device)
                        {
                                if (wParam == SIZE_MINIMIZED)
                                {
                                        VkDevice vDevice = device->getDevice();
                                        if (vDevice != VK_NULL_HANDLE)
                                        {
                                                vkDeviceWaitIdle(vDevice);
                                        }
                                        return 0;
                                }
                                else
                                {
                                        RECT rect;
                                        if (GetClientRect(hwnd, &rect) && rect.right > 0 &&
                                            rect.bottom > 0)
                                        {
                                                game->onResizeCallback();
                                        }
                                }
                        }
                        return 0;
                }
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

MainLauncher::CallbackType MainProvider::launch =
    [](HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
        const char* CLASS_NAME = "GameWindowClass";

        WNDCLASSEX wc    = {};
        wc.cbSize        = sizeof(WNDCLASSEX);
        wc.lpfnWndProc   = WindowProc;
        wc.hInstance     = hInstance;
        wc.lpszClassName = CLASS_NAME;
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

        RegisterClassEx(&wc);

        RECT workArea{};
        SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

        int width  = workArea.right - workArea.left;
        int height = workArea.bottom - workArea.top;
        int posX   = workArea.left + (width - gameInitialWindowWidth) / 2;
        int posY   = workArea.top + (height - gameInitialWindowHeight) / 2;

        HWND hwnd = CreateWindowEx(0, CLASS_NAME, gameName, WS_OVERLAPPEDWINDOW, posX, posY,
                                   gameInitialWindowWidth, gameInitialWindowHeight, NULL, NULL, hInstance, nullptr);
        if (hwnd == NULL)
        {
                GameError::exitWithError("Failed to create window", "HWND is NULL");
                return;
        }
        MainGame game;
        game.handle.hwnd      = hwnd;
        game.handle.msg       = {};
        game.handle.hInstance = hInstance;
        game.handle.pCmdLine  = pCmdLine;

        LogHandle logHandle;
        logHandle.hwnd      = hwnd;
        logHandle.hInstance = hInstance;
        Log::initialize(logHandle);

        LogConfig logConfig;
        logConfig.minLevel         = LogLevel::Debug;
        logConfig.enableTimestamp  = true;
        logConfig.enableStackTrace = true;
        logConfig.enableColors     = true;
        Log::setConfig(logConfig);

        Log::info("Game booted successfully");

        game.onCreateCallback();
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&game));

        ShowCursor(FALSE);
        RECT rect;
        GetClientRect(hwnd, &rect);
        POINT topLeft     = {rect.left, rect.top};
        POINT bottomRight = {rect.right, rect.bottom};
        ClientToScreen(hwnd, &topLeft);
        ClientToScreen(hwnd, &bottomRight);
        RECT clipRect = {topLeft.x, topLeft.y, bottomRight.x, bottomRight.y};
        ClipCursor(&clipRect);

        POINT center = {(rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2};
        ClientToScreen(hwnd, &center);
        SetCursorPos(center.x, center.y);

        ShowWindow(hwnd, nCmdShow);
        game.onLaunch();
};

#elif defined(__linux__)

MainLauncher::CallbackType MainProvider::onLaunch = [](int argc, char** argv)
{
        // TODO: Implement the main loop for Linux using Wayland
        MainGame game;
        game.onLaunch();
}

#endif

} // namespace rl
