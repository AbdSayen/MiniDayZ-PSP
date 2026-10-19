#pragma once

#include "mdaz/types.hpp"

#include <string>
#include <vector>

namespace mdaz {

enum ItemCategory {
  ItemCat_Unknown = 0,
  ItemCat_Food,
  ItemCat_Drink,
  ItemCat_Medical,
  ItemCat_Ammo,
  ItemCat_Weapon,
  ItemCat_Melee,
  ItemCat_Clothing,
  ItemCat_Backpack,
  ItemCat_Tool,
  ItemCat_Misc
};

struct ItemDef {
  ItemId id;
  std::string name;
  ItemCategory category;
  // Numeric gameplay fields — filled when extracted from C2; 0 = unknown/not extracted.
  float food_restore;
  float water_restore;
  float heal_amount;
  int stack_max;
  bool known_stats;

  ItemDef();
};

class ItemDatabase {
public:
  bool load_from_json_file(const char* path);
  bool load_from_json_string(const char* json);

  const ItemDef* find(ItemId id) const;
  std::size_t count() const { return items_.size(); }
  const std::vector<ItemDef>& all() const { return items_; }

private:
  std::vector<ItemDef> items_;
  void classify(ItemDef& d);
};

} // namespace mdaz
