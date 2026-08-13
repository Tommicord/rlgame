#ifndef RL_LOG_LOG_FORMATTER_H
#define RL_LOG_LOG_FORMATTER_H

#include "Rl.Log/LogCommon.h"
#include "Rl.Log/LogFormatSpec.h"
#include "Rl.Log/LogFormatTypes.h"

#include <type_traits>

namespace rl
{

template <typename T> struct LogArray;

/** Formatter for log messages with format string support */
class LogFormatter
{
        public:
                /** Formats a message with no arguments
                 * @param buffer Output buffer
                 * @param bufferSize Size of output buffer
                 * @param format Format string
                 * @return Number of characters written */
                static int format(char* buffer, size_t bufferSize, const char* format) noexcept;

                /** Formats a message with arguments
                 * @param buffer Output buffer
                 * @param bufferSize Size of output buffer
                 * @param fmt Format string
                 * @param value First argument to format
                 * @param args Remaining arguments to format
                 * @return Number of characters written */
                template <typename T, typename... Args>
                static int format(char*       buffer,
                                  size_t      bufferSize,
                                  const char* fmt,
                                  T&&         value,
                                  Args&&... args) noexcept
                {
                        if (buffer == nullptr || fmt == nullptr || bufferSize == 0)
                                return -1;

                        size_t      pos = 0;
                        const char* ptr = fmt;

                        while (*ptr != '\0' && pos < bufferSize - 1)
                        {
                                if (*ptr == '%')
                                {
                                        formatValue(buffer, pos, bufferSize, ptr,
                                                    std::forward<T>(value));
                                        if constexpr (sizeof...(Args) > 0)
                                        {
                                                int remaining =
                                                    format(buffer + pos, bufferSize - pos, ptr,
                                                           std::forward<Args>(args)...);
                                                if (remaining >= 0)
                                                        pos += static_cast<size_t>(remaining);
                                        }
                                        else
                                        {
                                                int remaining = LogFormatter::format(
                                                    buffer + pos, bufferSize - pos, ptr);
                                                if (remaining >= 0)
                                                        pos += static_cast<size_t>(remaining);
                                        }
                                        break;
                                }
                                else
                                {
                                        buffer[pos++] = *ptr++;
                                        buffer[pos]   = '\0';
                                }
                        }

                        buffer[pos] = '\0';
                        return static_cast<int>(pos);
                }

        private:
                /** Formats a single value according to format specification
                 * @param buffer Output buffer
                 * @param pos Current position in buffer
                 * @param bufferSize Size of output buffer
                 * @param format Format string pointer
                 * @param value Value to format */
                template <typename T>
                static void formatValue(char*        buffer,
                                        size_t&      pos,
                                        size_t       bufferSize,
                                        const char*& format,
                                        T&&          value) noexcept
                {
                        FormatSpec  spec;
                        const char* end = parseFormatSpec(format, spec);

                        // Array formatting: %a.<type> expects LogArray
                        if (spec.type == FormatType::Array)
                        {
                                if constexpr (std::is_same_v<std::decay_t<T>, LogArray<float>> ||
                                              std::is_same_v<std::decay_t<T>, LogArray<double>> ||
                                              std::is_same_v<std::decay_t<T>, LogArray<int>> ||
                                              std::is_same_v<std::decay_t<T>,
                                                             LogArray<unsigned int>>)
                                {
                                        formatArray(buffer, pos, bufferSize, value);
                                        format = end;
                                        return;
                                }
                        }

                        formatTyped(buffer, pos, bufferSize, std::forward<T>(value));
                        format = end;
                }

                /** Formats an array as a comma-separated list
                 * @param buffer Output buffer
                 * @param pos Current position in buffer
                 * @param bufferSize Size of output buffer
                 * @param array Array to format */
                template <typename T>
                static void formatArray(char*              buffer,
                                        size_t&            pos,
                                        size_t             bufferSize,
                                        const LogArray<T>& array) noexcept
                {
                        if (array.data == nullptr || pos >= bufferSize)
                                return;

                        if (pos < bufferSize - 1)
                        {
                                buffer[pos++] = '[';
                                buffer[pos]   = '\0';
                        }

                        FormatSpec elementSpec;
                        elementSpec.precision = 6;

                        for (size_t i = 0; i < array.count && pos < bufferSize - 2; i++)
                        {
                                if (i > 0)
                                {
                                        if (pos + 2 >= bufferSize)
                                                break;
                                        buffer[pos++] = ',';
                                        buffer[pos++] = ' ';
                                        buffer[pos]   = '\0';
                                }

                                if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>)
                                {
                                        elementSpec.type = FormatType::Float;
                                        LogFormatTypes::formatFloat(
                                            buffer, pos, bufferSize,
                                            static_cast<double>(array.data[i]), elementSpec);
                                }
                                else if constexpr (std::is_signed_v<T>)
                                {
                                        elementSpec.type     = FormatType::Int;
                                        elementSpec.isSigned = true;
                                        LogFormatTypes::formatint(
                                            buffer, pos, bufferSize,
                                            static_cast<int64_t>(array.data[i]), elementSpec);
                                }
                                else
                                {
                                        elementSpec.type     = FormatType::Int;
                                        elementSpec.isSigned = false;
                                        LogFormatTypes::formatUint(
                                            buffer, pos, bufferSize,
                                            static_cast<uint64_t>(array.data[i]), elementSpec);
                                }
                                if (pos >= bufferSize - 1)
                                        break;
                        }

                        // Closing bracket
                        if (pos < bufferSize - 1)
                        {
                                buffer[pos++] = ']';
                                buffer[pos]   = '\0';
                        }
                }

                /** Formats a typed value based on its type
                 * @param buffer Output buffer
                 * @param pos Current position in buffer
                 * @param bufferSize Size of output buffer
                 * @param value Value to format */
                template <typename T>
                static void
                formatTyped(char* buffer, size_t& pos, size_t bufferSize, T&& value) noexcept
                {
                        FormatSpec spec;
                        spec.type      = FormatType::Int;
                        spec.isSigned  = true;
                        spec.precision = 6;

                        if constexpr (std::is_same_v<std::decay_t<T>, float> ||
                                      std::is_same_v<std::decay_t<T>, double>)
                        {
                                spec.type = FormatType::Float;
                                LogFormatTypes::formatFloat(buffer, pos, bufferSize,
                                                            static_cast<double>(value), spec);
                        }
                        else if constexpr (std::is_integral_v<std::decay_t<T>>)
                        {
                                if constexpr (std::is_signed_v<std::decay_t<T>>)
                                {
                                        spec.isSigned = true;
                                        LogFormatTypes::formatint(buffer, pos, bufferSize,
                                                                  static_cast<int64_t>(value),
                                                                  spec);
                                }
                                else
                                {
                                        spec.isSigned = false;
                                        LogFormatTypes::formatUint(buffer, pos, bufferSize,
                                                                   static_cast<uint64_t>(value),
                                                                   spec);
                                }
                        }
                        else if constexpr (std::is_pointer_v<std::decay_t<T>>)
                        {
                                if constexpr (std::is_same_v<std::decay_t<T>, const char*>)
                                {
                                        spec.type = FormatType::String;
                                        LogFormatTypes::formatString(buffer, pos, bufferSize, value,
                                                                     spec);
                                }
                                else
                                {
                                        spec.type = FormatType::Pointer;
                                        LogFormatTypes::formatPointer(
                                            buffer, pos, bufferSize,
                                            reinterpret_cast<const void*>(value));
                                }
                        }
                        else
                        {
                                // Default to string representation if possible
                                spec.type = FormatType::String;
                                LogFormatTypes::formatString(buffer, pos, bufferSize, "unknown",
                                                             spec);
                        }
                }
};

} // namespace rl

#endif // RL_LOG_LOG_FORMATTER_H
