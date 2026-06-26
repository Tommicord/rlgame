export module Rl.World.Unit.UnitResourceName;

import <vector>;
import <string>;
import <string_view>;

namespace Rl::World
{

export class UnitResourceName
{
  protected:
  /* Identifies the unit resource name, for example: rl.world.UnitGrass */
  char*  name;

  /* Stores the length of the unit resource name */
  size_t nameLen;

  public:
  /* The prefix of the resource name */
  static constexpr auto prefix = "rl.unit";

  /* Creates a basic unit resource name for registry identifiers */
  explicit UnitResourceName(const std::vector<std::string_view>& name) noexcept;

  /* Destroys a basic resource name object */
  ~UnitResourceName();

  /* Constructs the resource name from base identifier */
  void ConstructResourceName(const std::vector<std::string_view>& base, size_t maxSize) noexcept;

  /* Splits the resource name into smaller tokens */
  [[nodiscard]]
  std::vector<char*> SplitResourceName() const;

  /* Gets the stored resource name */
  [[nodiscard]]
  char* GetResourceName() const;

  /* Gets the stored resource name length */
  [[nodiscard]]
  size_t GetResourceNameLength() const;

  /* Compares the resource name with other resource name */
  [[nodiscard]]
  bool Equals(const UnitResourceName& resource) const;
};

} // namespace Rl::World
