#pragma once
#include "rl/World/Unit/Unit.h"

namespace Rl::World {

class IUnitGrowable
{
public:
    /* Destructs a IUnitGrowable object */
    virtual ~IUnitGrowable() = default;

    /* Returns if the Grass unit can grow */
    virtual bool IsGrowState() = 0;

    /* Returns if the Grass unit can grow */
    virtual void Grow() = 0;
};

class UnitGrass : public AbstractUnit, public IUnitGrowable
{
public:
    UnitGrass();
protected:
    bool IsGrowState() override;
    void Grow() override;
};

}