#pragma once

#include "mdaz/combat.hpp"
#include "mdaz/types.hpp"

namespace mdaz {

// Weapon item IDs from original l_eng_items.xml (1.4.1).
// Damage/ROF values: PLACEHOLDER until extracted from C2 — do not treat as sverka.
struct WeaponDef {
  ItemId item_id;
  const char* name;
  WeaponStats stats;
  ItemId ammo_id; // 0 if melee / none
  int mag_capacity; // chamber size; 0 = feed straight from inventory (bows)
};

const WeaponDef* find_weapon(ItemId item_id);
int weapon_def_count();
const WeaponDef* weapon_def_at(int index);

} // namespace mdaz
