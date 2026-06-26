export module Rl.Base.IModel;

import Rl.Base.Binding;
import Rl.Base.IUpdatable;
import Rl.Base.IDrawable;

import <type_traits>;

namespace Rl::Providers
{

export template<typename T>
inline constexpr bool IsUpdatable = std::is_base_of_v<IUpdatable, T>;

export template <class T, class D, class R, class DV>
  requires IsUpdatable<T>
class IStateModel
{
public:
  /* Initializer of a state model */
  explicit IStateModel(Game::MainBinding& context)
  {
  }

  /* Default destructor for child classes */
  virtual ~IStateModel() = default;

  /* Gets the stored object */
  virtual T& GetObject() const = 0;

  /* Gets the stored camera */
  virtual R& GetResource() const = 0;

  /* Gets the stored camera */
  virtual D& GetDrawable() const = 0;

  /* Gets the stored camera */
  virtual DV& GetBinding() const = 0;

  /* Draws the drawable using the vulkan context */
  void Draw(Game::MainBinding& context)
  {
    GetDrawable().OnDraw(GetResource(), GetBinding(), context);
  }

  /* Dispatches compute shaders using the vulkan context */
  void DrawCompute(Game::MainBinding& context)
  {
    static_cast<IStateDrawable>(
      GetDrawable()
    ).OnDrawCompute(
      GetResource(), GetBinding(), context
    );
  }

  /* Updates the drawable state */
  void Update(Game::MainBinding& context)
  {
    static_cast<IUpdatable>(
      GetObject()
    ).Update();
    static_cast<IStateDrawable>(
      GetDrawable()
    ).OnUpdate(
      GetResource(), GetBinding(), context
    );
  }
};
} // namespace Rl::Providers
