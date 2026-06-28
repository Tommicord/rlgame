export module Rl.RayLog.IRayLogSerializable;

import <string>;

namespace Rl::RayLog
{

/* Abstract interface for serializable RayLog logging classes */
export template<class T>
class IRayLogSerializable
{
public:
  /* Default destructor for the printer strategy */
  virtual ~IRayLogSerializable() = default;

  /* Must inherit the ToString method (needs a value) */
  virtual constexpr const std::string& ToString(T type) const
  {
    return std::string();
  };

  /* Must inherit the ToString method (no value needed) */
  virtual constexpr const std::string& ToString() const
  {
    return std::string();
  };
};

}