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
  static std::string FormatInt(int value)
  {
    return std::to_string(value);
  }

  [[nodiscard]]
  static std::string FormatFloat(float value, int precision = 6)
  {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    return ss.str();
  }

  [[nodiscard]]
  static std::string FormatString(const std::string& value)
  {
    return value;
  }

  [[nodiscard]]
  static std::string FormatHex(int value)
  {
    std::stringstream ss;
    ss << "0x" << std::hex << value;
    return ss.str();
  }

  [[nodiscard]]
  static std::string FormatPointer(const void* ptr)
  {
    std::stringstream ss;
    ss << "0x" << std::hex << reinterpret_cast<uintptr_t>(ptr);
    return ss.str();
  }

  [[nodiscard]]
  static std::string FormatBool(bool value)
  {
    return value ? "true" : "false";
  }

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

}
