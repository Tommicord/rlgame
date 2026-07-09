export module Rl.RayLog.ColorPrinter;

import Rl.RayLog.ISerializable;

import <string>;

namespace Rl::RayLog
{

/* Represents a color for a log message */
export enum class RayLogColor : unsigned char {
  Red,
  Blue,
  Cyan,
  BoldRed,
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
  constexpr std::string ToString(const RayLogColor level) const override
  {
    switch (level)
    {
    case RayLogColor::Red:
      return "\033[0;31m";
    case RayLogColor::Blue:
      return "\033[0;33m";
    case RayLogColor::Cyan:
      return "\033[0;34m";
    case RayLogColor::BoldRed:
      return "\033[1;35m";
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
