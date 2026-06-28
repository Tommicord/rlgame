export module Rl.RayLog.Logger;

import Rl.RayLog.Config;
import Rl.RayLog.LogLevel;
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
import <string>;
import <sstream>;
import <mutex>;
import <variant>;

namespace Rl::RayLog
{

export class RayLog
{
  RayLogRingBuffer<RayLogMessage> messageQueue{MaxQueueSize};
  RayLogThreadPool threadPool{WorkerThreads};
  RayLogFileOutput fileOutput;
  RayLogLevelPrinter levelPrinter;
  std::mutex logMutex;
  static RayLog* instance;

  RayLog() = default;

  public:
  RayLog(const RayLog&) = delete;
  RayLog& operator=(const RayLog&) = delete;

  static RayLog& GetInstance()
  {
    static RayLog instance;
    return instance;
  }

  void Log(RayLogLevel level, const std::string& tag, const std::string& format, 
           const std::vector<std::variant<int, float, std::string, bool, void*, 
           std::vector<int>, std::vector<float>, std::vector<std::string>>>& args)
  {
    std::string formatted = FormatMessage(format, args);
    
    RayLogMessage message(level, formatted, tag);
    
    if (!messageQueue.Push(message))
    {
      FlushQueue();
      messageQueue.Push(message);
    }

    if (level == RayLogLevel::Fatal)
    {
      FlushQueue();
      RayLogFatalHandler::Handle(formatted);
    }

    if (ImmediateFlush)
      FlushQueue();
  }

  void FlushQueue()
  {
    RayLogMessage message;
    while (messageQueue.Pop(message))
    {
      threadPool.Enqueue([this, message]
      {
        std::string logLine = BuildLogLine(message);
        fileOutput.Write(logLine);
        RayLogPlatformOutput::Write(logLine);
      });
    }
  }

  private:
  std::string BuildLogLine(const RayLogMessage& message)
  {
    std::stringstream ss;
    ss << "[Thread " << RayLogTimestamp::GetThreadId() << "] ";
    ss << RayLogTimestamp::Format() << " ";
    ss << levelPrinter.ToString(message.level) << " ";
    ss << message.tag << ": ";
    ss << message.formattedMessage;
    return ss.str();
  }

  std::string FormatMessage(const std::string& format, 
                            const std::vector<std::variant<int, float, std::string, bool, 
                            void*, std::vector<int>, std::vector<float>, std::vector<std::string>>>& args)
  {
    auto tokens = RayLogFormatParser::Parse(format);
    std::stringstream ss;
    size_t argIndex = 0;

    for (const auto& token : tokens)
    {
      if (!token.isSpecifier)
      {
        ss << token.value;
      }
      else if (argIndex < args.size())
      {
        ss << FormatArgument(token.value, args[argIndex]);
        argIndex++;
      }
    }

    return ss.str();
  }

  std::string FormatArgument(const std::string& specifier, 
                            const std::variant<int, float, std::string, bool, void*,
                            std::vector<int>, std::vector<float>, std::vector<std::string>>& arg)
  {
    if (specifier == "%s")
      return RayLogFormatter::FormatString(std::get<std::string>(arg));
    if (specifier == "%d")
      return RayLogFormatter::FormatInt(std::get<int>(arg));
    if (specifier == "%f")
      return RayLogFormatter::FormatFloat(std::get<float>(arg));
    if (specifier == "%h")
      return RayLogFormatter::FormatHex(std::get<int>(arg));
    if (specifier == "%p")
      return RayLogFormatter::FormatPointer(std::get<void*>(arg));
    if (specifier == "%b")
      return RayLogFormatter::FormatBool(std::get<bool>(arg));
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
      int precision = std::stoi(specifier.substr(3));
      return RayLogFormatter::FormatFloat(std::get<float>(arg), precision);
    }
    return "?";
  }
};

RayLog* RayLog::instance = nullptr;

}
