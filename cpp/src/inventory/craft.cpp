#include "mdaz/craft.hpp"

namespace mdaz {

namespace {

// Recipes aligned with Mini DAYZ 1.4.1 guides (Prima/GameSkinny/fandom).
// FULL C2 Craft_N groups still incomplete — see KNOWN_ISSUES.
const CraftRecipe kRecipes[] = {
  // Bandage from rags
  { "Craft_rags_bandage", 18, 1, 26, 1, 0, 0, false, 0, false },
  { "Craft_shirt_rags", 26, 2, 350, 1, 0, 0, false, 0, false },
  // Arrows: wooden sticks + any knife (knife not consumed)
  { "Craft_sticks_arrow", 51, 5, 38, 5, 0, 0, false, 0, true },
  { "Craft_pile_sticks", 38, 4, 50, 1, 0, 0, false, 0, false }, // woodpile → 4 sticks
  { "Craft_sticks_ashwood", 37, 1, 38, 3, 0, 0, false, 0, true }, // 3 sticks+knife → ashwood
  // Bow: ashwood stick + rope
  { "Craft_ashwood_bow", 163, 1, 37, 1, 41, 1, false, 0, false },
  { "Craft_firekit", 23, 1, 50, 1, 24, 1, false, 0, false },
  { "Craft_fishing_rod", 87, 1, 37, 1, 39, 1, false, 0, false }, // ashwood + duct tape
  { "Craft_molotov", 102, 1, 25, 1, 26, 1, false, 0, false },
  { "Craft_composite_arrow", 52, 1, 51, 1, 37, 1, false, 0, false },
  // Improvised bag: burlap sack + rope; backpack: bag + 3 sticks
  { "Craft_improvised_bag", 454, 1, 40, 1, 41, 1, false, 0, false },
  { "Craft_improvised_pack", 455, 1, 454, 1, 38, 3, false, 0, false },
  { "Cook_steak", 61, 1, 60, 1, 0, 0, true, 0, false },
  { "Cook_rabbit", 72, 1, 71, 1, 0, 0, true, 0, false },
  { "Cook_perch", 95, 1, 94, 1, 0, 0, true, 0, false },
  { "Cook_ruffe", 96, 1, 93, 1, 0, 0, true, 0, false }, // Ruffe → big fillet
  // Improvised splint: sticks + rags → bandage (also clears limp in try_craft)
  { "Craft_improvised_splint", 18, 1, 38, 2, 26, 1, false, 0, false },
  // Torch from ashwood + rags (tinder)
  { "Craft_torch_tinder", 23, 1, 37, 1, 26, 1, false, 0, false },
  // Improvised rope from rags
  { "Craft_rags_rope", 41, 1, 26, 4, 0, 0, false, 0, false },
  // Papers as tinder: papers + sticks → matches
  { "Craft_papers_matches", 24, 1, 49, 1, 38, 2, false, 0, false },
  // Woodpile at fire → charcoal substitute: woodpile → heatpack-like burn (camp kit fuel)
  { "Craft_pile_firekit", 23, 1, 50, 2, 49, 1, false, 0, false }, // 2 piles + papers → kit
};

const int kCount = static_cast<int>(sizeof(kRecipes) / sizeof(kRecipes[0]));

int count_of(const Inventory& inv, ItemId id) {
  int n = 0;
  for (int i = 0; i < inv.slot_count(); ++i) {
    if (inv.slot(i).id == id) n += inv.slot(i).count;
  }
  return n;
}

bool has_knife(const Inventory& inv) {
  return count_of(inv, 42) > 0 || count_of(inv, 43) > 0 || count_of(inv, 44) > 0;
}

bool has_tool(const Inventory& inv, const CraftRecipe& r) {
  if (r.tool_any_knife) return has_knife(inv);
  if (r.tool_id == 0) return true;
  return count_of(inv, r.tool_id) > 0;
}

bool consume(Inventory& inv, ItemId id, int need) {
  for (int i = 0; i < inv.slot_count() && need > 0; ++i) {
    if (inv.slot(i).id == id) {
      const int take = inv.slot(i).count < need ? inv.slot(i).count : need;
      inv.remove_at(i, take);
      need -= take;
    }
  }
  return need == 0;
}

} // namespace

int craft_recipe_count() { return kCount; }

const CraftRecipe* craft_recipe_at(int i) {
  if (i < 0 || i >= kCount) return 0;
  return &kRecipes[i];
}

bool craft_can(const Inventory& inv, const CraftRecipe& r) {
  if (!has_tool(inv, r)) return false;
  if (count_of(inv, r.ing_a) < r.count_a) return false;
  if (r.ing_b != 0 && count_of(inv, r.ing_b) < r.count_b) return false;
  return true;
}

bool craft_do(Inventory& inv, const CraftRecipe& r, const ItemDatabase& db) {
  if (!craft_can(inv, r)) return false;
  if (!consume(inv, r.ing_a, r.count_a)) return false;
  if (r.ing_b != 0 && !consume(inv, r.ing_b, r.count_b)) return false;
  return inv.add_item(r.result_id, r.result_count, db);
}

} // namespace mdaz
