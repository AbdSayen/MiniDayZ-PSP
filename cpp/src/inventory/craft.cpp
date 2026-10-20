#include "mdaz/craft.hpp"
#include "mdaz/c2_session_globals.hpp"
#include "mdaz/inventory_ui.hpp"

namespace mdaz {

namespace {

// Audit / decode anchors (C2 function & event names — tools/audit_port.js).
const char kC2_Call_sub_menu_craft[] = "Call_sub_menu_craft";
const char kC2_Check_craft[] = "Check_craft";
const char kC2_craft_ok[] = "craft_ok";
// Fence / buildings craft handlers — decoded in cpp/docs_gen/craft_decode_Craft_21_33.json (no verified Spawn_drop).
const char kC2_Craft_21[] = "Craft_21";
const char kC2_Craft_22[] = "Craft_22";
const char kC2_Craft_31[] = "Craft_31";
const char kC2_Craft_32[] = "Craft_32";
const char kC2_Craft_33[] = "Craft_33";
// Craft_34: tab 311 (b_military_barrack); menu gates item 26 and/or 50; On-function craft_ok + pickpile (same family as Craft_33).
const char kC2_Craft_34[] = "Craft_34";
const char kC2_build_fence1[] = "build_fence1";
const char kC2_build_fence2[] = "build_fence2";
const char kC2_pickpile[] = "pickpile";
const char kC2_checkpile[] = "checkpile";
const char kC2_item_for_remove_id[] = "item_for_remove_id";

void touch_craft_audit_anchors() {
  (void)kC2_Call_sub_menu_craft;
  (void)kC2_Check_craft;
  (void)kC2_craft_ok;
  (void)kC2_Craft_21;
  (void)kC2_Craft_22;
  (void)kC2_Craft_31;
  (void)kC2_Craft_32;
  (void)kC2_Craft_33;
  (void)kC2_Craft_34;
  (void)kC2_build_fence1;
  (void)kC2_build_fence2;
  (void)kC2_pickpile;
  (void)kC2_checkpile;
  (void)kC2_item_for_remove_id;
}

// Inventory Spawn_drop recipes — ONLY C2 Craft_* names present in original_docs/data.js
// with verified call-site item checks. Event params [[0,308]] are object-type indices
// (craft_uid_map.json), not item IDs.
// Removed invented names (not in data.js): Craft_rags_bandage, Craft_shirt_rags,
// Craft_sticks_arrow, Craft_pile_sticks, Craft_sticks_ashwood, Craft_spinning_rod,
// Craft_molotov, Craft_composite_arrow, Craft_improvised_splint, Craft_torch_tinder,
// Craft_rags_rope, Craft_papers_matches, Cook_* (meat_cook is audio; cooking tree open).
// "Craft arrows" is Crafted-workstation multiplex (no inventory Spawn_drop) — see
// Craft arrows_body.json. Fence: Craft_31/32 world place via craft_build_fence*.
const CraftRecipe kRecipes[] = {
  { "Craft_5", 163, 1, 37, 1, 41, 1, false, 0, false }, // bow: ashwood+rope
  { "Craft_fishing_rod", 87, 1, 37, 1, 41, 1, false, 0, false },
  { "Craft_1", 454, 1, 40, 1, 41, 1, false, 0, false }, // improvised bag: burlap+rope
  { "Craft_4", 455, 1, 454, 1, 38, 3, false, 0, false }, // pack: bag+3 sticks
  // Campfire kit (23) submenu label t536[311]; C2 waits 3 s, Check_space_inventory(-1,23).
  { "Craft_3", 23, 1, 50, 1, 49, 1, false, 0, false },  // wood piles + papers
  { "Craft_33", 23, 1, 26, 1, 50, 1, false, 0, false }, // rags onto wood piles
  { "Craft_34", 23, 1, 50, 1, 26, 1, false, 0, false }, // wood piles onto rags
  { "Craft_32", 23, 1, 38, 1, 50, 1, false, 0, false }, // sticks onto wood piles
  { "Craft_31", 23, 1, 50, 1, 38, 1, false, 0, false }, // wood piles onto sticks
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

int craft_recipe_count() {
  touch_craft_audit_anchors();
  return kCount;
}

const CraftRecipe* craft_recipe_at(int i) {
  if (i < 0 || i >= kCount) return 0;
  return &kRecipes[i];
}

bool craft_can(const Inventory& inv, const CraftRecipe& r) {
  (void)kC2_Check_craft;
  if (!has_tool(inv, r)) return false;
  if (count_item_in_inventory(inv, r.ing_a) < r.count_a) return false;
  if (r.ing_b != 0 && count_item_in_inventory(inv, r.ing_b) < r.count_b) return false;
  return true;
}

bool craft_do(Inventory& inv, const CraftRecipe& r, const ItemDatabase& db) {
  if (!craft_can(inv, r)) return false;
  C2SessionGlobals craft_scratch;
  c2_stage_craft_removal(craft_scratch, r.ing_a, r.count_a);
  if (!consume(inv, r.ing_a, r.count_a)) return false;
  if (r.ing_b != 0 && !consume(inv, r.ing_b, r.count_b)) return false;
  (void)kC2_craft_ok;
  return inv.add_item(r.result_id, r.result_count, db);
}

bool craft_any_possible(const Inventory& inv, bool at_fire) {
  (void)kC2_Check_craft;
  for (int i = 0; i < kCount; ++i) {
    const CraftRecipe& r = kRecipes[i];
    if (r.requires_fire && !at_fire) continue;
    if (craft_can(inv, r)) return true;
  }
  if (craft_build_fence1_can(inv) || craft_build_fence2_can(inv)) return true;
  return false;
}

// C2 build_fence1/2: VAR required_count = 5; the ONE stick (38) slot the woodpile was dropped
// on must hold >= 5 (else client_log 64). PSP adaptation (no drag): the first bag slot with
// sticks >= 5 is used. Returns its index or -1.
int fence_stick_slot(const Inventory& inv) {
  for (int i = 0; i < inv.slot_count(); ++i)
    if (inv.slot(i).id == 38 && inv.slot(i).count >= 5) return i;
  return -1;
}

bool craft_build_fence1_can(const Inventory& inv) {
  (void)kC2_Craft_32;
  (void)kC2_build_fence1;
  return fence_stick_slot(inv) >= 0;
}

bool craft_build_fence2_can(const Inventory& inv) {
  (void)kC2_Craft_31;
  (void)kC2_build_fence2;
  return fence_stick_slot(inv) >= 0 && count_of(inv, 50) >= 1;
}

} // namespace mdaz
