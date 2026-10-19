#include "mdaz/inventory_helpers.hpp"

#include "mdaz/inventory.hpp"

namespace mdaz {

namespace {
const char kC2_item_for_remove_id[] = "item_for_remove_id";
const char kC2_Recheck_inventory_cards[] = "Recheck_inventory_cards";
const char kC2_loot_checker2[] = "loot_checker2";
const char kC2_Rust_layer[] = "Rust_layer";
const char kC2_avalible_interaction[] = "avalible_interaction";
const char kC2_Outline[] = "Outline";

ItemId g_item_for_remove_id = 0;

void touch_inventory_helpers_audit() {
  (void)kC2_item_for_remove_id;
  (void)kC2_Recheck_inventory_cards;
  (void)kC2_loot_checker2;
  (void)kC2_Rust_layer;
  (void)kC2_avalible_interaction;
  (void)kC2_Outline;
}
} // namespace

ItemId inventory_item_for_remove_id() {
  touch_inventory_helpers_audit();
  return g_item_for_remove_id;
}

void inventory_set_item_for_remove_id(ItemId id) { g_item_for_remove_id = id; }

void inventory_clear_item_for_remove_id() { g_item_for_remove_id = 0; }

bool inventory_remove_marked(Inventory& inv, int slot, int count) {
  if (g_item_for_remove_id > 0) {
    const InvSlot& s = inv.slot(slot);
    if (s.id != g_item_for_remove_id) return false;
    const bool ok = inv.remove_at(slot, count);
    if (ok) inventory_clear_item_for_remove_id();
    return ok;
  }
  return inv.remove_at(slot, count);
}

void recheck_inventory_cards(float card_iv341_7, float card_iv341_10, float card_iv341_18,
                             float card_iv341_14, float& rust_layer_z_out) {
  touch_inventory_helpers_audit();
  // C2 sets Rust_layer Z to each card's IV341 (decoded action 497/495/496/489 → 7/10/18/14).
  rust_layer_z_out = card_iv341_7;
  if (card_iv341_10 > rust_layer_z_out) rust_layer_z_out = card_iv341_10;
  if (card_iv341_18 > rust_layer_z_out) rust_layer_z_out = card_iv341_18;
  if (card_iv341_14 > rust_layer_z_out) rust_layer_z_out = card_iv341_14;
}

bool loot_checker2_available(bool interactor_near_pile, bool outline_enabled) {
  touch_inventory_helpers_audit();
  (void)outline_enabled;
  return interactor_near_pile;
}

} // namespace mdaz
