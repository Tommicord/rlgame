#include "Rl.Log/LogFormatArray.h"
#include "Rl.Log/LogFormatTypes.h"

#include <cstdio>
#include <cstring>
#include <type_traits>

namespace rl
{

template <typename T>
void LogArrayFormatter::formatTypedArray(char*             buffer,
                                         size_t&           pos,
                                         size_t            bufferSize,
                                         const T*          data,
                                         size_t            count,
                                         const FormatSpec& spec) noexcept
{
  FormatSpec elementSpec = spec;

  if constexpr (std::is_same_v<T, float>)
  {
    elementSpec.type = FormatType::Float;
  }
  else if constexpr (std::is_same_v<T, double>)
  {
    elementSpec.type = FormatType::Float;
  }
  else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, int>)
  {
    elementSpec.type     = FormatType::Int;
    elementSpec.isSigned = true;
  }
  else if constexpr (std::is_same_v<T, uint32_t> || std::is_same_v<T, unsigned int>)
  {
    elementSpec.type     = FormatType::Int;
    elementSpec.isSigned = false;
  }
  else if constexpr (std::is_same_v<T, int64_t>)
  {
    elementSpec.type     = FormatType::LongLong;
    elementSpec.isSigned = true;
  }
  else if constexpr (std::is_same_v<T, uint64_t>)
  {
    elementSpec.type     = FormatType::LongLong;
    elementSpec.isSigned = false;
  }
  else if constexpr (std::is_same_v<T, int16_t>)
  {
    elementSpec.type     = FormatType::Short;
    elementSpec.isSigned = true;
  }
  else if constexpr (std::is_same_v<T, uint16_t>)
  {
    elementSpec.type     = FormatType::Short;
    elementSpec.isSigned = false;
  }
  else if constexpr (std::is_same_v<T, int8_t>)
  {
    elementSpec.type     = FormatType::Char;
    elementSpec.isSigned = true;
  }
  else if constexpr (std::is_same_v<T, uint8_t>)
  {
    elementSpec.type     = FormatType::Char;
    elementSpec.isSigned = false;
  }
  else
  {
    elementSpec.type     = FormatType::Int;
    elementSpec.isSigned = true;
  }

  for (size_t i = 0; i < count && pos < bufferSize - 2; i++)
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
      LogFormatTypes::formatFloat(buffer, pos, bufferSize, static_cast<double>(data[i]),
                                  elementSpec);
    }
    else if constexpr (std::is_signed_v<T>)
    {
      LogFormatTypes::formatint(buffer, pos, bufferSize, static_cast<int64_t>(data[i]),
                                elementSpec);
    }
    else
    {
      LogFormatTypes::formatUint(buffer, pos, bufferSize, static_cast<uint64_t>(data[i]),
                                 elementSpec);
    }

    // Safety check to prevent buffer overflow
    if (pos >= bufferSize - 1)
      break;
  }
}

void LogArrayFormatter::formatArray(char*             buffer,
                                    size_t&           pos,
                                    size_t            bufferSize,
                                    const void*       data,
                                    size_t            count,
                                    const FormatSpec& spec) noexcept
{
  if (data == nullptr || pos >= bufferSize)
    return;
  if (pos < bufferSize - 1)
  {
    buffer[pos++] = '[';
    buffer[pos]   = '\0';
  }

  // Use template-based type deduction based on subType
  switch (spec.subType)
  {
  case 'd':
    formatTypedArray(buffer, pos, bufferSize, static_cast<const int32_t*>(data), count, spec);
    break;
  case 'u':
    formatTypedArray(buffer, pos, bufferSize, static_cast<const uint32_t*>(data), count, spec);
    break;
  case 'f':
    formatTypedArray(buffer, pos, bufferSize, static_cast<const float*>(data), count, spec);
    break;
  case 's':
    {
      auto*      strArray    = static_cast<const char* const*>(data);
      FormatSpec elementSpec = spec;
      elementSpec.type       = FormatType::String;
      for (size_t i = 0; i < count && pos < bufferSize - 2; i++)
      {
        if (i > 0)
        {
          buffer[pos++] = ',';
          buffer[pos++] = ' ';
          buffer[pos]   = '\0';
        }
        auto* str = strArray[i];
        if (str != nullptr)
          LogFormatTypes::formatString(buffer, pos, bufferSize, str, elementSpec);
        else
          LogFormatTypes::formatString(buffer, pos, bufferSize, "null", elementSpec);
      }
    }
    break;
  default:
    formatTypedArray(buffer, pos, bufferSize, static_cast<const int32_t*>(data), count, spec);
    break;
  }

  // Closing bracket
  if (pos < bufferSize - 1)
  {
    buffer[pos++] = ']';
    buffer[pos]   = '\0';
  }
}

} // namespace rl
