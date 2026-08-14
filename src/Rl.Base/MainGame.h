#ifndef RL_BASE_MAIN_GAME_H
#define RL_BASE_MAIN_GAME_H

#ifdef __ANDROID__
#include <android/native_activity.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

#include "Rl.Base/GameDevice.h"
#include "Rl.Base/GameInput.h"
#include "Rl.Base/GameTime.h"
#include "Rl.Player/Player.h"
#include "Rl.Player/PlayerController.h"

namespace rl
{

struct IMainGame
{
    ~IMainGame() noexcept           = default;
    virtual void onCreateCallback() = 0;
    virtual void onResizeCallback() = 0;
    virtual void onLaunch()         = 0;
};
struct MainGame;
#if defined(__ANDROID__)
struct MainGameAndroidHandle final
{
    // Android native app state
    struct android_app*   app;
    struct ANativeWindow* window;
    MainGameAndroidHandle() noexcept : app(nullptr), window(nullptr)
    {
    }
};
#elif defined(_WIN32)
struct MainGameWin32Handle final
{
    MSG       msg;
    HWND      hwnd;
    PWSTR     pCmdLine;
    HINSTANCE hInstance;

    MainGameWin32Handle() noexcept : msg{}, hwnd(nullptr), pCmdLine(nullptr), hInstance(nullptr)
    {
    }
};
#elif defined(__linux__)
struct MainGameLinuxHandle final
{
    // X11 window handle
    void* display;
    void* window;
    MainGameLinuxHandle() noexcept : display(nullptr), window(nullptr)
    {
    }
};
#endif

struct MainGame final : public IMainGame
{
#ifdef __ANDROID__
    MainGameAndroidHandle handle{};
#elif defined(_WIN32)
    MainGameWin32Handle handle{};
#elif defined(__linux__)
    MainGameLinuxHandle handle{};
#endif
    GameDeviceInstance device;
    GameTime           time;

    MainGame() noexcept : device(handle)
    {
    }
    ~MainGame() = default;
    void onCreateCallback() override;
    void onResizeCallback() override;
    void onLaunch() override;
};

} // namespace rl

#endif // RL_BASE_MAIN_H
