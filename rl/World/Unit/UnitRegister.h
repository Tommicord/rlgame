#pragma once

#include <string>
#include <typeinfo>

namespace Rl::World
{

/* Defines an abstract class for Unit register */
class UnitRegister
{
  public:
  /* Constructs a basic unit register */
  virtual ~UnitRegister() = default;

  /* Gets the Unit class id */
  [[nodiscard]]
  virtual consteval int GetClassId() const = 0;
};

/* Defines an interface for Unit identification */
template <typename T>
class IUnitIdentifiable : public UnitRegister
{
  public:
  using Id = unsigned int;

  /* Compile-Time Static method, Gens a hash for the Unit class */
  static consteval unsigned int GenClassId()
  {
    constexpr unsigned int factor   = 6;
    constexpr unsigned int complete = 65534;
    unsigned int           hash     = 0x0000;
    std::string_view       name     = typeid(T).name();

    for (const char c : name)
    {
      hash ^= static_cast<unsigned int>(c);
      hash &= complete;
      hash >>= factor;
    }

    hash ^= hash >> 7;
    hash *= 0x89ea6bfa;
    hash ^= hash >> 12;
    hash *= 0xc252ae35;
    hash ^= (hash >> 15);
    return (hash + 1) & 0xFFFFu;
  }

  /* Compile-Evaluated static function that gets the id for the Unit */
  [[nodiscard]]
  static consteval int GetStaticUnitId()
  {
    const int id = GenClassId();
    return id;
  }

  /* Compile-Evaluated function that gets the id of the Unit  */
  [[nodiscard]]
  consteval int GetClassId() const override
  {
    return GetStaticUnitId();
  }
};

} // namespace Rl::World
