#ifndef RL_BASE_MAIN_H
#define RL_BASE_MAIN_H

#ifdef __ANDROID__
#include <android_native_app_glue.h>
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace rl
{
#ifdef __ANDROID__
using AndroidMainFunc = void (*)(struct android_app* state);
#elif defined(_WIN32)
using Win32MainFunc = void (*)(HINSTANCE hInstance,
                               HINSTANCE hPrevInstance,
                               PWSTR     pCmdLine,
                               int       nCmdShow);
#else
using AnyMainFunc = void (*)(int argc, char** argv);
#endif
struct MainProvider
{
    MainProvider() noexcept = default;
#ifdef __ANDROID__
    static AndroidMainFunc launch;
#elif defined(_WIN32)
    static Win32MainFunc launch;
#else
    static AnyMainFunc launch;
#endif
};

struct MainLauncher
{
#ifdef __ANDROID__
    using CallbackType = AndroidMainFunc;
#elif defined(_WIN32)
    using CallbackType = Win32MainFunc;
#else
    using CallbackType = AnyMainFunc;
#endif
    CallbackType func;

    MainLauncher(CallbackType callback) : func(callback)
    {
    }
#ifdef __ANDROID__
    int launch(struct android_app* state) const
    {
      func(state);
      return 0;
    }
#elif defined(_WIN32)
    int launch(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) const
    {
      func(hInstance, hPrevInstance, pCmdLine, nCmdShow);
      return 0;
    }
#else
    int launch(int argc, char** argv) const
    {
      func(argc, argv);
      return 0;
    }
#endif
};
} // namespace rl

#ifdef __ANDROID__
extern "C" void android_main(struct android_app* state)
{
  rl::MainLauncher launcher([](struct android_app* state) { rl::MainProvider::onLaunch(state) });
  return launcher.onLaunch(state);
}
#elif defined(_WIN32)

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
  rl::MainLauncher launcher(
      [](HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
      { rl::MainProvider::launch(hInstance, hPrevInstance, pCmdLine, nCmdShow); });
  return launcher.launch(hInstance, hPrevInstance, pCmdLine, nCmdShow);
}
#else
int main(int argc, char** argv)
{

  rl::MainLauncher launcher([](int argc, char** argv) { rl::MainProvider::onLaunch(argc, argv); });
  return launcher.onLaunch(argc, argv);
}
#endif

#endif // RL_BASE_MAIN_H
