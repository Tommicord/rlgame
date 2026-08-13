#include "Rl.Log/LogConsoleAndroid.h"

#ifdef __ANDROID__

#include "Rl.Log/LogLevel.h"

namespace rl
{

void LogConsoleAndroid::write(LogLevel level, const char* message) noexcept
{
        if (message == nullptr)
                return;
        __android_log_print(toAndroidPriority(level), "rlgame", "%s", message);
}

void LogConsoleAndroid::setColor(LogLevel level) noexcept
{
        // Android logcat doesn't support color, ignore
}

void LogConsoleAndroid::resetColor() noexcept
{
        // Android logcat doesn't support color, ignore
}

android_LogPriority LogConsoleAndroid::toAndroidPriority(LogLevel level) const noexcept
{
        switch (level)
        {
        case LogLevel::Trace:
                return ANDROID_LOG_VERBOSE;
        case LogLevel::Debug:
                return ANDROID_LOG_DEBUG;
        case LogLevel::Info:
                return ANDROID_LOG_INFO;
        case LogLevel::Warning:
                return ANDROID_LOG_WARN;
        case LogLevel::Error:
                return ANDROID_LOG_ERROR;
        default:
                return ANDROID_LOG_INFO;
        }
}

} // namespace rl

#endif // __ANDROID__
