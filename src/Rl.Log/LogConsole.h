#ifndef RL_LOG_LOG_CONSOLE_H
#define RL_LOG_LOG_CONSOLE_H

#include "Rl.Log/LogLevel.h"

#ifdef __ANDROID__
#include <android/native_activity.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace rl
{

/** Platform-specific handle for log output */
struct LogHandle
{
#ifdef __ANDROID__
                struct android_app* app = nullptr; /**< Android app instance */
#elif defined(_WIN32)
                HWND      hwnd      = nullptr; /**< Windows window handle */
                HINSTANCE hInstance = nullptr; /**< Windows instance handle */
#elif defined(__linux__)
                void* display = nullptr; /**< Linux display */
                void* window  = nullptr; /**< Linux window */
#endif
};

/** Abstract interface for platform-specific console output */
class LogConsole
{
        public:
                virtual ~LogConsole() = default;
                /** Writes a message to the console
                 * @param level The log level
                 * @param message The message to write */
                virtual void write(LogLevel level, const char* message) noexcept = 0;
                /** Sets the console color for the given log level
                 * @param level The log level */
                virtual void setColor(LogLevel level) noexcept = 0;
                /** Resets the console color to default */
                virtual void resetColor() noexcept = 0;
};

/** Creates a platform-specific console instance
 * @param handle Platform-specific log handle
 * @return Pointer to the created console */
LogConsole* createConsole(const LogHandle& handle) noexcept;

} // namespace rl

#endif // RL_LOG_LOG_CONSOLE_H
