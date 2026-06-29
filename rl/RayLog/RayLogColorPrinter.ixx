export module Rl.RayLog.Color;

import Rl.RayLog.IRayLogSerializable;

import <string>;

namespace Rl::RayLog
{

/* Represents a color for a log message */
enum class RayLogColor : unsigned char
{
  Red,
  Green,
  Blue,
  Cyan,
  BoldRed,
  BoldGreen,
  BoldBlue,
  BoldCyan,
  Reset
};

/* RayLog logging color printer from RayLogColor enum */
export class RayLogColorPrinter final : public IRayLogSerializable<RayLogColor>
{
  public:
  /* Stringify a RayLogColor enum value */
  [[nodiscard]]
  constexpr const std::string& ToString(const RayLogColor level) const override
  {
    switch (level)
    {
    case RayLogColor::Red:
      return "\033[0;31m";
    case RayLogColor::Green:
      return "\033[0;32m";
    case RayLogColor::Blue:
      return "\033[0;33m";
    case RayLogColor::Cyan:
      return "\033[0;34m";
    case RayLogColor::BoldRed:
      return "\033[1;35m";
    case RayLogColor::BoldGreen:
      return "\033[1;32m";
    case RayLogColor::BoldBlue:
      return "\033[1;33m";
    case RayLogColor::BoldCyan:
      return "\033[1;34m";
    case RayLogColor::Reset:
      return "\033[0m";
    default:
      return "\033[0;37m";
    }
  }
};

} // namespace Rl::RayLog
