// C2 Audio tables (data.js). See include/mdaz/c2_sfx.hpp and docs/AUDIO_AUDIT.md.
#include "mdaz/c2_sfx.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/weapons.hpp"

namespace mdaz {

namespace {
const char* pick(Rng& rng, const char* const* a, int n) { return a[rng.next_int(0, n - 1)]; }
#define MDAZ_CHOOSE(rng, ...) \
  ([&]() { static const char* const k_[] = { __VA_ARGS__ }; \
           return pick(rng, k_, (int)(sizeof(k_) / sizeof(k_[0]))); }())
C2Sfx sfx(const char* n, float db, bool at, const char* tag) { C2Sfx s; s.name = n; s.db = db; s.at_object = at; s.tag = tag; return s; }
C2Sfx none() { return sfx(0, 0.f, false, ""); }
} // namespace

bool c2_is_pistol_item(ItemId id) {
  switch (id) {
  case 151: case 152: case 153: case 165: case 170: case 171: case 172: case 176:
  case 179: case 180: case 181: case 182:
    return true;
  default:
    return false;
  }
}

C2Sfx c2_player_fire_sfx(ItemId id, bool sil, Rng& rng) {
  switch (id) {
  // --- rifles: player IV3 == 1, IV1 ---
  case 156: return sfx(MDAZ_CHOOSE(rng, "mosin_1", "mosin_2", "mosin_3"), 0.f, false, "");      // IV1 2
  case 158: return sil ? sfx("m4_silent_1", 0.f, true, "m4")                                     // IV1 3
                       : sfx(MDAZ_CHOOSE(rng, "m4_single_2", "m4_single_1"), 0.f, true, "m4");
  case 154: return sfx(MDAZ_CHOOSE(rng, "izh_1", "izh_2"), 0.f, true, "bm16");                   // IV1 4 bm16
  case 160: return sil ? sfx("AK_silent_1", 0.f, true, "m4") : sfx("Ak74_1", 0.f, true, "m4");  // IV1 5
  case 159: return sfx("akm_5", 0.f, true, "akm");                                               // IV1 6
  case 161: return sil ? sfx("AK_silent_2", 0.f, true, "m4") : sfx("Ak74_3", 0.f, true, "m4");  // IV1 7
  case 157: return sfx(MDAZ_CHOOSE(rng, "SKS_close_0", "SKS_close_1"), 0.f, true, "bm16");       // IV1 8
  case 162: return sil ? sfx("m4_silent_2", 0.f, true, "m4") : sfx("l85", 0.f, true, "m4");     // IV1 10
  case 155: return sfx(MDAZ_CHOOSE(rng, "remington_1", "remington_2"), 0.f, true, "bm16");       // IV1 11
  case 163: return sfx("Bow_1", 0.f, false, "");                                                 // IV1 13
  case 164: return sfx("Bow_1", 0.f, false, "");                                                 // IV1 14
  case 166: return sfx("sporter_22", 0.f, true, "m4");                                           // IV1 16
  case 167: return sfx("repeater", 0.f, true, "m4");                                             // IV1 17
  case 168: return sfx(MDAZ_CHOOSE(rng, "svd_single_0", "svd_single_1"), -10.f, false, "");      // IV1 18
  case 169: return sfx(MDAZ_CHOOSE(rng, "mp5k_single_0", "mp5k_single_1"), -5.f, true, "m4");    // IV1 19
  case 173: return sfx(MDAZ_CHOOSE(rng, "ump45_single_1", "ump45_single_2"), -5.f, true, "m4");  // IV1 21
  case 174: return sfx("akm_3", 0.f, true, "m4");                                                // IV1 22
  case 175: return sil ? sfx(MDAZ_CHOOSE(rng, "m4_silent_1", "m4_silent_2"), 0.f, true, "m4")    // IV1 23
                       : sfx(MDAZ_CHOOSE(rng, "fn_fal_single_1", "fn_fal_single_2"), -5.f, true, "m4");
  case 178: return sfx("saiga12", -5.f, true, "bm16");                                           // IV1 24
  case 177: return sil ? sfx("m4_silent_1", 0.f, true, "m4")                                     // IV1 25
                       : sfx(MDAZ_CHOOSE(rng, "steyraug_shot_0", "steyraug_shot_1"), 0.f, true, "m4");
  case 183: return sil ? sfx("m4_silent_1", 0.f, true, "m4") : sfx("bizon_shot_1", 0.f, true, "m4"); // IV1 26
  case 184: return sil ? sfx("m4_silent_1", 0.f, true, "m4")                                     // IV1 27
                       : sfx(MDAZ_CHOOSE(rng, "groza_shot_2", "groza_shot_1"), 0.f, true, "m4");
  case 185: return sil ? sfx("m4_silent_1", 0.f, true, "m4") : sfx("vss_shot_3", 0.f, true, "m4"); // IV1 28
  case 186: return sfx("sv98_shot_1", 0.f, false, "");                                           // IV1 29
  case 187: return sil ? sfx("m4_silent_1", 0.f, true, "m4") : sfx("madsen_shot", -5.f, true, "m4"); // IV1 30
  // --- pistols: player IV3 == 2, IV43 ---
  case 151: return sfx(MDAZ_CHOOSE(rng, "FNX_1", "FNX_2"), 0.f, true, "fnx");                    // IV43 1
  case 152: return sfx(MDAZ_CHOOSE(rng, "colt_1", "colt_2"), 0.f, true, "fnx");                  // IV43 2
  case 153: return sfx("magnum_single_1", 0.f, true, "bm16");                                    // IV43 3
  case 170: return sfx(MDAZ_CHOOSE(rng, "glock19_single_1", "glock19_single_2"), 0.f, true, "fnx"); // IV43 4
  case 165: return sfx(MDAZ_CHOOSE(rng, "amph_1", "amph_2"), 0.f, true, "m4");                   // IV43 5
  case 171: return sfx(MDAZ_CHOOSE(rng, "izh_1", "izh_2"), 0.f, true, "bm16");                   // IV43 6
  case 172: return sfx(MDAZ_CHOOSE(rng, "mosin_1", "mosin_2", "mosin_3"), 0.f, false, "");       // IV43 7
  case 176: return sfx(MDAZ_CHOOSE(rng, "colt_1", "colt_2"), 0.f, true, "fnx");                  // IV43 8
  case 179: return sfx(MDAZ_CHOOSE(rng, "mac10_1", "mac10_2"), 0.f, true, "m4");                 // IV43 9
  case 180: return sfx("repeater", 0.f, true, "m4");                                             // IV43 10
  case 181: return sfx("pm_shot", 0.f, true, "fnx");                                             // IV43 11
  case 182: return sfx(MDAZ_CHOOSE(rng, "pb_shot_1", "pb_shot_2"), 0.f, true, "fnx");            // IV43 12
  default: return none();
  }
}

C2Sfx c2_item_activate_sfx(ItemId id) {
  switch (id) {
  case 1: case 2: case 3: case 5: case 6: case 7: case 27: case 31: case 32: case 33:
  case 61: case 72: case 73: case 74: case 85: case 95: case 96: case 97:
    return sfx("EatingSoft_0", -10.f, true, "");
  case 4: return sfx("EatingCrunchy_0", -10.f, true, "");
  case 8: case 9: case 10: case 30: case 63:
    return sfx("DrinkSoda_0", -10.f, true, "");
  case 18: case 26: return sfx("bandage", -5.f, false, "");
  case 21: return sfx("gren pin", 0.f, false, "");
  case 24: return sfx("match_burn", 0.f, false, "");
  case 25: case 54: case 55: return sfx("Whiskey", -10.f, true, "");   // 54/55: drink water (IV > 0)
  case 29: return sfx("vitamins", -10.f, true, "");
  case 47: case 48: case 68: return sfx("planting_seeds", 0.f, false, "");
  case 51: case 52: return sfx("Bow_reload", -5.f, true, "");
  case 53: return sfx("action_refuel_0", -5.f, true, "");
  case 59: return sfx("action_repair_0", -5.f, true, "");
  case 60: case 71: case 91: case 92: case 93: case 94: return sfx("meat_cook", 0.f, false, "");
  case 62: return sfx("nuko", 0.f, false, "");
  case 76: case 77: case 78: case 79: case 80: case 81: case 83: case 89: case 90: case 98:
    return sfx("reload_pistol", -5.f, true, "");
  case 102: return sfx("Match_0", 0.f, false, "");
  default: return none();
  }
}

C2Sfx c2_item_activate2_sfx(ItemId id) {
  switch (id) {
  case 60: case 71: case 91: case 92: case 93: case 94: return sfx("EatingSoft_0", -10.f, true, "");
  case 20: return sfx("flare_throw", 10.f, true, "");
  case 54: case 55: return sfx("md_canteen_fill", 0.f, false, "");
  case 5: case 31: case 32: return sfx("planting_seeds", 0.f, false, "");
  default: return none();
  }
}

C2Sfx c2_pickup_sfx(ItemId id, bool wearable, Rng& rng) {
  const WeaponDef* w = find_weapon(id);
  if (w && w->stats.is_melee) return sfx(MDAZ_CHOOSE(rng, "pick1", "pick2"), -5.f, false, "");
  if (w && c2_is_pistol_item(id)) return sfx("pick_pistol", 0.f, false, "");
  if (w) return sfx("pick_rifle", 0.f, false, "");
  if (wearable) return sfx("gearpick", 0.f, false, "");
  return sfx(MDAZ_CHOOSE(rng, "pick1", "pick2"), -5.f, false, "");
}

C2Sfx c2_drop_sfx(bool gear, Rng& rng) {
  return gear ? sfx(MDAZ_CHOOSE(rng, "geardrop1", "geardrop2"), 0.f, false, "")
              : sfx(MDAZ_CHOOSE(rng, "drop1", "drop2"), 0.f, false, "");
}

C2Sfx c2_bandit_fire_sfx(int iv11, Rng& rng) {
  switch (iv11) {
  case 4: return sfx(MDAZ_CHOOSE(rng, "mosin_1", "mosin_2", "mosin_3"), 0.f, true, "m4");   // sid480134374344705
  case 5: return sfx("SKS_close_0", -5.f, true, "m4");                                      // sid297817646363859
  case 6: return sfx(MDAZ_CHOOSE(rng, "m4_single_2", "m4_single_1"), 0.f, true, "m4");      // sid962455108847833
  case 7: return sfx("Ak74_1", 0.f, true, "m4");                                            // sid186668153980230
  case 8: return sfx("akm_5", 25.f, true, "akm");                                           // sid372598292341732
  case 9: return sfx("Ak74_3", 0.f, true, "m4");                                            // sid306446140110898
  case 11: return sfx(MDAZ_CHOOSE(rng, "remington_1", "remington_2"), 0.f, true, "m4");     // sid293950420521776
  case 16: return sfx("sporter_22", 0.f, true, "m4");                                       // sid901848258504294
  case 17: return sfx("repeater", 0.f, true, "m4");                                         // sid918044114683645
  case 20: return sfx(MDAZ_CHOOSE(rng, "mp5k_single_0", "mp5k_single_1"), 0.f, true, "m4"); // sid242902325693189
  case 21: return sfx(MDAZ_CHOOSE(rng, "ump45_single_1", "ump45_single_2"), 0.f, true, "m4"); // sid133086041356979
  case 22: return sfx("bizon_shot_1", 0.f, true, "m4");                                     // sid734741495294438
  default: return none();
  }
}

const char* c2_footstep_sfx(int surface, Rng& rng) {
  switch (surface) {
  case 1: return MDAZ_CHOOSE(rng, "forest_run_1", "forest_run_2", "forest_run_3", "forest_run_4");
  case 2: return MDAZ_CHOOSE(rng, "sand_run_1", "sand_run_2", "sand_run_3", "sand_run_4");
  case 3: return MDAZ_CHOOSE(rng, "wood_1", "wood_2", "wood_3", "wood_4");
  case 4: return MDAZ_CHOOSE(rng, "concrete_run_1", "concrete_run_2", "concrete_run_3", "concrete_run_4");
  default: return MDAZ_CHOOSE(rng, "run_1", "run_2", "run_3", "run_4");
  }
}

const char* c2_choose_spotted(Rng& r) { return MDAZ_CHOOSE(r, "spotted_0", "spotted_1", "spotted_2", "spotted_3", "spotted_4", "spotted_5", "spotted_6"); }
const char* c2_choose_zed_attack(Rng& r) { return MDAZ_CHOOSE(r, "attack_0", "attack_1", "attack_10", "attack_11", "attack_12", "attack_13"); }
const char* c2_choose_body(Rng& r) { return MDAZ_CHOOSE(r, "body_1", "body_2"); }
const char* c2_choose_crow(Rng& r) { return MDAZ_CHOOSE(r, "crow_1", "crow_2", "crow_3", "crow_4", "crow_5"); }
const char* c2_choose_chop(Rng& r) { return MDAZ_CHOOSE(r, "chop", "chop2", "chop3", "chop4"); }
const char* c2_choose_treefall(Rng& r) { return MDAZ_CHOOSE(r, "treefall_02", "treefall_03"); }
const char* c2_choose_car_crash(Rng& r) { return MDAZ_CHOOSE(r, "car_crash_1", "car_crash_2", "car_crash_3"); }
const char* c2_choose_fence_destroy(Rng& r) { return MDAZ_CHOOSE(r, "Fence_destroy_1", "Fence_destroy_2"); }
const char* c2_choose_wolf_attack(Rng& r) { return MDAZ_CHOOSE(r, "wolf_attack", "wolf_attack_2", "wolf_attack_3"); }
const char* c2_choose_wolf_spot(Rng& r) { return MDAZ_CHOOSE(r, "wolf_spot", "wolf_spot_2"); }
const char* c2_choose_tank_death(Rng& r) { return MDAZ_CHOOSE(r, "zed_tank_1", "zed_tank_2", "zed_tank_3"); }
const char* c2_choose_punch(Rng& r) { return MDAZ_CHOOSE(r, "Punch_Swipes_0", "Punch_Swipes_1"); }

} // namespace mdaz

namespace mdaz {
namespace {
const char* pk(Rng& rng, const char* const* a, int n) { return a[rng.next_int(0, n - 1)]; }
C2Sfx mk(const char* n, float db, const char* tag) { C2Sfx s; s.name = n; s.db = db; s.at_object = true; s.tag = tag; return s; }
#define MDAZ_CH(rng, ...) \
  ([&]() { static const char* const k_[] = { __VA_ARGS__ }; \
           return pk(rng, k_, (int)(sizeof(k_) / sizeof(k_[0]))); }())
} // namespace

C2Sfx c2_teammate_fire_sfx(int iv14, Rng& rng) {
  switch (iv14) {
  case 4: return mk(MDAZ_CH(rng, "mosin_1", "mosin_2", "mosin_3"), 0.f, "m4");
  case 5: return mk("SKS_close_0", -5.f, "m4");
  case 6: return mk(MDAZ_CH(rng, "m4_single_2", "m4_single_1"), 0.f, "m4");
  case 7: return mk("Ak74_1", 0.f, "m4");
  case 8: return mk("akm_5", 25.f, "akm");
  case 9: return mk("Ak74_3", 0.f, "m4");
  case 10: return mk("l85", 0.f, "m4");
  case 11: return mk(MDAZ_CH(rng, "remington_1", "remington_2"), 0.f, "m4");
  case 15: return mk(MDAZ_CH(rng, "amph_1", "amph_2"), 0.f, "m4");
  case 16: return mk("sporter_22", 0.f, "m4");
  case 17: return mk("repeater", 0.f, "m4");
  case 19: return mk("pm_shot", 0.f, "fnx");
  case 20: return mk(MDAZ_CH(rng, "mp5k_single_0", "mp5k_single_1"), 0.f, "m4");
  case 21: return mk(MDAZ_CH(rng, "ump45_single_1", "ump45_single_2"), 0.f, "m4");
  case 22: return mk(MDAZ_CH(rng, "colt_1", "colt_2"), 0.f, "fnx");
  case 23: return mk("magnum_single_1", 0.f, "357");
  case 24: return mk(MDAZ_CH(rng, "izh_1", "izh_2"), 0.f, "bm16");
  default: { C2Sfx s; s.name = 0; s.db = 0.f; s.at_object = false; s.tag = ""; return s; }
  }
}

C2Sfx c2_friend_fire_sfx(int iv14, Rng& rng) {
  switch (iv14) {
  case 1: case 20: return mk(MDAZ_CH(rng, "FNX_1", "FNX_2"), 0.f, "fnx");
  case 2: return mk("magnum_single_1", 0.f, "357");
  case 3: return mk(MDAZ_CH(rng, "izh_1", "izh_2"), 0.f, "bm16");
  case 4: return mk(MDAZ_CH(rng, "mosin_1", "mosin_2", "mosin_3"), 0.f, "m4");
  case 5: return mk("SKS_close_0", -5.f, "m4");
  case 6: return mk(MDAZ_CH(rng, "m4_single_2", "m4_single_1"), 0.f, "m4");
  case 7: return mk("Ak74_1", 0.f, "m4");
  case 8: return mk("akm_5", 25.f, "akm");
  case 9: return mk("Ak74_3", 0.f, "m4");
  case 10: return mk("l85", 0.f, "m4");
  case 11: return mk(MDAZ_CH(rng, "remington_1", "remington_2"), 0.f, "m4");
  case 12: return mk(MDAZ_CH(rng, "colt_1", "colt_2"), 0.f, "fnx");
  case 15: return mk(MDAZ_CH(rng, "amph_1", "amph_2"), 0.f, "m4");
  case 16: return mk("sporter_22", 0.f, "m4");
  case 17: return mk("repeater", 0.f, "m4");
  case 19: return mk(MDAZ_CH(rng, "mp5k_single_0", "mp5k_single_1"), 0.f, "m4");
  default: { C2Sfx s; s.name = 0; s.db = 0.f; s.at_object = false; s.tag = ""; return s; }
  }
}

int c2_building_floor(int sub) {
  switch (sub) {
  case 9: case 10: case 11: case 12: case 13: case 14: case 18: case 24: case 31: case 32:
  case 33: case 40:
    return 1;
  case 2: case 3: case 4: case 5: case 7: case 8: case 15: case 16: case 17: case 19: case 20:
  case 21: case 25: case 26: case 27: case 28: case 29: case 30: case 34: case 35: case 36:
  case 38: case 39:
    return 2;
  default:
    return 0;
  }
}

const char* c2_day_loop_file(int lvl, bool erase) {
  return (erase ? lvl == 1 : lvl >= 2) ? "ambient" : "MD_winter_amb";
}
const char* c2_night_loop_file(int lvl, bool erase) {
  return (erase ? lvl == 1 : lvl >= 2) ? "night_loop" : "MD_Winter_night_amb";
}

} // namespace mdaz
