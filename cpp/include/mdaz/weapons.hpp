#pragma once

#include "mdaz/combat.hpp"
#include "mdaz/types.hpp"

namespace mdaz {

// Weapon item IDs from original l_eng_items.xml (1.4.1).
// Firearm base damage / magazine / reload / pellets / speed: data.js (see weapons.cpp Wf).
struct WeaponDef {
  ItemId item_id;
  const char* name;
  WeaponStats stats;
  ItemId ammo_id; // 0 if melee / none
  int mag_capacity; // chamber size; 0 = feed straight from inventory (bows)
  float reload_sec;  // data.js Mag_reload duration (0 = legacy estimate)
  int pellets;       // projectiles per shot (shotgun_pellet x5)
  float bullet_speed;// C2 Bullet behaviour speed, px/s
  int c2_bullet;     // C2 bullet object type index (0 = none / melee)
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

// C2 Firearm shooting: Audio choose tags (original_docs/media/*.ogg).
const char* weapon_fire_sfx_tag(ItemId weapon_id, class Rng& rng);

// C2 player_weapons sprite layers (reload_rifle / molotov_reload visibility toggles).
// s_weapons / s_weapons2 PNG sheets are not in original_docs/images — overlay uses char_anims hands.
void touch_weapon_c2_audit_anchors();

} // namespace mdaz
