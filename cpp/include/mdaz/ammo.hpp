#pragma once

#include "mdaz/inventory.hpp"
#include "mdaz/types.hpp"

namespace mdaz {

// Sum matching ammo stacks in bag slots (Check_ammo_inventory bag portion).
int count_ammo_in_bag(const Inventory& inv, ItemId ammo_id);

// Bag + Check_ammo_pants → outerwear → vest → backpack (C2 cascade after bag).
int count_ammo_for_reload(const Inventory& inv, ItemId ammo_id);

// Remove up to `need` rounds from bag slots (low index first). Returns amount taken.
int consume_ammo_from_bag(Inventory& inv, ItemId ammo_id, int need);

// Bag first, then clothing regions in C2 order. Returns total taken.
int consume_ammo_for_reload(Inventory& inv, ItemId ammo_id, int need);

} // namespace mdaz
