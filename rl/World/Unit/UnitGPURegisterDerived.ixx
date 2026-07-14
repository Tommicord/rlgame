export module Rl.World.Unit.UnitGPURegisterDerived;

import Rl.World.Unit.UnitRegistryGPU;

namespace Rl::World
{

/* Lightweight helper to decouple unit registration from the game layer */
export class UnitGPURegisterDerived
{
public:
    explicit UnitGPURegisterDerived(UnitRegistryGPU& registry) noexcept :
        registry(&registry)
    {
    }

    template <typename Derived> void Register(Derived* unit) const
    {
        if (registry != nullptr)
        {
            registry->Register(unit);
        }
    }

private:
    UnitRegistryGPU* registry;
};

} // namespace Rl::World
