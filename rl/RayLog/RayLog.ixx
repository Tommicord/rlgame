export module Rl.RayLog.Logger;

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
import Rl.RayLog.MessageBuilder;

import <vector>;
import <string>;
import <sstream>;
import <mutex>;
import <variant>;

namespace Rl::RayLog
{

export class RayLog
{
  RayLogRingBuffer<RayLogMessage> messageQueue{MaxQueueSize};
  RayLogMessageBuilder            messageBuilder;
  RayLogThreadPool                threadPool{WorkerThreads};
  RayLogFileOutput                fileOutput;
  mutable std::mutex              logMutex;

  RayLog() = default;

  public:
  RayLog(const RayLog&) = delete;
  RayLog& operator=(const RayLog&) = delete;

  static RayLog& GetInstance()
  {
    static RayLog instance;
    return instance;
  }

  void Log(const RayLogLevel          level,
      const std::string&              tag,
      const std::string&              format,
      const std::vector<std::variant<int,
          float,
          std::string,
          bool,
          void*,
          std::vector<int>,
          std::vector<float>,
          std::vector<std::string>>>& args)
  {
    const std::string   formatted = messageBuilder.FormatMessage(format, args);
    const RayLogMessage message(level, formatted, tag);
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
      threadPool.Enqueue(
          [this, message]
          {
            const std::string logLine = messageBuilder.BuildLogLine(message);
            fileOutput.Write(logLine);
            RayLogPlatformOutput::Write(logLine);
          });
    }
  }
};

} // namespace Rl::RayLog
