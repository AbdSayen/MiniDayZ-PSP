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
  ItemId tool_id;         // required in inventory but not consumed (0=none); any of 42/43/44 if tool_any_knife
  bool tool_any_knife;    // if true, any hunter/army/butcher knife counts as tool
};

int craft_recipe_count();
const CraftRecipe* craft_recipe_at(int i);

// Returns true if inventory has ingredients (does not consume).
bool craft_can(const Inventory& inv, const CraftRecipe& r);
bool craft_do(Inventory& inv, const CraftRecipe& r, const ItemDatabase& db);

// C2 Check_craft / GUI_craft_possible — any recipe craftable now (respects fire).
bool craft_any_possible(const Inventory& inv, bool at_fire);

// Craft_32/build_fence1 & Craft_31/build_fence2 ingredient gates (item_for_remove_id required_count=5).
bool craft_build_fence1_can(const Inventory& inv);
bool craft_build_fence2_can(const Inventory& inv);

} // namespace mdaz
