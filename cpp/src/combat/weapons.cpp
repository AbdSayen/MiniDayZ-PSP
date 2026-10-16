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

WeaponDef Wg(ItemId id, const char* name, WeaponStats st, ItemId ammo, int mag) {
  WeaponDef d;
  d.item_id = id;
  d.name = name;
  d.stats = st;
  d.ammo_id = ammo;
  d.mag_capacity = mag;
  return d;
}

WeaponDef Wm(ItemId id, const char* name, WeaponStats st) {
  return Wg(id, name, st, 0, 0);
}

// Firearm base dmg: FNX SetInstanceVar IV0=23 candidate from bullet_iv0_hints.
// Mag capacities approximate DayZ/MiniDAYZ stubs (KNOWN_ISSUES).
const WeaponDef kWeapons[] = {
  Wg(151, "FNX pistol", gun(23.f, 320.f, 0.28f, 3.f), 12, 15),
  Wg(152, "Colt 1911", gun(25.f, 300.f, 0.32f, 3.f), 12, 7),
  Wg(153, "Magnum", gun(45.f, 340.f, 0.55f, 5.f), 16, 6),
  Wg(154, "IZH-43", gun(55.f, 180.f, 0.9f, 8.f), 17, 2),
  Wg(155, "Remington shotgun", gun(60.f, 200.f, 0.85f, 8.f), 17, 6),
  Wg(156, "Mosin Nagant", gun(70.f, 520.f, 1.4f, 2.f), 13, 5),
  Wg(157, "SKS", gun(50.f, 450.f, 0.45f, 2.5f), 14, 10),
  Wg(158, "M4 carbine", gun(40.f, 420.f, 0.12f, 2.f), 11, 30),
  Wg(159, "AKM", gun(42.f, 400.f, 0.14f, 2.5f), 14, 30),
  Wg(160, "AK-74", gun(38.f, 410.f, 0.13f, 2.2f), 15, 30),
  Wg(161, "AKs-74u", gun(35.f, 320.f, 0.12f, 3.f), 15, 30),
  Wg(162, "L85", gun(40.f, 420.f, 0.13f, 2.f), 11, 30),
  Wg(163, "Improvised bow", gun(25.f, 280.f, 1.0f, 4.f), 51, 0),
  Wg(164, "Crossbow", gun(40.f, 300.f, 1.2f, 2.f), 52, 1),
  Wg(165, "Amphibia pistol", gun(18.f, 220.f, 0.2f, 4.f), 58, 10),
  Wg(166, "Sporter 22", gun(15.f, 350.f, 0.25f, 2.f), 58, 10),
  Wg(167, "Repeater", gun(48.f, 400.f, 0.7f, 3.f), 16, 7),
  Wg(168, "SVD", gun(75.f, 600.f, 0.8f, 1.5f), 13, 10),
  Wg(169, "MP5k", gun(28.f, 300.f, 0.1f, 3.5f), 69, 30),
  Wg(170, "Glock", gun(26.f, 300.f, 0.22f, 3.f), 69, 15),
  Wg(177, "AUG", gun(40.f, 430.f, 0.12f, 2.f), 11, 30),
  Wg(178, "Saiga 12k", gun(58.f, 190.f, 0.35f, 7.f), 17, 8),
  Wg(179, "MAC-10", gun(24.f, 260.f, 0.08f, 5.f), 12, 30),
  Wg(180, "Mare's Leg", gun(42.f, 360.f, 0.55f, 3.5f), 16, 6),
  Wm(42, "Hunter knife", melee(14.f, 36.f, 0.4f)),
  Wm(43, "Army knife", melee(16.f, 36.f, 0.4f)),
  Wm(44, "Butcher knife", melee(12.f, 36.f, 0.4f)),
  Wm(200, "Pipewrench", melee(18.f, 40.f, 0.6f)),
  Wm(201, "Shovel", melee(18.f, 42.f, 0.7f)),
  Wm(202, "Hatchet", melee(24.f, 40.f, 0.65f)),
  Wm(203, "Baseball bat", melee(20.f, 42.f, 0.6f)),
  Wm(204, "Fire axe", melee(30.f, 42.f, 0.75f)),
  Wm(205, "Crowbar", melee(22.f, 40.f, 0.65f)),
  Wm(206, "Pickaxe", melee(28.f, 40.f, 0.8f)),
  Wm(207, "Pitchfork", melee(24.f, 48.f, 0.7f)),
  Wm(208, "Sledgehammer", melee(40.f, 40.f, 1.0f)),
  Wm(209, "Sword", melee(15.f, 44.f, 0.55f)),
  Wm(210, "Pan", melee(12.f, 36.f, 0.5f)),
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
