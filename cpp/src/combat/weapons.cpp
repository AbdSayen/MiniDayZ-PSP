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
constexpr float kGunDmgPlaceholder = 25.f;

WeaponStats gun(float dmg, float range, float interval, float disp) {
  WeaponStats w;
  w.damage = dmg;
  w.range = range;
  w.fire_interval = interval;
  w.dispersion_deg = disp;
  w.is_melee = false;
  return w;
}

WeaponStats gun_ph(float range, float interval, float disp) {
  return gun(kGunDmgPlaceholder, range, interval, disp);
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

// Mag capacities / ROF / range: approximate stubs (KNOWN_ISSUES). Base dmg: sverka only where noted.
const WeaponDef kWeapons[] = {
  // 151: data.js Game_events — bullet Set IV1←172.151 + IV0←23 (New_bullet spawn); see weapon_bullet_iv0_sverka.json
  Wg(151, "FNX pistol", gun(23.f, 320.f, 0.28f, 3.f), 12, 15),
  Wg(152, "Colt 1911", gun_ph(300.f, 0.32f, 3.f), 12, 7),
  Wg(153, "Magnum", gun_ph(340.f, 0.55f, 5.f), 16, 6),
  Wg(154, "IZH-43", gun_ph(180.f, 0.9f, 8.f), 17, 2),
  Wg(155, "Remington shotgun", gun_ph(200.f, 0.85f, 8.f), 17, 6),
  Wg(156, "Mosin Nagant", gun_ph(520.f, 1.4f, 2.f), 13, 5),
  Wg(157, "SKS", gun_ph(450.f, 0.45f, 2.5f), 14, 10),
  Wg(158, "M4 carbine", gun_ph(420.f, 0.12f, 2.f), 11, 30),
  Wg(159, "AKM", gun_ph(400.f, 0.14f, 2.5f), 14, 30),
  Wg(160, "AK-74", gun_ph(410.f, 0.13f, 2.2f), 15, 30),
  Wg(161, "AKs-74u", gun_ph(320.f, 0.12f, 3.f), 15, 30),
  Wg(162, "L85", gun_ph(420.f, 0.13f, 2.f), 11, 30),
  Wg(163, "Improvised bow", gun_ph(280.f, 1.0f, 4.f), 51, 0),
  Wg(164, "Crossbow", gun_ph(300.f, 1.2f, 2.f), 52, 1),
  Wg(165, "Amphibia pistol", gun_ph(220.f, 0.2f, 4.f), 58, 10),
  Wg(166, "Sporter 22", gun_ph(350.f, 0.25f, 2.f), 58, 10),
  Wg(167, "Repeater", gun_ph(400.f, 0.7f, 3.f), 16, 7),
  Wg(168, "SVD", gun_ph(600.f, 0.8f, 1.5f), 13, 10),
  Wg(169, "MP5k", gun_ph(300.f, 0.1f, 3.5f), 69, 30),
  Wg(170, "Glock", gun_ph(300.f, 0.22f, 3.f), 69, 15),
  Wg(177, "AUG", gun_ph(430.f, 0.12f, 2.f), 11, 30),
  Wg(178, "Saiga 12k", gun_ph(190.f, 0.35f, 7.f), 17, 8),
  Wg(179, "MAC-10", gun_ph(260.f, 0.08f, 5.f), 12, 30),
  Wg(180, "Mare's Leg", gun_ph(360.f, 0.55f, 3.5f), 16, 6),
  Wg(181, "PM", gun_ph(280.f, 0.3f, 3.5f), 100, 8),
  Wg(182, "PB", gun_ph(260.f, 0.28f, 3.2f), 100, 8),
  Wg(183, "Bizon", gun_ph(300.f, 0.11f, 3.5f), 100, 64),
  Wg(184, "Groza", gun_ph(400.f, 0.14f, 2.5f), 101, 20),
  Wg(185, "VSS", gun_ph(450.f, 0.2f, 2.f), 101, 10),
  Wg(186, "SV-98", gun_ph(580.f, 1.1f, 1.5f), 13, 10),
  Wg(187, "Madsen", gun_ph(420.f, 0.1f, 3.f), 14, 30),
  Wg(171, "Sawed IZH", gun_ph(140.f, 0.95f, 10.f), 17, 2),
  Wg(172, "Sawed Mosin", gun_ph(280.f, 1.2f, 4.f), 13, 5),
  Wg(173, "UMP-45", gun_ph(310.f, 0.12f, 3.f), 12, 25),
  Wg(174, "RPK", gun_ph(430.f, 0.12f, 2.8f), 14, 40),
  Wg(175, "FN-CAL", gun_ph(420.f, 0.13f, 2.2f), 11, 30),
  Wg(176, "Engraved Colt", gun_ph(300.f, 0.32f, 3.f), 12, 7),
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
  auto pick2 = [&rng](const char* a, const char* b) {
    return rng.next_int(0, 1) ? b : a;
  };
  switch (weapon_id) {
  case 175: // fncal — fn_fal_single_1_impl.json (choose + muzzle on player_weapons)
    return pick2(kC2_fn_fal_single_1, kC2_fn_fal_single_2);
  case 159:
  case 174:
    return pick2("akm_3", "akm_5");
  case 160:
  case 161:
    return pick2("Ak74_1", "Ak74_3");
  case 151:
    return pick2("FNX_1", "FNX_2");
  case 152:
  case 176:
    return pick2("colt_1", "colt_2");
  case 170:
    return pick2("glock19_single_1", "glock19_single_2");
  case 163:
  case 164:
    return "Bow_1";
  case 154:
  case 155:
  case 171:
    return pick2("izh_1", "izh_2");
  case 178:
    return "saiga12";
  case 156:
  case 172:
    return pick2("mosin_1", "mosin_2");
  case 158:
  case 162:
  case 177:
    return pick2("m4_single_1", "m4_single_2");
  case 169:
  case 173:
    return pick2("mp5k_single_0", "mp5k_single_1");
  case 179:
    return pick2("mac10_1", "mac10_2");
  case 168:
  case 186:
    return pick2("svd_single_0", "svd_single_1");
  case 181:
    return "pm_shot";
  case 182:
    return pick2("pb_shot_1", "pb_shot_2");
  case 184:
    return pick2("groza_shot_1", "groza_shot_2");
  case 185:
    return "vss_shot_3";
  case 187:
    return "madsen_shot";
  case 183:
    return "bizon_shot_1";
  default:
    break;
  }
  return "gunshot";
}

} // namespace mdaz
