#pragma once

#include "mdaz/inventory.hpp"
#include "mdaz/item_db.hpp"
#include "mdaz/types.hpp"

namespace mdaz {

// Craft recipes — ingredient IDs from l_eng_items.xml.
// Recipe list is incomplete until full Craft_* event groups are ported.
struct CraftRecipe {
  const char* name;       // matches C2 function-like id when known
  ItemId result_id;
  int result_count;
  ItemId ing_a;
  int count_a;
  ItemId ing_b;
  int count_b;
  bool requires_fire;     // cook recipes need nearby campfire
};

int craft_recipe_count();
const CraftRecipe* craft_recipe_at(int i);

// Returns true if inventory has ingredients (does not consume).
bool craft_can(const Inventory& inv, const CraftRecipe& r);
bool craft_do(Inventory& inv, const CraftRecipe& r, const ItemDatabase& db);

} // namespace mdaz
