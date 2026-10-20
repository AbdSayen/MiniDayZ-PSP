// Item 11d: C2 GUI_Drag (DragnDrop behaviour on family t925) + gui_pickpile, 1:1 with data.js:
//  - release over a bag/container cell (gui_item_backpack IV0 "Slot"), GUI_craft_possible = 0:
//    Replace_clear_slot(target region, target index, dragged IV3..IV7) -> Replace_fit_slot puts
//    the target content into the source cell and the dragged content into the target (a swap,
//    an empty target just receives the item);
//  - GUI_craft_possible = 1 (Check_craft "Slot": same frame for the Combine ids) ->
//    Call_sub_menu_craft -> sub_menu "Combine" (Combine_ammo);
//  - release over a gui_pickpile entry (IV0 "pickpile"): Replace_slot_ground(IV17 = t912 UID,
//    ...) -> Drop_slot(source) then source cell = t912 IV2 / IV1, t912 destroyed.
//  - checkpile: t532 rows = t909 ground gear (IV5 = -1) then every t912, 7 per page.
#include "mdaz/game.hpp"
#include "mdaz/c2_sfx.hpp"
#include "mdaz/item_db.hpp"
#include "mdaz/loot.hpp"
#include <cstdio>

namespace mdaz {

namespace {
// Check_craft "Slot" pairs (target frame, dragged frame) with different ids: their
// Call_sub_menu_craft entries are crafting recipes (Craft_*, Attach Battery, ...), not Combine.
const int kCraftSlotPairs[][2] = {
  {40, 41}, {37, 41}, {41, 37}, {41, 40}, {44, 38}, {38, 42}, {38, 43}, {38, 44}, {44, 40},
  {40, 42}, {40, 43}, {40, 44}, {49, 50}, {50, 26}, {50, 38}, {50, 49}, {38, 44}, {22, 75}, {75, 22},
};
const float kPickpileReach = 40.f;   // port pile reach (C2: player_collision_base overlaps item)
float dist2(Vec2 a, Vec2 b) { const float dx = a.x - b.x, dy = a.y - b.y; return dx * dx + dy * dy; }
}

int c2_check_craft_slot(ItemId target, ItemId dragged) {
  if (target <= 0 || dragged <= 0) return 0;
  if (target == dragged && c2_combine_menu(target)) return 1;   // -> "Combine"
  for (unsigned i = 0; i < sizeof(kCraftSlotPairs) / sizeof(kCraftSlotPairs[0]); ++i)
    if (kCraftSlotPairs[i][0] == target && kCraftSlotPairs[i][1] == dragged) return 2;
  return 0;
}

int Game::pickpile_count() const { return pickpile_entry(-1, 0, 0); }

// k-th checkpile row (gear first, then items, in pile order); k < 0 returns the row count.
int Game::pickpile_entry(int k, int* pile_out, int* item_out) const {
  int n = 0;
  for (int pass = 0; pass < 2; ++pass) {
    for (int i = 0; i < loot_.count(); ++i) {
      const LootPile* p = loot_.at(i);
      if (!p->active || p->item_count <= 0) continue;
      if (dist2(p->pos, player_.pos) > kPickpileReach * kPickpileReach) continue;
      for (int j = 0; j < p->item_count; ++j) {
        if (p->items[j] <= 0) continue;
        if ((pass == 0) != is_gear_item(p->items[j])) continue;
        if (n == k) {
          if (pile_out) *pile_out = i;
          if (item_out) *item_out = j;
          return 1;
        }
        ++n;
      }
    }
  }
  return k < 0 ? n : 0;
}

ItemId Game::pickpile_id(int k, int* count_out) const {
  int pi = -1, j = -1;
  if (!pickpile_entry(k, &pi, &j)) return 0;
  const LootPile* p = loot_.at(pi);
  if (count_out) *count_out = p->counts[j];
  return p->items[j];
}

void Game::compact_pile(int pi) {
  LootPile* p = loot_.at(pi);
  int w = 0;
  for (int j = 0; j < p->item_count; ++j) {
    if (p->items[j] <= 0) continue;
    if (w != j) {
      p->items[w] = p->items[j];
      p->counts[w] = p->counts[j];
      p->iv6[w] = p->iv6[j];
      p->boxes[w] = p->boxes[j];
      p->boxes[j].clear();
    }
    ++w;
  }
  p->item_count = w;
  if (w == 0) p->active = false;
}

// Click on a pickpile row: Pick_player_item (gear) / Pick_inventory_item (one empty cell).
void Game::pick_pile_entry(int k) {
  int pi = -1, j = -1;
  if (!pickpile_entry(k, &pi, &j)) return;
  LootPile* p = loot_.at(pi);
  const ItemId id = p->items[j];
  bool taken = false;
  if (is_gear_item(id)) {
    ItemId old = 0;
    GearBox ob;
    float oiv = -1.f;
    if (inventory_.equip_gear(id, &old, &p->boxes[j], &ob, p->iv6[j], &oiv)) {
      taken = true;
      p->boxes[j].clear();
      p->items[j] = 0;
      p->counts[j] = 0;
      compact_pile(pi);
      if (old) drop_gear_and_overflow(old, &ob, oiv);
    }
  } else if (inventory_.add_item_new_slot(id, p->counts[j], p->iv6[j])) {
    taken = true;
    p->items[j] = 0;
    p->counts[j] = 0;
    compact_pile(pi);
  }
  if (taken) sfx_play(c2_pickup_sfx(id, is_gear_item(id), sfx_rng_), player_.pos);
  else inv_full_flash_ = 1.5f;
}

void Game::c2_drag_drop_on_slot(int src, int dst) {
  if (src < 0 || dst < 0 || src == dst) return;
  if (src >= inventory_.slot_count() || dst >= inventory_.slot_count()) return;
  InvSlot& a = inventory_.slot(src);
  InvSlot& b = inventory_.slot(dst);
  if (a.empty()) return;
  const int craft = c2_check_craft_slot(b.id, a.id);
  if (craft == 1) {   // sub_menu "Combine" -> Combine_ammo
    if (inventory_.combine_slots(src, dst, items_) != -1) sfx2d("eject", 0.f);
    return;
  }
  if (craft == 2) return;   // Call_sub_menu_craft recipe entries: not ported (KNOWN_ISSUES 11d)
  // Replace_clear_slot -> Replace_fit_slot: swap the two cells (id, count, t909 IV6).
  const InvSlot t = b;
  b = a;
  a = t;
}

void Game::c2_drag_drop_on_pile(int src, int k) {
  int pi = -1, j = -1;
  if (src < 0 || src >= inventory_.slot_count() || inventory_.slot(src).empty()) return;
  if (!pickpile_entry(k, &pi, &j)) return;
  LootPile* p = loot_.at(pi);
  const ItemId gid = p->items[j];
  if (c2_check_craft_slot(gid, inventory_.slot(src).id) != 0) return;   // craft sub_menu (not ported)
  // Replace_slot_ground: Drop_slot(source) first; then the t912 item (IV2 id, IV1 count) goes
  // into the source cell. A t909 ground gear row is not a t912 -> the cell stays empty.
  InvSlot take;
  if (!is_gear_item(gid)) {
    take.id = gid;
    take.count = p->counts[j];
    take.iv6 = p->iv6[j];
    p->items[j] = 0;
    p->counts[j] = 0;
    compact_pile(pi);
  }
  try_drop_slot(src);
  if (!take.empty()) inventory_.slot(src) = take;
}

void Game::update_pickpile_ui(const InputState& in) {
  InventoryUiState& ui = inventory_ui_;
  const int n = pickpile_count();
  if (n <= 0) { ui.focus_pile = -1; ui.pile_page = 0; return; }
  const int maxpage = (n - 1) / 7;   // gui_pickpile_maxpage = ceil(rows / 7) - 1
  if (ui.pile_page > maxpage) ui.pile_page = maxpage;
  int k = ui.pile_page * 7 + ui.focus_pile;
  if (k >= n) k = n - 1;
  if (in.nav_left) {
    if (k > 0) k -= 1;
  }
  if (in.nav_right && k + 1 < n) k += 1;
  ui.pile_page = k / 7;
  ui.focus_pile = k % 7;
  if (in.nav_up) {
    ui.focus_pile = -1;
    ui.pile_page = 0;
    const int cap = inventory_.capacity();
    ui.focus_bag_slot = cap > 0 ? cap - 1 : 0;
    return;
  }
  if (in.throw_pressed && ui.drag_src >= 0) {   // release over the pickpile row
    c2_drag_drop_on_pile(ui.drag_src, k);
    ui.drag_src = -1;
    return;
  }
  if (ui.drag_src >= 0) return;
  if (in.confirm_pressed || in.attack_pressed) pick_pile_entry(k);
}

// Test-only (item 11d shots): two bag cells + a ground pile, then drag steps.
void Game::harness_drag(int step) {
  InventoryUiState& ui = inventory_ui_;
  if (step == 0) {
    for (int i = 0; i < inventory_.slot_count(); ++i) inventory_.slot(i) = InvSlot();
    inventory_.slot(0).id = 156; inventory_.slot(0).count = 1; inventory_.slot(0).iv6 = 75.f;
    inventory_.slot(1).id = 12; inventory_.slot(1).count = 25;
    inventory_.slot(2).id = 12; inventory_.slot(2).count = 30;
    loot_.spawn_item_iv6(Vec2(player_.pos.x + 6.f, player_.pos.y), 54, 30, -1.f);
    loot_.spawn_item_iv6(Vec2(player_.pos.x + 6.f, player_.pos.y + 4.f), 21, 1, -1.f);
    ui.open();
    show_bag_ = true;
    ui.focus_bag_slot = 0;
  } else if (step == 1) {   // pick cell 0 up, move cursor to cell 1 (hover)
    ui.drag_src = 0;
    ui.focus_bag_slot = 1;
  } else if (step == 2) {   // release over cell 1: swap
    c2_drag_drop_on_slot(0, 1);
    ui.drag_src = -1;
  } else if (step == 3) {   // drag cell 0 (ammo 25) onto cell 2 (ammo 30): Combine
    c2_drag_drop_on_slot(0, 2);
  } else if (step == 4) {   // drag cell 1 (rifle) onto pickpile row 0: Replace_slot_ground
    ui.drag_src = 1;
    ui.focus_bag_slot = -1;
    ui.focus_pile = 0;
  } else if (step == 5) {
    c2_drag_drop_on_pile(1, 0);
    ui.drag_src = -1;
  }
  char line[200];
  std::snprintf(line, sizeof(line), "harness_drag step=%d s0=%d/%d s1=%d/%d s2=%d/%d pile=%d",
                step, (int)inventory_.slot(0).id, inventory_.slot(0).count, (int)inventory_.slot(1).id,
                inventory_.slot(1).count, (int)inventory_.slot(2).id, inventory_.slot(2).count,
                pickpile_count());
  std::printf("%s\n", line);
  if (platform_) platform_->log_line(line);
}

} // namespace mdaz
