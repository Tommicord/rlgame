export module Rl.Base.IModel;

import Rl.Base.Binding;
import Rl.Base.IUpdatable;
import Rl.Base.IDrawable;

import <type_traits>;

namespace Rl::Providers
{

/* Validate in compile time if the class is an updatable */
export template <typename T>
concept IsUpdatable = std::is_base_of_v<IUpdatable, T>;

/* Validate in compile time if the class is derived from templated base class */
export template <class Derived, template <typename...> class Base>
concept IsDerivedTemplate =
    requires(Derived& derived) { []<class... Ts>(Base<Ts...>&) {}(derived); };

/* Acts like the controller or manager of Resources, Drawable, and Bindings (Vulkan resources) */
export template <class T, class D, class R, class B>
  requires(IsUpdatable<T> && IsDerivedTemplate<D, IStateDrawable>)
class IStateModel
{
  public:
  /* Initializer of a state model */
  explicit IStateModel(Main::MainBinding& context)
  {
  }

  /* Default destructor for child classes */
  virtual ~IStateModel() = default;

  /* Gets the stored object */
  virtual T& GetObjectRef() const = 0;

  /* Gets the stored camera */
  virtual R& GetResource() const = 0;

  /* Gets the stored camera */
  virtual D& GetDrawable() const = 0;

  /* Gets the stored camera */
  virtual B& GetBinding() const = 0;

  /* Draws the drawable using the vulkan context */
  void Draw(Main::MainBinding& context)
  {
    static_cast<D&>(GetDrawable()).OnDraw(GetResource(), GetBinding(), context);
  }

  /* Dispatches compute shaders using the vulkan context */
  void DrawCompute(Main::MainBinding& context)
  {
    static_cast<D&>(GetDrawable()).OnDrawCompute(GetResource(), GetBinding(), context);
  }

  /* Updates the drawable state */
  void Update(Main::MainBinding& context)
  {
    static_cast<IUpdatable&>(GetObjectRef()).Update();
    static_cast<D&>(GetDrawable()).OnUpdate(GetResource(), GetBinding(), context);
  }
};

} // namespace Rl::Providers
