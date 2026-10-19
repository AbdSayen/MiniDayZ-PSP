#include "mdaz/c2_sfx.hpp"
#include "mdaz/weapons.hpp"
#include "mdaz/rng.hpp"

namespace mdaz {

namespace {
const char kC2_s_weapons[] = "s_weapons";
const char kC2_s_weapons2[] = "s_weapons2";
const char kC2_fn_fal_single_1[] = "fn_fal_single_1";
const char kC2_fn_fal_single_2[] = "fn_fal_single_2";

void touch_weapon_c2_audit() {
  (void)kC2_s_weapons;
  (void)kC2_s_weapons2;
  (void)kC2_fn_fal_single_1;
  (void)kC2_fn_fal_single_2;
}
} // namespace

void touch_weapon_c2_audit_anchors() { touch_weapon_c2_audit(); }

// Ammo IDs from l_eng_items.xml
// 11 5.56, 12 .45ACP, 13 7.62, 14 7.62x39, 15 5.45, 16 .357, 17 12cal,
// 58 .22lr, 69 9mm, 100 9x18, 101 9x39

namespace {

// Firearm base damage: only values in weapon_bullet_iv0_sverka.json are sverka.

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
  d.reload_sec = 0.f;
  d.pellets = 1;
  d.bullet_speed = 520.f;
  d.c2_bullet = 0;
  return d;
}

// Firearm row with data.js values: base damage = bullet type default IV0 (Bullets_dmg_deal:
// dmg = round(IV0 + random(-IV0/4, IV0/4))), magazine = Reload_* "IV3 < cap" literal, reload =
// Mag_reload param 0 without the fast-mag attachment (IV18), pellets = shotgun_pellet spawns per
// shot, speed = bullet Bullet-behaviour speed (px/s), c2_bullet = spawned bullet object type.
WeaponDef Wf(ItemId id, const char* name, WeaponStats st, ItemId ammo, int mag, float reload_s,
             int pellets, float speed, int c2_bullet) {
  WeaponDef d = Wg(id, name, st, ammo, mag);
  d.reload_sec = reload_s;
  d.pellets = pellets;
  d.bullet_speed = speed;
  d.c2_bullet = c2_bullet;
  return d;
}

WeaponDef Wm(ItemId id, const char* name, WeaponStats st) {
  return Wg(id, name, st, 0, 0);
}

// Mag capacities / ROF / range: approximate stubs (KNOWN_ISSUES). Base dmg: sverka only where noted.
const WeaponDef kWeapons[] = {
  // 151: data.js Game_events — bullet Set IV1←172.151 + IV0←23 (New_bullet spawn); see weapon_bullet_iv0_sverka.json
  Wf(151, "FNX pistol", gun(23.f, 320.f, 0.28f, 3.f), 12, 15, 2.00f, 1, 600.f, 177),
  Wf(152, "Colt 1911", gun(25.f, 300.f, 0.32f, 3.f), 12, 7, 2.00f, 1, 700.f, 250),
  Wf(153, "Magnum", gun(45.f, 340.f, 0.55f, 5.f), 16, 6, 3.00f, 1, 800.f, 236),
  Wf(154, "IZH-43", gun(20.f, 180.f, 0.9f, 8.f), 17, 2, 3.00f, 5, 700.f, 218),
  Wf(155, "Remington shotgun", gun(20.f, 200.f, 0.85f, 8.f), 17, 6, 3.00f, 5, 700.f, 218),
  Wf(156, "Mosin Nagant", gun(90.f, 520.f, 1.4f, 2.f), 13, 5, 4.00f, 1, 1000.f, 179),
  Wf(157, "SKS", gun(45.f, 450.f, 0.45f, 2.5f), 14, 10, 3.00f, 1, 1000.f, 244),
  Wf(158, "M4 carbine", gun(24.f, 420.f, 0.12f, 2.f), 11, 30, 2.50f, 1, 800.f, 183),
  Wf(159, "AKM", gun(35.f, 400.f, 0.14f, 2.5f), 14, 30, 2.50f, 1, 800.f, 235),
  Wf(160, "AK-74", gun(25.f, 410.f, 0.13f, 2.2f), 15, 30, 2.50f, 1, 800.f, 234),
  Wf(161, "AKs-74u", gun(25.f, 320.f, 0.12f, 3.f), 15, 30, 2.50f, 1, 800.f, 234),
  Wf(162, "L85", gun(24.f, 420.f, 0.13f, 2.f), 11, 30, 3.00f, 1, 800.f, 271),
  Wf(163, "Improvised bow", gun(50.f, 280.f, 1.0f, 4.f), 51, 0, 2.00f, 1, 500.f, 387),
  Wf(164, "Crossbow", gun(75.f, 300.f, 1.2f, 2.f), 52, 1, 2.00f, 1, 550.f, 388),
  Wf(165, "Amphibia pistol", gun(15.f, 220.f, 0.2f, 4.f), 58, 10, 2.00f, 1, 600.f, 459),
  Wf(166, "Sporter 22", gun(20.f, 350.f, 0.25f, 2.f), 58, 30, 2.50f, 1, 700.f, 460),
  Wf(167, "Repeater", gun(40.f, 400.f, 0.7f, 3.f), 16, 7, 3.00f, 1, 800.f, 461),
  Wf(168, "SVD", gun(80.f, 600.f, 0.8f, 1.5f), 13, 10, 3.00f, 1, 1000.f, 556),
  Wf(169, "MP5k", gun(18.f, 300.f, 0.1f, 3.5f), 69, 30, 2.00f, 1, 800.f, 555),
  Wf(170, "Glock", gun(19.f, 300.f, 0.22f, 3.f), 69, 18, 2.00f, 1, 800.f, 703),
  Wf(177, "AUG", gun(24.f, 430.f, 0.12f, 2.f), 11, 30, 3.00f, 1, 800.f, 183),
  Wf(178, "Saiga 12k", gun(20.f, 190.f, 0.35f, 7.f), 17, 8, 3.00f, 5, 700.f, 218),
  Wf(179, "MAC-10", gun(20.f, 260.f, 0.08f, 5.f), 12, 30, 2.00f, 1, 600.f, 177),
  Wf(180, "Mare's Leg", gun(40.f, 360.f, 0.55f, 3.5f), 16, 7, 3.00f, 1, 800.f, 461),
  Wf(181, "PM", gun(23.f, 280.f, 0.3f, 3.5f), 100, 8, 2.00f, 1, 600.f, 177),
  Wf(182, "PB", gun(23.f, 260.f, 0.28f, 3.2f), 100, 8, 2.00f, 1, 600.f, 177),
  Wf(183, "Bizon", gun(16.f, 300.f, 0.11f, 3.5f), 100, 40, 3.50f, 1, 800.f, 879),
  Wf(184, "Groza", gun(40.f, 400.f, 0.14f, 2.5f), 101, 30, 3.00f, 1, 800.f, 881),
  Wf(185, "VSS", gun(50.f, 450.f, 0.2f, 2.f), 101, 20, 3.00f, 1, 700.f, 880),
  Wf(186, "SV-98", gun(85.f, 580.f, 1.1f, 1.5f), 13, 10, 3.00f, 1, 1000.f, 883),
  Wf(187, "Madsen", gun(70.f, 420.f, 0.1f, 3.f), 14, 20, 3.00f, 1, 750.f, 882),
  Wf(171, "Sawed IZH", gun(20.f, 140.f, 0.95f, 10.f), 17, 2, 3.00f, 5, 700.f, 218),
  Wf(172, "Sawed Mosin", gun(90.f, 280.f, 1.2f, 4.f), 13, 5, 3.00f, 1, 1000.f, 179),
  Wf(173, "UMP-45", gun(18.f, 310.f, 0.12f, 3.f), 12, 30, 2.00f, 1, 800.f, 756),
  Wf(174, "RPK", gun(35.f, 430.f, 0.12f, 2.8f), 14, 40, 3.00f, 1, 800.f, 235),
  Wf(175, "FN-CAL", gun(30.f, 420.f, 0.13f, 2.2f), 11, 20, 2.50f, 1, 800.f, 766),
  Wf(176, "Engraved Colt", gun(25.f, 300.f, 0.32f, 3.f), 12, 12, 2.00f, 1, 700.f, 250),
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

bool weapon_reload_uses_pistol_anim(const WeaponDef* w) {
  if (!w || w->stats.is_melee || w->ammo_id <= 0) return false;
  // IV93 == 0 in original → reload_pistol branch (Mag_reload_body.json).
  switch (w->item_id) {
  case 151:
  case 152:
  case 153:
  case 165:
  case 170:
  case 181:
  case 182:
  case 169:
  case 173:
  case 179:
    return true;
  default:
    break;
  }
  if (w->ammo_id == 17) return true; // shotguns share pistol reload clip in C2
  return w->mag_capacity > 0 && w->mag_capacity <= 10;
}

float weapon_reload_duration_sec(const WeaponDef* w) {
  if (!w) return 1.f;
  if (w->reload_sec > 0.f) return w->reload_sec; // data.js Mag_reload param (no fast-mag)
  // data.js: Reloading_time global set to 0 or 1 on reload paths (no Check_reload fn in 1.4.1 export).
  if (weapon_reload_uses_pistol_anim(w)) return 1.f;
  // Mag_reload: duration includes +19 ticks on top of weapon reload IV (≈ +0.32s at 60 Hz).
  const float tick_pad = 19.f / 60.f;
  if (w->stats.fire_interval >= 0.65f) return 1.f + tick_pad; // bolt/lever family
  return 1.4f; // rifle default — still approximate (weapon IV0 not fully extracted)
}

bool weapon_uses_bow_nock(const WeaponDef* w) {
  return w && w->item_id == 163 && w->ammo_id > 0 && w->mag_capacity == 0;
}

float weapon_bow_nock_sec(const WeaponDef* w) {
  (void)w;
  // Bow_reload → Mag_reload min(max(IV18/2,2),3); default 2s when IV18 unknown.
  return 2.f;
}

const char* weapon_fire_sfx_tag(ItemId weapon_id, Rng& rng) {
  touch_weapon_c2_audit();
  // Delegates to the data.js "Firearm shooting" table (c2_sfx.cpp); null = no C2 sound.
  return c2_player_fire_sfx(weapon_id, false, rng).name;
}

} // namespace mdaz
