#pragma once
#include "rl/World/Unit/Unit.h"

namespace Rl::World {

class IUnitGrowable
{
public:
    /* Destructs a IUnitGrowable object */
    virtual ~IUnitGrowable() = default;

    /* Returns if the Grass unit can grow */
    virtual bool InGrowState() = 0;

    /* Returns if the Grass unit can grow */
    virtual void Grow() = 0;
};

class UnitGrass : public AbstractUnit, public IUnitGrowable
{
public:
    UnitGrass() noexcept;
protected:
    bool InGrowState() override;
    void Grow() override;
};

}