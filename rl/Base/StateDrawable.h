#pragma once

namespace Rl::Game
{

// Forward reference to Game class
// We not import the Game.h header
// To avoid circular dependencies
class Game;

// The same as VulkanContext
struct VulkanContext;

} // namespace Rl::Game

namespace Rl::Providers
{

struct StateResource
{
  virtual ~StateResource() = default;
};

struct StateDrawableVulkan
{
  virtual ~StateDrawableVulkan() = default;
};

template <class T = void, class U = void>
class StateDrawable
{
  public:
  using Context                                                         = Game::VulkanContext;
  using ResValueType                                                    = T;
  using VkValueType                                                     = U;
  StateDrawable()                                                       = default;
  StateDrawable(const StateDrawable& other)                             = delete;
  StateDrawable& operator=(const StateDrawable& other)                  = delete;
  virtual ~StateDrawable()                                              = default;
  virtual void OnDraw(T& resource, U& vk, Game::VulkanContext& context) = 0;
  virtual void OnDrawCompute(T& resource, U& vk, Game::VulkanContext& context) = 0;
  virtual void OnUpdate(T& resource, U& vk, Context& context)                  = 0;
  virtual void OnCreate(T& resources, U& vk, Context& context)                 = 0;
  virtual void OnDestroy(T& resources, U& vk, Context& context)                = 0;
  virtual void OnPause()                                                       = 0;
  virtual void OnResume()                                                      = 0;
};

} // namespace Rl::Providers
