#include "Rl.Log/LogFormatSpec.h"
#include <cstring>

namespace rl
{

const char* parseFormatSpec(const char* format, FormatSpec& spec) noexcept
{
        if (format == nullptr || *format != '%')
                return format;

        const char* ptr = format + 1;
        spec.type       = FormatType::Int;
        spec.precision  = 6;
        spec.isSigned   = true;
        spec.subType    = 0;
        while (*ptr != '\0')
        {
                if (*ptr == 'h' && *(ptr + 1) == 'h')
                {
                        spec.type = FormatType::Char;
                        ptr += 2;
                }
                else if (*ptr == 'h')
                {
                        spec.type = FormatType::Short;
                        ptr++;
                }
                else if (*ptr == 'l' && *(ptr + 1) == 'l')
                {
                        spec.type = FormatType::LongLong;
                        ptr += 2;
                }
                else if (*ptr == 'l')
                {
                        spec.type = FormatType::Long;
                        ptr++;
                }
                else if (*ptr == 'j')
                {
                        spec.type = FormatType::intMax;
                        ptr++;
                }
                else if (*ptr == 'z')
                {
                        spec.type = FormatType::Size;
                        ptr++;
                }
                else if (*ptr == 't')
                {
                        spec.type = FormatType::PtrDiff;
                        ptr++;
                }
                else if (*ptr == 'L')
                {
                        spec.type = FormatType::Double;
                        ptr++;
                }
                else if (*ptr == '.')
                {
                        // Parse precision for floats
                        ptr++;
                        spec.precision = 0;
                        while (*ptr >= '0' && *ptr <= '9')
                        {
                                spec.precision = spec.precision * 10 + (*ptr - '0');
                                ptr++;
                        }
                }
                else if (*ptr == 'a' && *(ptr + 1) == '.')
                {
                        // Array formatting
                        ptr += 2;
                        spec.type    = FormatType::Array;
                        spec.subType = *ptr;
                        if (spec.subType != '\0')
                                ptr++;
                        break;
                }
                else if (*ptr == 'd' || *ptr == 'i')
                {
                        spec.isSigned = true;
                        ptr++;
                        break;
                }
                else if (*ptr == 'u')
                {
                        spec.isSigned = false;
                        ptr++;
                        break;
                }
                else if (*ptr == 'f' || *ptr == 'F')
                {
                        spec.type = FormatType::Float;
                        ptr++;
                        break;
                }
                else if (*ptr == 's')
                {
                        spec.type = FormatType::String;
                        ptr++;
                        break;
                }
                else if (*ptr == 'p')
                {
                        spec.type = FormatType::Pointer;
                        ptr++;
                        break;
                }
                else if (*ptr == 'c')
                {
                        spec.type = FormatType::Char;
                        ptr++;
                        break;
                }
                else if (*ptr == 'x' || *ptr == 'X')
                {
                        spec.isSigned = false;
                        ptr++;
                        break;
                }
                else if (*ptr == 'o')
                {
                        spec.isSigned = false;
                        ptr++;
                        break;
                }
                else
                {
                        ptr++;
                }
        }

        return ptr;
}

} // namespace rl
