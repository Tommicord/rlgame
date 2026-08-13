#ifndef RL_LOG_LOG_H
#define RL_LOG_LOG_H

#include "Rl.Log/LogCommon.h"
#include "Rl.Log/LogConfig.h"
#include "Rl.Log/LogConsole.h"
#include "Rl.Log/LogFormatter.h"
#include "Rl.Log/LogLevel.h"
#include "Rl.Log/LogMutex.h"
#include "Rl.Log/LogStackTrace.h"

#include <cstring>
#include <utility>
#include <vector>

namespace rl
{

/** Array wrapper for logging purposes */
template <typename T> struct LogArray
{
                const T* data; /**< Pointer to array data */
                size_t   count; /**< Number of elements in array */
};

/** Creates a LogArray from a pointer and count
 * @param data Pointer to the array data
 * @param count Number of elements
 * @return LogArray wrapper */
template <typename T> LogArray<T> makeLogArray(const T* data, size_t count)
{
        return LogArray<T>{data, count};
}

/** Main logging class providing static methods for logging */
class Log
{
        public:
                /** Initializes the logging system
                 * @param handle Platform-specific log handle */
                static void initialize(const LogHandle& handle) noexcept;
                /** Shuts down the logging system */
                static void shutdown() noexcept;
                /** Sets the logging configuration
                 * @param config The configuration to set */
                static void setConfig(const LogConfig& config) noexcept;
                /** Returns the current logging configuration
                 * @return Reference to the current configuration */
                static const LogConfig& getConfig() noexcept;

                /** Logs a trace message
                 * @param format Format string
                 * @param args Arguments for format string */
                template <typename... Args>
                static void trace(const char* format, Args&&... args) noexcept
                {
                        log(LogLevel::Trace, format, std::forward<Args>(args)...);
                }

                /** Logs a debug message
                 * @param format Format string
                 * @param args Arguments for format string */
                template <typename... Args>
                static void debug(const char* format, Args&&... args) noexcept
                {
                        log(LogLevel::Debug, format, std::forward<Args>(args)...);
                }

                /** Logs an info message
                 * @param format Format string
                 * @param args Arguments for format string */
                template <typename... Args>
                static void info(const char* format, Args&&... args) noexcept
                {
                        log(LogLevel::Info, format, std::forward<Args>(args)...);
                }

                /** Logs a warning message
                 * @param format Format string
                 * @param args Arguments for format string */
                template <typename... Args>
                static void warning(const char* format, Args&&... args) noexcept
                {
                        log(LogLevel::Warning, format, std::forward<Args>(args)...);
                }

                /** Logs an error message with stack trace
                 * @param format Format string
                 * @param args Arguments for format string */
                template <typename... Args>
                static void error(const char* format, Args&&... args) noexcept
                {
                        logWithStackTrace(LogLevel::Error, format, std::forward<Args>(args)...);
                }

                /** Logs a message at the specified level
                 * @param level The log level
                 * @param format Format string
                 * @param args Arguments for format string */
                template <typename... Args>
                static void log(LogLevel level, const char* format, Args&&... args) noexcept
                {
                        loginternal(level, format, std::forward<Args>(args)...);
                }

        private:
                /** internal logging implementation
                 * @param level The log level
                 * @param format Format string
                 * @param args Arguments for format string */
                template <typename... Args>
                static void
                loginternal(LogLevel level, const char* format, Args&&... args) noexcept;

                /** Logs a message with stack trace
                 * @param level The log level
                 * @param format Format string
                 * @param args Arguments for format string */
                template <typename... Args>
                static void
                logWithStackTrace(LogLevel level, const char* format, Args&&... args) noexcept;

                /** Checks if a log level should be logged
                 * @param level The log level to check
                 * @return true if should log, false otherwise */
                static bool shouldLog(LogLevel level) noexcept;
                /** Initializes the console output */
                static void initializeConsole() noexcept;
                /** Converts log level to string
                 * @param level The log level
                 * @return String representation */
                static const char* logLevelToString(LogLevel level) noexcept;
                /** Returns internal configuration reference
                 * @return Reference to internal configuration */
                static LogConfig& getConfiginternal() noexcept;
                /** Returns the console instance
                 * @return Pointer to console */
                static LogConsole* getConsole() noexcept;
                /** Returns the logging mutex
                 * @return Reference to mutex */
                static LogMutex& getMutex() noexcept;
};

template <typename... Args>
void Log::loginternal(LogLevel level, const char* format, Args&&... args) noexcept
{
        if (!getConsole())
                return;
        LogLock lock(getMutex());

        if (!shouldLog(level))
                return;
        initializeConsole();

        char   buffer[LOG_BUFFER_SIZE]{};
        size_t pos = 0;

        LogConfig& config = getConfiginternal();
        if (config.enableTimestamp)
        {
                // TODO: Implement timestamp formatting
        }

        const char* levelStr = logLevelToString(level);
        size_t      levelLen = strlen(levelStr);

        if (pos + levelLen + 3 < sizeof(buffer))
        {
                buffer[pos++] = '[';
                memcpy(buffer + pos, levelStr, levelLen);
                pos += levelLen;
                buffer[pos++] = ']';
                buffer[pos++] = ' ';
                buffer[pos]   = '\0';
        }

        int formatted = LogFormatter::format(buffer + pos, sizeof(buffer) - pos, format,
                                             std::forward<Args>(args)...);

        if (formatted > 0)
        {
                pos += static_cast<size_t>(formatted);
        }

        LogConsole* console = getConsole();
        if (config.enableColors)
        {
                console->setColor(level);
        }
        console->write(level, buffer);
        if (config.enableColors)
        {
                console->resetColor();
        }
}

template <typename... Args>
void Log::logWithStackTrace(LogLevel level, const char* format, Args&&... args) noexcept
{
        loginternal(level, format, std::forward<Args>(args)...);

        LogConfig& config = getConfiginternal();
        if (config.enableStackTrace)
        {
                std::vector<StackFrame> frames(LOG_MAX_STACK_FRAMES);
                int frameCount = LogStackTrace::capture(frames.data(), LOG_MAX_STACK_FRAMES);

                if (frameCount > 0)
                {
                        for (int i = 0; i < frameCount; i++)
                        {
                                LogStackTrace::demangle(&frames[i]);
                        }
                        std::vector<char> stackBuffer(LOG_BUFFER_SIZE);
                        LogStackTrace::formatStackTrace(stackBuffer.data(), sizeof(stackBuffer),
                                                        frames.data(), frameCount);
                        if (stackBuffer[0] != '\0')
                        {
                                LogConsole* console = getConsole();
                                if (config.enableColors)
                                {
                                        console->setColor(level);
                                }

                                console->write(level, stackBuffer.data());

                                if (config.enableColors)
                                {
                                        console->resetColor();
                                }
                        }
                }
        }
}

} // namespace rl

#endif // RL_LOG_LOG_H
