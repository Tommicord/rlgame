export module Rl.RayLog.LogLevel;

import Rl.RayLog.IRayLogSerializable;

import <string>;

namespace Rl::RayLog
{

/* RayLogLevel: Trace, Debug, Info, Warning, Error, Fatal */
enum class RayLogLevel : unsigned char
{
  Trace,
  Debug,
  Info,
  Warning,
  Error,
  Fatal
};

/* RayLog logging level printer from RayLogLevel enum */
export class RayLogLevelPrinter final : public IRayLogSerializable<RayLogLevel>
{
  /* Stringify a RayLogLevel enum value */
  constexpr const std::string& ToString(const RayLogLevel level) const override
  {
    switch (level)
    {
    case RayLogLevel::Trace:
      return "TRACE";
    case RayLogLevel::Debug:
      return "DEBUG";
    case RayLogLevel::Info:
      return "INFO";
    case RayLogLevel::Warning:
      return "WARN";
    case RayLogLevel::Error:
      return "ERROR";
    case RayLogLevel::Fatal:
      return "FATAL";
    default:
      return "UNKNOWN";
    }
  }
};

} // namespace Rl::RayLog
