export module Rl.RayLog.Macro;

import Rl.RayLog.Logger;
import Rl.RayLog.LevelPrinter;
import Rl.RayLog.Formatter;

import <string>;

namespace Rl::RayLog
{

export template <typename... Ts>
inline void LogTrace(const std::string& tag, const std::string& format, const Ts&... args)
{
  std::string formatted = Rl::RayLog::Detail::FormatMessage(format, args...);
  RayLog::GetInstance().Log(RayLogLevel::Trace, tag, formatted, {});
}

export template <typename... Ts>
inline void LogDebug(const std::string& tag, const std::string& format, const Ts&... args)
{
  std::string formatted = Rl::RayLog::Detail::FormatMessage(format, args...);
  RayLog::GetInstance().Log(RayLogLevel::Debug, tag, formatted, {});
}

export template <typename... Ts>
inline void LogInfo(const std::string& tag, const std::string& format, const Ts&... args)
{
  std::string formatted = Rl::RayLog::Detail::FormatMessage(format, args...);
  RayLog::GetInstance().Log(RayLogLevel::Info, tag, formatted, {});
}

export template <typename... Ts>
inline void LogWarning(
    const std::string& tag, const std::string& format, const Ts&... args)
{
  std::string formatted = Rl::RayLog::Detail::FormatMessage(format, args...);
  RayLog::GetInstance().Log(RayLogLevel::Warning, tag, formatted, {});
}

export template <typename... Ts>
inline void LogError(const std::string& tag, const std::string& format, const Ts&... args)
{
  std::string formatted = Rl::RayLog::Detail::FormatMessage(format, args...);
  RayLog::GetInstance().Log(RayLogLevel::Error, tag, formatted, {});
}

export template <typename... Ts>
inline void LogFatal(const std::string& tag, const std::string& format, const Ts&... args)
{
  std::string formatted = Rl::RayLog::Detail::FormatMessage(format, args...);
  RayLog::GetInstance().Log(RayLogLevel::Fatal, tag, formatted, {});
}

export inline void LogFlush()
{ RayLog::GetInstance().FlushQueue(); }

} // namespace Rl::RayLog
