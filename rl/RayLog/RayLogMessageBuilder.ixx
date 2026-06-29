export module Rl.RayLog.MessageBuilder;

import Rl.RayLog.Config;
import Rl.RayLog.RingBuffer;
import Rl.RayLog.ThreadPool;
import Rl.RayLog.Message;
import Rl.RayLog.Timestamp;
import Rl.RayLog.Formatter;
import Rl.RayLog.FormatParser;
import Rl.RayLog.FileOutput;
import Rl.RayLog.PlatformOutput;
import Rl.RayLog.FatalHandler;
import Rl.RayLog.LevelPrinter;
import Rl.RayLog.ColorPrinter;

import <vector>;
import <string>;
import <sstream>;
import <mutex>;
import <variant>;

namespace Rl::RayLog
{
/* The message builder for the RayLog logging */
export class RayLogMessageBuilder
{
protected:
  /* The logging level printer */
  RayLogLevelPrinter levelPrinter;

  /* The logging ANSI color printer */
  RayLogColorPrinter colorPrinter;
public:
  /* Builds a log line that represents a log */
  [[nodiscard]]
  std::string BuildLogLine(const RayLogMessage& message) const
  {
    std::stringstream ss;
    switch (message.level)
    {
    case RayLogLevel::Fatal:
      ss << colorPrinter.ToString(RayLogColor::BoldRed);
    case RayLogLevel::Debug:
      ss << colorPrinter.ToString(RayLogColor::BoldBlue);
    default:
      ss << colorPrinter.ToString(RayLogColor::BoldCyan);
    }
    ss << "[TID " << RayLogTimestamp::GetThreadId() << "] ";
    ss << RayLogTimestamp::Format() << " ";
    ss << levelPrinter.ToString(message.level) << " ";
    ss << message.tag << ": ";
    ss << message.formattedMessage;
    ss << colorPrinter.ToString(RayLogColor::Reset);
    return ss.str();
  }
  /* Formats the spec */
  std::string FormatMessage(const std::string& format,
      const std::vector<std::variant<int,
          float,
          std::string,
          bool,
          void*,
          std::vector<int>,
          std::vector<float>,
          std::vector<std::string>>>&          args) const
  {
    auto              tokens = RayLogFormatParser::Parse(format);
    std::stringstream ss;
    size_t            argIndex = 0;

    for (const auto& [isSpecifier, value] : tokens)
    {
      if (!isSpecifier)
      {
        ss << value;
      }
      else if (argIndex < args.size())
      {
        ss << FormatArgument(value, args[argIndex]);
        argIndex++;
      }
    }

    return ss.str();
  }

  std::string FormatArgument(const std::string& specifier,
      const std::variant<int,
          float,
          std::string,
          bool,
          void*,
          std::vector<int>,
          std::vector<float>,
          std::vector<std::string>>&            arg) const
  {
    if (specifier == "%s")
      if (std::holds_alternative<std::string>(arg))
      {
        return RayLogFormatter::FormatString(std::get<std::string>(arg));
      }
    if (specifier == "%d")
      if (std::holds_alternative<int>(arg))
      {
        return RayLogFormatter::FormatInt(std::get<int>(arg));
      }
    if (specifier == "%f")
      if (std::holds_alternative<float>(arg))
      {
        return RayLogFormatter::FormatFloat(std::get<float>(arg));
      }
    if (specifier == "%h")
      if (std::holds_alternative<int>(arg))
      {
        return RayLogFormatter::FormatHex(std::get<int>(arg));
      }
    if (specifier == "%p")
      if (std::holds_alternative<void*>(arg))
      {
        return RayLogFormatter::FormatPtr(std::get<void*>(arg));
      }
    if (specifier == "%b")
      if (std::holds_alternative<bool>(arg))
      {
        return RayLogFormatter::FormatBool(std::get<bool>(arg));
      }
    if (specifier == "%a")
    {
      if (std::holds_alternative<std::vector<int>>(arg))
        return RayLogFormatter::FormatArray(std::get<std::vector<int>>(arg));
      if (std::holds_alternative<std::vector<float>>(arg))
        return RayLogFormatter::FormatArray(std::get<std::vector<float>>(arg));
      if (std::holds_alternative<std::vector<std::string>>(arg))
        return RayLogFormatter::FormatArray(std::get<std::vector<std::string>>(arg));
    }
    if (specifier.starts_with("%f."))
    {
      const int precision = std::stoi(specifier.substr(3));
      return RayLogFormatter::FormatFloat(std::get<float>(arg), precision);
    }
    return "?";
  }
};

} // namespace Rl::RayLog