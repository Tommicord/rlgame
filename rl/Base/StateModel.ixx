export module Rl.Base.StateModel;

namespace Rl::Providers
{

export class StateModel
{
  public:
  virtual ~StateModel() = default;
  virtual void Update() = 0;
};

} // namespace Rl::Providers
