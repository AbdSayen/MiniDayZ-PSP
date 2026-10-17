#pragma once

#include "mdaz/inventory.hpp"
#include "mdaz/types.hpp"

namespace mdaz {

// Sum matching ammo stacks in bag slots (Check_ammo_inventory bag portion).
int count_ammo_in_bag(const Inventory& inv, ItemId ammo_id);

// Remove up to `need` rounds from bag slots (low index first). Returns amount taken.
int consume_ammo_from_bag(Inventory& inv, ItemId ammo_id, int need);

} // namespace mdaz
