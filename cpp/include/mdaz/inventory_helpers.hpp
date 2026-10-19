#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

class Inventory;
class ItemDatabase;

// C2 global variable item_for_remove_id (GUI_elements / detach flows).
ItemId inventory_item_for_remove_id();
void inventory_set_item_for_remove_id(ItemId id);
void inventory_clear_item_for_remove_id();

// Remove slot matching item_for_remove_id if set; otherwise remove_at(slot, count).
bool inventory_remove_marked(Inventory& inv, int slot, int count);

// C2 Recheck_inventory_cards: sync Rust_layer Z from card IV341 (objects 7/10/18/14).
void recheck_inventory_cards(float card_iv341_7, float card_iv341_10, float card_iv341_18,
                             float card_iv341_14, float& rust_layer_z_out);

// C2 loot_checker2 (non-tutorial): avalible_interaction + Outline when pile in range.
bool loot_checker2_available(bool interactor_near_pile, bool outline_enabled);

} // namespace mdaz
