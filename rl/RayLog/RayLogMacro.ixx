export module Rl.RayLog.Macro;

import Rl.RayLog.Logger;
import Rl.RayLog.LogLevel;
import <string>;
import <vector>;

namespace Rl::RayLog
{

export inline void LogTrace(const std::string& tag, const std::string& format, 
                            const std::vector<std::variant<int, float, std::string, bool, void*,
                            std::vector<int>, std::vector<float>, std::vector<std::string>>>& args = {})
{
  RayLog::GetInstance().Log(RayLogLevel::Trace, tag, format, args);
}

export inline void LogDebug(const std::string& tag, const std::string& format,
                            const std::vector<std::variant<int, float, std::string, bool, void*,
                            std::vector<int>, std::vector<float>, std::vector<std::string>>>& args = {})
{
  RayLog::GetInstance().Log(RayLogLevel::Debug, tag, format, args);
}

export inline void LogInfo(const std::string& tag, const std::string& format,
                           const std::vector<std::variant<int, float, std::string, bool, void*,
                           std::vector<int>, std::vector<float>, std::vector<std::string>>>& args = {})
{
  RayLog::GetInstance().Log(RayLogLevel::Info, tag, format, args);
}

export inline void LogWarning(const std::string& tag, const std::string& format,
                              const std::vector<std::variant<int, float, std::string, bool, void*,
                              std::vector<int>, std::vector<float>, std::vector<std::string>>>& args = {})
{
  RayLog::GetInstance().Log(RayLogLevel::Warning, tag, format, args);
}

export inline void LogError(const std::string& tag, const std::string& format,
                            const std::vector<std::variant<int, float, std::string, bool, void*,
                            std::vector<int>, std::vector<float>, std::vector<std::string>>>& args = {})
{
  RayLog::GetInstance().Log(RayLogLevel::Error, tag, format, args);
}

export inline void LogFatal(const std::string& tag, const std::string& format,
                            const std::vector<std::variant<int, float, std::string, bool, void*,
                            std::vector<int>, std::vector<float>, std::vector<std::string>>>& args = {})
{
  RayLog::GetInstance().Log(RayLogLevel::Fatal, tag, format, args);
}

export inline void Flush()
{
  RayLog::GetInstance().FlushQueue();
}

}
