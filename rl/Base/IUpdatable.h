#pragma once

namespace Rl::Providers
{

class IUpdatable
{
  public:
  virtual ~IUpdatable() = default;
  virtual void Update() = 0;
};

} // namespace Rl::Providers
