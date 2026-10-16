#include "mdaz/craft.hpp"

namespace mdaz {

namespace {

// Verified item IDs from l_eng_items.xml. Combinations inferred from item names
// and common Mini DAYZ recipes; FULL C2 Craft_N groups still need porting.
// Documented as PARTIAL in KNOWN_ISSUES.
const CraftRecipe kRecipes[] = {
  { "Craft_rags_bandage", 18, 1, 26, 1, 0, 0 },          // Rags -> Bandage
  { "Craft_sticks_arrow", 51, 1, 38, 1, 0, 0 },          // Wood sticks -> Crafted arrows
  { "Craft_ashwood_bow", 163, 1, 37, 1, 38, 2 },         // Ashwood + sticks -> Improvised bow
  { "Craft_firekit", 23, 1, 50, 1, 24, 1 },              // Wood piles + Matches -> Campfire kit
};

const int kCount = static_cast<int>(sizeof(kRecipes) / sizeof(kRecipes[0]));

int count_of(const Inventory& inv, ItemId id) {
  int n = 0;
  for (int i = 0; i < inv.slot_count(); ++i) {
    if (inv.slot(i).id == id) n += inv.slot(i).count;
  }
  return n;
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
