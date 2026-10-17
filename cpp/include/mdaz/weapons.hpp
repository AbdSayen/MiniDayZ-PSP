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

// C2 Mag_reload: reload_pistol vs Reload_rifle anim/audio when player_weapons IV93 == 0.
bool weapon_reload_uses_pistol_anim(const WeaponDef* w);

// Reloading_time: many pistol paths set global to 1s; Mag_reload adds 19 ticks to weapon IV0.
float weapon_reload_duration_sec(const WeaponDef* w);

// Bow (163): Mag_reload param 2 + Bow_reload anim; clamp reload ≈2–3s in C2.
bool weapon_uses_bow_nock(const WeaponDef* w);
float weapon_bow_nock_sec(const WeaponDef* w);

} // namespace mdaz
