#ifndef RL_LOG_LOG_FORMAT_ARRAY_H
#define RL_LOG_LOG_FORMAT_ARRAY_H

#include <cstddef>
#include "Rl.Log/LogCommon.h"
#include "Rl.Log/LogFormatSpec.h"

namespace rl
{

class LogArrayFormatter
{
        public:
                static void formatArray(char*             buffer,
                                        size_t&           pos,
                                        size_t            bufferSize,
                                        const void*       data,
                                        size_t            count,
                                        const FormatSpec& spec) noexcept;

        private:
                template <typename T>
                static void formatTypedArray(char*             buffer,
                                             size_t&           pos,
                                             size_t            bufferSize,
                                             const T*          data,
                                             size_t            count,
                                             const FormatSpec& spec) noexcept;
};

} // namespace rl

#endif // RL_LOG_LOG_FORMAT_ARRAY_H
