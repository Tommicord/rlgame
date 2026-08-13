#include "Rl.Log/LogFormatter.h"

namespace rl
{

int LogFormatter::format(char* buffer, size_t bufferSize, const char* format) noexcept
{
        if (buffer == nullptr || format == nullptr || bufferSize == 0)
                return -1;

        size_t      pos = 0;
        const char* ptr = format;

        while (*ptr != '\0' && pos < bufferSize - 1)
        {
                if (*ptr == '%')
                {
                        // No more arguments, just copy the %
                        if (pos < bufferSize - 1)
                        {
                                buffer[pos++] = '%';
                                buffer[pos]   = '\0';
                        }
                        ptr++;
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

} // namespace rl
