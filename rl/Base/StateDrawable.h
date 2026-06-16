#pragma once

namespace Rl::Providers {

struct StateResource
{
    virtual ~StateResource() {}
};

template<typename T = void>
class StateDrawable
{
public:
    using value_type = T;
    StateDrawable() = default;
    StateDrawable(const StateDrawable& other) = delete;
    StateDrawable& operator=(const StateDrawable& other) = delete;
    virtual ~StateDrawable() = default;
    virtual void OnDraw() = 0;
    virtual void OnUpdate() = 0;
    virtual void OnCreate(value_type resources) = 0;
    virtual void OnDestroy(value_type resources) = 0;
    virtual void OnPause() = 0;
    virtual void OnResume() = 0;
};



} // namespace Rl::Providers
