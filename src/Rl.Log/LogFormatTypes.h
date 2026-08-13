#ifndef RL_LOG_LOG_FORMAT_TYPES_H
#define RL_LOG_LOG_FORMAT_TYPES_H

#include <cstddef>
#include "Rl.Log/LogCommon.h"
#include "Rl.Log/LogFormatSpec.h"

namespace rl
{

/** Type-specific formatting utilities for log messages */
class LogFormatTypes
{
        public:
                /** Formats a signed integer
                 * @param buffer Output buffer
                 * @param pos Current position in buffer
                 * @param bufferSize Size of output buffer
                 * @param value The value to format
                 * @param spec Format specification */
                static void formatint(char*             buffer,
                                      size_t&           pos,
                                      size_t            bufferSize,
                                      int64_t           value,
                                      const FormatSpec& spec) noexcept;
                /** Formats an unsigned integer
                 * @param buffer Output buffer
                 * @param pos Current position in buffer
                 * @param bufferSize Size of output buffer
                 * @param value The value to format
                 * @param spec Format specification */
                static void formatUint(char*             buffer,
                                       size_t&           pos,
                                       size_t            bufferSize,
                                       uint64_t          value,
                                       const FormatSpec& spec) noexcept;
                /** Formats a floating point value
                 * @param buffer Output buffer
                 * @param pos Current position in buffer
                 * @param bufferSize Size of output buffer
                 * @param value The value to format
                 * @param spec Format specification */
                static void formatFloat(char*             buffer,
                                        size_t&           pos,
                                        size_t            bufferSize,
                                        double            value,
                                        const FormatSpec& spec) noexcept;
                /** Formats a string
                 * @param buffer Output buffer
                 * @param pos Current position in buffer
                 * @param bufferSize Size of output buffer
                 * @param str The string to format
                 * @param spec Format specification */
                static void formatString(char*             buffer,
                                         size_t&           pos,
                                         size_t            bufferSize,
                                         const char*       str,
                                         const FormatSpec& spec) noexcept;
                /** Formats a pointer address
                 * @param buffer Output buffer
                 * @param pos Current position in buffer
                 * @param bufferSize Size of output buffer
                 * @param ptr The pointer to format */
                static void formatPointer(char*       buffer,
                                          size_t&     pos,
                                          size_t      bufferSize,
                                          const void* ptr) noexcept;
};

} // namespace rl

#endif // RL_LOG_LOG_FORMAT_TYPES_H
