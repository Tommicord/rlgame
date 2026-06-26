export module Rl.World.Unit.UnitRegister;

import <string>;
import <string_view>;
import <source_location>;

namespace Rl::World
{

/* Defines an abstract class for Unit register */
export class UnitRegister
{
  public:
  /* Constructs a basic unit register */
  virtual ~UnitRegister() = default;

  /* Gets the Unit class id */
  [[nodiscard]]
  virtual consteval unsigned short GetClassId() const = 0;
};

/* Defines an interface for Unit identification */
export template <typename T> class IUnitIdentifiable : public UnitRegister
{
  public:
  using Id = unsigned int;

  /* Compile-Time Static method, Gens a hash for the Unit class */
  static consteval unsigned short GenClassId()
  {
    constexpr unsigned int     factor = 6;
    constexpr unsigned int     complete = 65535;
    unsigned int               hash = 0x0000;
    constexpr std::string_view name = SimpleClassName();

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
  static consteval unsigned short GetStaticClassId()
  {
    const int id = GenClassId();
    return id;
  }

  /* Compile-Evaluated function that gets the id of the Unit  */
  [[nodiscard]]
  consteval unsigned short GetClassId() const override
  {
    return GetStaticClassId();
  }

  [[nodiscard]]
  static consteval std::string_view SimpleClassName()
  {
#if defined(__clang__)
    constexpr std::string_view name{__PRETTY_FUNCTION__};
    constexpr std::string_view prefix = "std::string_view SimpleClassName() [T = ";
    constexpr std::string_view suffix = "]";
#elif defined(__GNUC__)
    constexpr std::string_view name{__PRETTY_FUNCTION__};
    constexpr std::string_view prefix = "constexpr std::string_view SimpleClassName() [with T = ";
    constexpr std::string_view suffix = "]";
#elif defined(_MSC_VER)
    constexpr std::string_view name{__FUNCSIG__};
    constexpr std::string_view prefix = "class std::basic_string_view<char, "
                                        "std::char_traits<char> > __cdecl SimpleClassName<";
    constexpr std::string_view suffix = ">(void)";
#else
    return "UnitError";
#endif
    auto start = name.find(prefix);
    if (start == std::string_view::npos)
      return name;
    auto currentStart = start + prefix.length();
    auto end = name.find(suffix, currentStart);
    if (end == std::string_view::npos)
      return name;
    std::string_view typeName = name.substr(currentStart, end - currentStart);
#if defined(_MSC_VER)
    if (typeName.starts_with("class "))
    {
      typeName.remove_prefix(6);
    }
    else if (typeName.starts_with("struct "))
    {
      typeName.remove_prefix(7);
    }
#endif
    auto last = typeName.rfind("::");
    if (last != std::string_view::npos)
    {
      typeName.remove_prefix(last + 2);
    }
    return typeName;
  }
};

} // namespace Rl::World
