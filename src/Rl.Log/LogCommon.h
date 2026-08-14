#ifndef RL_LOG_LOG_COMMON_H
#define RL_LOG_LOG_COMMON_H

#include <cstddef>
#include <cstdint>

namespace rl
{

/** Size of the log message buffer */
constexpr size_t LOG_BUFFER_SIZE = 4096;
/** Maximum number of stack frames to capture */
constexpr size_t logMaxStackFrames = 64;

} // namespace rl

#endif // RL_LOG_LOG_COMMON_H
