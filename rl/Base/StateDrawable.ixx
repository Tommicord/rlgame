export module Rl.Base.StateDrawable;

namespace Rl::Providers
{

export class StateDrawable
{
  public:
  virtual ~StateDrawable() = default;
  virtual void Draw() = 0;
};

} // namespace Rl::Providers
