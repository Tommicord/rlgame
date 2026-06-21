#pragma once

#include "rl/Base/IUpdatable.h"
#include "rl/Base/StateDrawable.h"

// Forward reference
class VulkanContext;

namespace Rl::Providers
{

template <class T, class D, class R, class DV>
class StateModel
{
  public:
  /* Initializer of a state model */
  explicit StateModel(Game::VulkanContext& context)
  {
  }

  /* Default destructor for child classes */
  virtual ~StateModel() = default;

  /* Gets the stored object */
  virtual T& GetObject() = 0;

  /* Gets the stored camera */
  virtual R& GetResource() = 0;

  /* Gets the stored camera */
  virtual D& GetDrawable() = 0;

  /* Gets the stored camera */
  virtual DV& GetVulkanState() = 0;

  /* Draws the drawable using the vulkan context */
  void DrawFromStateModel(Game::VulkanContext& context)
  {
    GetDrawable().OnDraw(GetResource(), GetVulkanState(), context);
  }

  /* Dispatches compute shader using the vulkan context */
  void DrawComputeFromStateModel(Game::VulkanContext& context)
  {
    GetDrawable().OnDrawCompute(GetResource(), GetVulkanState(), context);
  }

  /* Updates the drawable state */
  void UpdateFromStateModel(Game::VulkanContext& context)
  {
    GetObject().Update();
    GetDrawable().OnUpdate(GetResource(), GetVulkanState(), context);
  }
};

} // namespace Rl::Providers
