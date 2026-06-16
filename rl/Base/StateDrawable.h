#pragma once

#include <memory>

namespace Rl::Providers {

template<typename T>
class StateDrawableResources : public std::enable_shared_from_this<T> {
};

template<typename T = void>
class StateDrawable
{
public:
    using value_type = T;
    explicit StateDrawable(std::shared_ptr<StateDrawableResources<T>> resources)
        : resources(resources) {};
    StateDrawable() = default;
    StateDrawable(const StateDrawable& other) = delete;
    StateDrawable& operator=(const StateDrawable& other) = delete;
    virtual ~StateDrawable() = default;
    virtual void OnDraw() = 0;
    virtual void OnUpdate() = 0;
    virtual void OnCreate(StateDrawableResources<value_type>& resources) = 0;
    virtual void OnDestroy(StateDrawableResources<value_type>& resources) = 0;
    virtual void OnPause() = 0;
    virtual void OnResume() = 0;
private:
    std::shared_ptr<StateDrawableResources<T>> resources;
};



} // namespace Rl::Providers
