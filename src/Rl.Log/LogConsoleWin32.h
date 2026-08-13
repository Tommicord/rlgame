#ifndef RL_LOG_LOG_CONSOLE_WIN32_H
#define RL_LOG_LOG_CONSOLE_WIN32_H

#include "Rl.Log/LogConsole.h"

#ifdef _WIN32
#include <windows.h>

namespace rl
{

/** Windows-specific console implementation using Win32 console API */
class LogConsoleWin32 : public LogConsole
{
        public:
                /** Constructs a Windows console
                 * @param handle Platform-specific log handle */
                explicit LogConsoleWin32(const LogHandle& handle) noexcept;
                /** Destroys the console */
                ~LogConsoleWin32() override = default;

                /** Writes a message to the Windows console
                 * @param level The log level
                 * @param message The message to write */
                void write(LogLevel level, const char* message) noexcept override;
                /** Sets the console color using Win32 API
                 * @param level The log level */
                void setColor(LogLevel level) noexcept override;
                /** Resets the console color to default */
                void resetColor() noexcept override;

        private:
                HANDLE consoleHandle; /**< Windows console handle */
                WORD   originalAttributes; /**< Original console attributes */
};

} // namespace rl

#endif // _WIN32

#endif // RL_LOG_LOG_CONSOLE_WIN32_H
