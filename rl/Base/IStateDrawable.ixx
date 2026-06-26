export module Rl.Base.IDrawable;

import Rl.Base.Binding;

namespace Rl::Providers
{

export struct IStateResource
{
  virtual ~IStateResource() = default;
};

export struct IStateDrawableBinding
{
  virtual ~IStateDrawableBinding() = default;
};

export template <class T = void, class U = void>
class IStateDrawable
{
  public:
  using ContextValue                                                         = Game::MainBinding;
  using ResValue                                                             = T;
  using BindingValue                                                         = U;
  IStateDrawable()                                                           = default;
  IStateDrawable(const IStateDrawable& other)                                = delete;
  IStateDrawable& operator=(const IStateDrawable& other)                     = delete;
  virtual ~IStateDrawable()                                                  = default;
  virtual void OnDraw(T& resource, U& vk, Game::MainBinding& context)        = 0;
  virtual void OnDrawCompute(T& resource, U& vk, Game::MainBinding& context) = 0;
  virtual void OnUpdate(T& resource, U& vk, ContextValue& context)           = 0;
  virtual void OnCreate(T& resources, U& vk, ContextValue& context)          = 0;
  virtual void OnDestroy(T& resources, U& vk, ContextValue& context)         = 0;
  virtual void OnPause()                                                     = 0;
  virtual void OnResume()                                                    = 0;
};

} // namespace Rl::Providers
