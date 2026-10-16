#include "mdaz/weapons.hpp"

namespace mdaz {

// Ammo IDs from l_eng_items.xml
// 11 5.56, 12 .45ACP, 13 7.62, 14 7.62x39, 15 5.45, 16 .357, 17 12cal,
// 58 .22lr, 69 9mm, 100 9x18, 101 9x39

namespace {

WeaponStats gun(float dmg, float range, float interval, float disp) {
  WeaponStats w;
  w.damage = dmg;
  w.range = range;
  w.fire_interval = interval;
  w.dispersion_deg = disp;
  w.is_melee = false;
  return w;
}

WeaponStats melee(float dmg, float range, float interval) {
  WeaponStats w;
  w.damage = dmg;
  w.range = range;
  w.fire_interval = interval;
  w.dispersion_deg = 0.f;
  w.is_melee = true;
  return w;
}

// Damage numbers are INITIAL GUESSES for structure only — KNOWN_ISSUES.md
const WeaponDef kWeapons[] = {
  { 151, "FNX pistol", gun(28.f, 320.f, 0.28f, 3.f), 12 },
  { 152, "Colt 1911", gun(30.f, 300.f, 0.32f, 3.f), 12 },
  { 153, "Magnum", gun(45.f, 340.f, 0.55f, 5.f), 16 },
  { 154, "IZH-43", gun(55.f, 180.f, 0.9f, 8.f), 17 },
  { 155, "Remington shotgun", gun(60.f, 200.f, 0.85f, 8.f), 17 },
  { 156, "Mosin Nagant", gun(70.f, 520.f, 1.4f, 2.f), 13 },
  { 157, "SKS", gun(50.f, 450.f, 0.45f, 2.5f), 14 },
  { 158, "M4 carbine", gun(40.f, 420.f, 0.12f, 2.f), 11 },
  { 159, "AKM", gun(42.f, 400.f, 0.14f, 2.5f), 14 },
  { 160, "AK-74", gun(38.f, 410.f, 0.13f, 2.2f), 15 },
  { 161, "AKs-74u", gun(35.f, 320.f, 0.12f, 3.f), 15 },
  { 162, "L85", gun(40.f, 420.f, 0.13f, 2.f), 11 },
  { 163, "Improvised bow", gun(25.f, 280.f, 1.0f, 4.f), 51 },
  { 164, "Crossbow", gun(40.f, 300.f, 1.2f, 2.f), 52 },
  { 165, "Amphibia pistol", gun(18.f, 220.f, 0.2f, 4.f), 58 },
  { 166, "Sporter 22", gun(15.f, 350.f, 0.25f, 2.f), 58 },
  { 167, "Repeater", gun(48.f, 400.f, 0.7f, 3.f), 16 },
  { 168, "SVD", gun(75.f, 600.f, 0.8f, 1.5f), 13 },
  { 169, "MP5k", gun(28.f, 300.f, 0.1f, 3.5f), 69 },
  { 170, "Glock", gun(26.f, 300.f, 0.22f, 3.f), 69 },
  { 200, "Pipewrench", melee(22.f, 38.f, 0.6f), 0 },
  { 204, "Fire axe", melee(35.f, 42.f, 0.75f), 0 },
  { 205, "Crowbar", melee(28.f, 40.f, 0.65f), 0 },
  { 209, "Sword", melee(32.f, 44.f, 0.55f), 0 },
};

const int kWeaponCount = static_cast<int>(sizeof(kWeapons) / sizeof(kWeapons[0]));

} // namespace

const WeaponDef* find_weapon(ItemId item_id) {
  for (int i = 0; i < kWeaponCount; ++i) {
    if (kWeapons[i].item_id == item_id) return &kWeapons[i];
  }
  return 0;
}

int weapon_def_count() { return kWeaponCount; }

const WeaponDef* weapon_def_at(int index) {
  if (index < 0 || index >= kWeaponCount) return 0;
  return &kWeapons[index];
}

} // namespace mdaz
