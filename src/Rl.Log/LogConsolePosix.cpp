#include "Rl.Log/LogConsolePosix.h"

#if !defined(_WIN32) && !defined(__ANDROID__)

#include <cstdio>
#include "Rl.Log/LogLevel.h"

namespace rl
{

void LogConsolePosix::write(LogLevel level, const char* message) noexcept
{
        if (message == nullptr)
                return;
        printf("%s\n", message);
}

void LogConsolePosix::setColor(LogLevel level) noexcept
{
        const char* colorCode = nullptr;

        switch (level)
        {
        case LogLevel::Trace:
                colorCode = "\033[37m"; // White (bright)
                break;
        case LogLevel::Debug:
                colorCode = "\033[32m"; // Green
                break;
        case LogLevel::Info:
                colorCode = "\033[36m"; // Cyan
                break;
        case LogLevel::Warning:
                colorCode = "\033[33m"; // Yellow
                break;
        case LogLevel::Error:
                colorCode = "\033[31m"; // Red
                break;
        }

        if (colorCode)
        {
                printf("%s", colorCode);
        }
}

void LogConsolePosix::resetColor() noexcept
{
        printf("\033[0m");
}

} // namespace rl

#endif // !defined(_WIN32) && !defined(__ANDROID__)
