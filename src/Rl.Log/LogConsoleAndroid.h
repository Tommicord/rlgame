#ifndef RL_LOG_LOG_CONSOLE_ANDROID_H
#define RL_LOG_LOG_CONSOLE_ANDROID_H

#ifdef __ANDROID__

#include <android/log.h>
#include "Rl.Log/LogConsole.h"

namespace rl
{

/** Android-specific console implementation using android_log */
class LogConsoleAndroid : public LogConsole
{
  public:
    /** Constructs an Android console
     * @param handle Platform-specific log handle */
    explicit LogConsoleAndroid(const LogHandle& handle) noexcept = default;
    /** Destroys the console */
    ~LogConsoleAndroid() override = default;

    /** Writes a message to Android log
     * @param level The log level
     * @param message The message to write */
    void write(LogLevel level, const char* message) noexcept override;
    /** Sets the color (no-op on Android)
     * @param level The log level */
    void setColor(LogLevel level) noexcept override;
    /** Resets the color (no-op on Android) */
    void resetColor() noexcept override;

  private:
    /** Converts log level to Android priority
     * @param level The log level
     * @return Android log priority */
    android_LogPriority toAndroidPriority(LogLevel level) const noexcept;
};

} // namespace rl

#endif // __ANDROID__

#endif // RL_LOG_LOG_CONSOLE_ANDROID_H
