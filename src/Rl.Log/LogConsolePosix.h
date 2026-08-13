#ifndef RL_LOG_LOG_CONSOLE_POSIX_H
#define RL_LOG_LOG_CONSOLE_POSIX_H

#if !defined(_WIN32) && !defined(__ANDROID__)

#include "Rl.Log/LogConsole.h"

namespace rl
{

/** POSIX-specific console implementation using stdout/stderr */
class LogConsolePosix : public LogConsole
{
        public:
                /** Constructs a POSIX console
                 * @param handle Platform-specific log handle */
                explicit LogConsolePosix(const LogHandle& handle) noexcept = default;
                /** Destroys the console */
                ~LogConsolePosix() override = default;

                /** Writes a message to stdout/stderr
                 * @param level The log level
                 * @param message The message to write */
                void write(LogLevel level, const char* message) noexcept override;
                /** Sets the console color using ANSI codes
                 * @param level The log level */
                void setColor(LogLevel level) noexcept override;
                /** Resets the console color to default */
                void resetColor() noexcept override;
};

} // namespace rl

#endif // !defined(_WIN32) && !defined(__ANDROID__)

#endif // RL_LOG_LOG_CONSOLE_POSIX_H
