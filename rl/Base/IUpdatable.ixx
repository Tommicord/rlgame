export module Rl.Base.IUpdatable;

namespace Rl::Providers
{

export class IUpdatable
{
  public:
  virtual ~IUpdatable() = default;
  virtual void Update() = 0;
};

} // namespace Rl::Providers
