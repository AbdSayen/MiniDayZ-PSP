#pragma once
// C2 Audio action tables extracted from data.js (audit: docs/AUDIO_AUDIT.md).
// Every entry cites the data.js event sid it reproduces.
#include "mdaz/types.hpp"

namespace mdaz {

class Rng;

struct C2Sfx {
  const char* name;  // media file stem (null = the original plays nothing here)
  float db;
  bool at_object;    // PlayAtObject(ByName): positioned at the object; else PlayByName
  const char* tag;   // "" when the original passes an empty tag
};

// Group "Firearm shooting": player rifle (IV3==1 keyed by IV1) / pistol (IV3==2 keyed by IV43).
// silenced = player_weapons IV17 > 0 (silencer attached).
C2Sfx c2_player_fire_sfx(ItemId weapon_id, bool silenced, Rng& rng);
// Function item_activate (param0 = item id): food / drink / medicine / tools.
C2Sfx c2_item_activate_sfx(ItemId item_id);
// Function item_activate2 (canteen / bottle fill = md_canteen_fill, cooked meat, seeds, flare).
C2Sfx c2_item_activate2_sfx(ItemId item_id);
// Pick_player_item / Check_space_*: melee pick1/2 -5, pistol pick_pistol, rifle pick_rifle,
// wearables gearpick, ordinary items pick1/2 -5.
C2Sfx c2_pickup_sfx(ItemId item_id, bool wearable, Rng& rng);
// Drop_slot / Drop_firearm / Drop_pistol / Drop_melee / Drop_helmet: drop1/2 0 dB;
// Drop_outerwear / pants / vest / backpack: geardrop1/2 0 dB.
C2Sfx c2_drop_sfx(bool gear, Rng& rng);
// Banditmate > Bot Bandit behavior (t827 IV11) gunshots; name null for an IV11 with no event.
C2Sfx c2_bandit_fire_sfx(int weapon_iv11, Rng& rng);
// Animations: footsteps every 0.25 s while moving, tag "run", -5 dB at the player.
// surface: 0 = run_1..4 (else), 1 = forest_run (forest tile), 2 = sand_run (t411),
// 3 = wood_1..4 (house floor warmzone IV1==1), 4 = concrete_run (warmzone IV1==2).
const char* c2_footstep_sfx(int surface, Rng& rng);
// Pools used by several events.
const char* c2_choose_spotted(Rng& rng);     // spotted_0..6
const char* c2_choose_zed_attack(Rng& rng);  // attack_0,1,10,11,12,13
const char* c2_choose_body(Rng& rng);        // body_1/2
const char* c2_choose_crow(Rng& rng);        // crow_1..5
const char* c2_choose_chop(Rng& rng);        // chop, chop2..4
const char* c2_choose_treefall(Rng& rng);    // treefall_02/03
const char* c2_choose_car_crash(Rng& rng);   // car_crash_1..3
const char* c2_choose_fence_destroy(Rng& rng);
const char* c2_choose_wolf_attack(Rng& rng); // wolf_attack, _2, _3
const char* c2_choose_wolf_spot(Rng& rng);   // wolf_spot, _2
const char* c2_choose_tank_death(Rng& rng);  // zed_tank_1..3
const char* c2_choose_punch(Rng& rng);       // Punch_Swipes_0/1

bool c2_is_pistol_item(ItemId weapon_id);

} // namespace mdaz

namespace mdaz {
// Teammate / Veteran friend bots (t549 / t522 IV14) gunshots (sid805634705794306 ...).
C2Sfx c2_teammate_fire_sfx(int weapon_iv14, Rng& rng);
// warmzone IV1 per create_map_element building subtype (map_element IV2): 1 = wood floor,
// 2 = concrete floor, 0 = no warmzone IV1 set (no indoor footstep event matches).
int c2_building_floor(int map_element_iv2);
// Veteran friend > Bot_Behavior3 (t522 IV14) gunshots.
C2Sfx c2_friend_fire_sfx(int weapon_iv14, Rng& rng);
// Day&Night (sid650766536843728 / sid607253053633264): CurrentLevel >= 2 ? normal : winter.
// Bunker_erase (sid172106691563512 / sid427082255021896): CurrentLevel == 1 ? normal : winter.
const char* c2_day_loop_file(int current_level, bool from_bunker_erase);
const char* c2_night_loop_file(int current_level, bool from_bunker_erase);
} // namespace mdaz
