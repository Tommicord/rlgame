#include <gtest/gtest.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "Rl.Log/Log.h"

int main(int argc, char** argv)
{
        ::testing::InitGoogleTest(&argc, argv);

        rl::LogHandle logHandle;
#if defined(_WIN32)
        HWND hwnd           = GetConsoleWindow();
        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
        logHandle.hwnd      = hwnd;
        logHandle.hInstance = hInstance;
#endif
        rl::Log::initialize(logHandle);

        rl::LogConfig logConfig;
        logConfig.minLevel         = rl::LogLevel::Debug;
        logConfig.enableTimestamp  = true;
        logConfig.enableStackTrace = true;
        logConfig.enableColors     = true;
        rl::Log::setConfig(logConfig);

        return RUN_ALL_TESTS();
}
