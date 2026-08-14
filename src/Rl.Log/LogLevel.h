#ifndef RL_LOG_LOG_LEVEL_H
#define RL_LOG_LOG_LEVEL_H

namespace rl
{

/** Enumeration of log severity levels */
enum class LogLevel
{
  Trace, /**< Most verbose logging level */
  Debug, /**< Debug information */
  Info, /**< General informational messages */
  Warning, /**< Warning messages */
  Error /**< Error messages */
};

/** Converts a log level to its string representation
 * @param level The log level to convert
 * @return String representation of the log level */
const char* logLevelToString(LogLevel level) noexcept;

} // namespace rl

#endif // RL_LOG_LOG_LEVEL_H
