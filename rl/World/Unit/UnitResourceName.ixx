export module Rl.World.Unit.UnitResourceName;

import <vector>;

namespace Rl::World
{

export class UnitResourceName
{
  static constexpr auto BASE = "rl.unit";

  protected:
  /* Identifies the unit resource name, for example: rl.world.UnitGrass */
  char*  name;

  /* Stores the length of the unit resource name */
  size_t nameLen;

  public:
  /* Creates a basic unit resource name for registry identifiers */
  explicit UnitResourceName(const std::vector<const char*>& name) noexcept;

  /* Destroys a basic resource name object */
  ~UnitResourceName();

  /* Constructs the resource name from base identifier */
  void ConstructResourceName(const std::vector<const char*>& base, size_t maxSize) noexcept;

  /* Splits the resource name into smaller tokens */
  [[nodiscard]]
  std::vector<char*> SplitResourceName() const;

  /* Gets the base string resource name */
  static const char* GetBaseResourceString();

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
