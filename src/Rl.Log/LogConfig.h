#ifndef RL_LOG_LOG_CONFIG_H
#define RL_LOG_LOG_CONFIG_H

#include "Rl.Log/LogLevel.h"

namespace rl
{

/** Configuration structure for logging system */
struct LogConfig
{
                LogLevel minLevel         = LogLevel::Info; /**< Minimum log level to output */
                bool     enableTimestamp  = true; /**< Whether to include timestamps */
                bool     enableStackTrace = true; /**< Whether to include stack traces on errors */
                bool     enableColors     = true; /**< Whether to enable colored output */
};

} // namespace rl

#endif // RL_LOG_LOG_CONFIG_H
