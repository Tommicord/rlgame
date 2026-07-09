export module Rl.RayLog.Formatter;

import <string>;
import <sstream>;
import <iomanip>;
import <vector>;
import <variant>;

namespace Rl::RayLog
{

export class RayLogFormatter
{
  public:
  [[nodiscard]]
  static std::string FormatInt(const int value)
  { return std::to_string(value); }

  [[nodiscard]]
  static std::string FormatFloat(const float value, const int precision = 4)
  {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    return ss.str();
  }

  [[nodiscard]]
  static std::string FormatDouble(const double value, const int precision = 4)
  {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    return ss.str();
  }

  [[nodiscard]]
  static std::string FormatString(const std::string& value)
  { return value; }

  [[nodiscard]]
  static std::string FormatHex(const int value)
  {
    std::stringstream ss;
    ss << "0x" << std::hex << value;
    return ss.str();
  }

  [[nodiscard]]
  static std::string FormatPtr(const void* ptr)
  {
    std::stringstream ss;
    ss << "0x" << std::hex << reinterpret_cast<uintptr_t>(ptr);
    return ss.str();
  }

  [[nodiscard]]
  static std::string FormatBool(const bool value)
  { return value ? "true" : "false"; }

  [[nodiscard]]
  static std::string FormatArray(const std::vector<int>& arr)
  {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < arr.size(); ++i)
    {
      ss << arr[i];
      if (i < arr.size() - 1)
        ss << ", ";
    }
    ss << "]";
    return ss.str();
  }

  [[nodiscard]]
  static std::string FormatArray(const std::vector<float>& arr)
  {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < arr.size(); ++i)
    {
      ss << arr[i];
      if (i < arr.size() - 1)
        ss << ", ";
    }
    ss << "]";
    return ss.str();
  }

  [[nodiscard]]
  static std::string FormatArray(const std::vector<std::string>& arr)
  {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < arr.size(); ++i)
    {
      ss << "\"" << arr[i] << "\"";
      if (i < arr.size() - 1)
        ss << ", ";
    }
    ss << "]";
    return ss.str();
  }
};

// Template-based formatting for compile-time type detection
export namespace Rl::RayLog::Detail
{
template <typename T> std::string FormatArg(const T& arg)
{
  std::stringstream ss;
  ss << arg;
  return ss.str();
}

template <> std::string FormatArg<std::string>(const std::string& arg)
{ return arg; }

template <> std::string FormatArg<std::vector<int>>(const std::vector<int>& arg)
{ return RayLogFormatter::FormatArray(arg); }

template <> std::string FormatArg<std::vector<float>>(const std::vector<float>& arg)
{ return RayLogFormatter::FormatArray(arg); }

template <>
std::string FormatArg<std::vector<std::string>>(const std::vector<std::string>& arg)
{ return RayLogFormatter::FormatArray(arg); }

template <> std::string FormatArg<bool>(const bool& arg)
{ return RayLogFormatter::FormatBool(arg); }

template <> std::string FormatArg<void*>(void* const& arg)
{ return RayLogFormatter::FormatPtr(arg); }

template <> std::string FormatArg<const void*>(const void* const& arg)
{ return RayLogFormatter::FormatPtr(arg); }

template <typename... Ts>
std::string FormatMessage(const std::string& format, const Ts&... args)
{
  std::vector<std::string> formatted_args = {FormatArg(args)...};
  std::stringstream        ss;
  size_t                   arg_index = 0;

  for (size_t i = 0; i < format.size(); ++i)
  {
    if (format[i] == '%' && i + 1 < format.size() && arg_index < formatted_args.size())
    {
      char spec = format[i + 1];
      if (spec == 's' || spec == 'd' || spec == 'f' || spec == 'h' || spec == 'p' ||
          spec == 'b' || spec == 'a')
      {
        ss << formatted_args[arg_index++];
        i++;
      }
      else if (spec == '%')
      {
        ss << '%';
        i++;
      }
      else
      {
        ss << format[i];
      }
    }
    else
    {
      ss << format[i];
    }
  }

  return ss.str();
}
} // namespace Rl::RayLog::Detail

} // namespace Rl::RayLog
