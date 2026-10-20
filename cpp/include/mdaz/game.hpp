#pragma once

#include "mdaz/animal.hpp"
#include "mdaz/audio.hpp"
#include "mdaz/bandit.hpp"
#include "mdaz/bullet.hpp"
#include "mdaz/bunker.hpp"
#include "mdaz/c2_session_globals.hpp"
#include "mdaz/campfire.hpp"
#include "mdaz/combat.hpp"
#include "mdaz/inventory.hpp"
#include "mdaz/inventory_ui.hpp"
#include "mdaz/item_db.hpp"
#include "mdaz/loot.hpp"
#include "mdaz/mapgen.hpp"
#include "mdaz/c2_mapgen.hpp"
#include "mdaz/cell_tilemaps.hpp"
#include "mdaz/menu_camera.hpp"
#include "mdaz/platform.hpp"
#include "mdaz/player.hpp"
#include "mdaz/quest.hpp"
#include "mdaz/char_anims.hpp"
#include "mdaz/layout_sprites.hpp"
#include "mdaz/world_tilemaps.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/save_game.hpp"
#include "mdaz/spawn_params.hpp"
#include "mdaz/timers.hpp"
#include "mdaz/types.hpp"
#include "mdaz/vehicle.hpp"
#include "mdaz/weather.hpp"
#include "mdaz/weapon_unlocks.hpp"
#include "mdaz/world.hpp"
#include "mdaz/zombie.hpp"
#include "mdaz/map_props.hpp"

namespace mdaz {
bool seg_hits_box(Vec2 a, Vec2 b, float x0, float y0, float x1, float y1);

struct CropSaveView;
struct C2Sfx;
struct TrapSaveView;
struct FireSaveView;

// Melee_2_auto 1:1 tables (game_chop.cpp).
int c2_melee_iv4(ItemId id);                   // player_collision_base IV4 for an equipped melee item
float c2_melee_reload(ItemId id);              // t909 IV4 Reloading_time_melee (0 = fist / not melee)
int c2_melee_swing_dmg(int iv4);               // melee_swing_general IV0
int c2_melee_crit_chance(int iv4, bool perk);  // round(random(100)) <= N, -1 = no crit event

class Game {
public:
  Game();
  ~Game();
  Game(const Game&) = delete;
  Game& operator=(const Game&) = delete;

  bool init(IPlatform* platform, const char* data_dir);
  void set_audio(IAudio* audio) { audio_ = audio; }

  void shutdown();
  void update(float dt);
  void render();
  // TEST HARNESS ONLY (desktop --shots-building): puts the player inside (mode 0) or just south
  // of (mode 1) the first police/other Solid building. Never called by gameplay code.
  bool harness_place_at_building(int mode);
  // Test-only (--shots-tutorial): placement and an item grant to reach later tutorial steps.
  // Test-only (--shots-death): kill a zed / fast zed / wolf / deer / rabbit / dog next to the player.
  void harness_death_demo();
  void harness_set_quest(int task, int status, int target) {  // test-only: quest panel screenshots
    quest_.q_task_id = task; quest_.q_status = status; quest_.q_target_sid = target;
  }
  void harness_set_pos(float x, float y) { player_.pos = Vec2(x, y); }
  // TEST HARNESS ONLY (--shots-chop): stand 25 px south of the nearest tree (sub 1..6) with a
  // hatchet (204) equipped. Returns the tree index or -1.
  // TEST HARNESS ONLY (MDAZ_HARNESS_TREES): stand 70 px north of the nearest tree_block of `sub`.
  int harness_place_at_tree_block(int sub) {
    int best = -1; float bd = 1e30f;
    for (int i = 0; i < tree_count_; ++i) {
      if (tree_sub_[i] != sub) continue;
      const float dx = trees_[i].x - player_.pos.x, dy = trees_[i].y - player_.pos.y;
      if (dx * dx + dy * dy < bd) { bd = dx * dx + dy * dy; best = i; }
    }
    if (best >= 0) player_.pos = Vec2(trees_[best].x, trees_[best].y - 70.f);
    return best;
  }
  Vec2 harness_player_pos() const { return player_.pos; }
  int harness_place_at_tree() {
    int best = -1; float bd = 1e30f;
    for (int i = 0; i < tree_count_; ++i) {
      if (tree_sub_[i] < 1 || tree_sub_[i] > 6) continue;
      const float dx = trees_[i].x - player_.pos.x, dy = trees_[i].y - player_.pos.y;
      if (dx * dx + dy * dy < bd) { bd = dx * dx + dy * dy; best = i; }
    }
    if (best < 0) return -1;
    player_.pos = Vec2(trees_[best].x, trees_[best].y + 25.f);
    harness_give_item(204, 1);
    for (int s = 0; s < inventory_.slot_count(); ++s) if (inventory_.slot(s).id == 204) { equipped_slot_ = s; break; }
    return best;
  }
  int harness_tree_hp(int i) const { return (i >= 0 && i < tree_count_) ? tree_hp_[i] : -1; }
  int harness_tree_count() const { return tree_count_; }
  int harness_inv_count(int id) const { int n = 0; for (int s = 0; s < inventory_.slot_count(); ++s) if (inventory_.slot(s).id == id) n += inventory_.slot(s).count; return n; }
  int harness_loot_count(int id) const;
  void harness_give_item(int id, int n) {   // frees the last used bag slot when the bag is full
    if (inventory_.add_item(static_cast<ItemId>(id), n, items_)) return;
    for (int i = inventory_.slot_count() - 1; i > 0; --i)
      if (!inventory_.slot(i).empty()) { inventory_.remove_at(i, inventory_.slot(i).count); break; }
    inventory_.add_item(static_cast<ItemId>(id), n, items_);
  }
  // TEST HARNESS ONLY (--shots-wreck): 0 = south of the first Humvee wreck, 1 = heli wreck,
  // 2 = Zed_check from 1600 px of a frame-0 zed_resp_point, then stand at the point.
  bool harness_place_at_wreck(int mode);
  // Test-only: stand 60 px east of the nearest waterpump (screenshot harness).
  bool harness_place_at_pump();
  // Bonds Guru studio intro (from jacksmith-psp): skipped by the --shots harnesses.
  void skip_intro();
  Vec2 harness_player_rel() const;  // test-only: player pos minus harness building hotspot
  bool wants_quit() const { return quit_; }

  AppState state() const { return state_; }
  const Player& player() const { return player_; }
  const Inventory& inventory() const { return inventory_; }
  const WeatherState& weather() const { return weather_; }
  const ItemDatabase& items() const { return items_; }
  const ZombieSystem& zombies() const { return zombies_; }
  const LootSystem& loot() const { return loot_; }
  const MapGenerator& mapgen() const { return mapgen_; }
  const BulletSystem& bullets() const { return bullets_; }
  const BunkerMap& bunker() const { return bunker_; }
  const QuestState& quest() const { return quest_; }
  bool in_bunker() const { return in_bunker_; }
  bool driving() const { return vehicles_.player_driving(); }
  bool show_bag() const { return show_bag_; }
  bool inventory_opened() const { return inventory_ui_.opened; }
  const InventoryUiState& inventory_ui_state() const { return inventory_ui_; }
  bool show_craft_menu() const { return show_craft_menu_; }
  bool show_controls() const { return show_controls_; }
  int craft_menu_index() const { return craft_menu_index_; }
  bool near_campfire() const { return campfires_.near_fire(player_.pos); }
  int equipped_slot() const { return equipped_slot_; }
  // Hotbar quick-slot selection: digit 1..8 picks directly, delta cycles (wraps 0..7);
  // an out-of-range current index is clamped to 0.
  static int next_hotbar_slot(int cur, int digit, int delta);
  static const int kQuickSlots = 8;
  float attack_cd() const { return attack_cd_; }
  float reload_cd() const { return reload_cd_; }
  float airdrop_alert() const { return airdrop_alert_; }
  float radio_t() const { return radio_t_; }
  float airdrop_eta() const { return timers_.airdrop; }
  float torch_battery() const { return torch_battery_; }
  float inv_full_flash() const { return inv_full_flash_; }
  int mag_rounds() const { return mag_rounds_; }
  int mag_capacity_equipped() const;
  float adrenaline_t() const { return adrenaline_t_; }
  float indoor_t() const { return indoor_t_; }
  float tent_warmth_t() const { return tent_warmth_t_; }
  float wetness() const { return wetness_; }
  float gasmask_filter() const { return gasmask_filter_; }
  bool gun_jammed() const { return gun_jammed_; }
  bool wearing_gasmask() const;
  bool near_choppable_tree() const;
  bool crouching() const { return crouching_; }
  bool near_vehicle() const;
  Vec2 last_airdrop_pos() const { return last_airdrop_pos_; }
  const Vehicle* driven_vehicle() const { return vehicles_.driven(); }
  int ammo_for_equipped() const;
  Difficulty menu_difficulty() const { return menu_difficulty_; }
  bool sector_visited(int sx, int sy) const;
  const C2SessionGlobals& c2_session() const { return c2_; }
  float weapon_unlock_toast_t() const { return wpn_unlock_toast_t_; }
  ItemId weapon_unlock_toast_weapon() const { return wpn_unlock_toast_id_; }
  // Smoothed frames-per-second (updated every ~0.5s from Game::update dt).
  float fps() const { return fps_; }
  // PSP UI (focus navigation, no pointer).
  float ui_time() const { return ui_t_; }
  int menu_focus() const { return menu_focus_; }
  int pause_focus() const { return pause_focus_; }
  const char* toast_text() const { return toast_t_ > 0.f ? toast_ : 0; }
  // Short label for the Circle/interact action available right now (0 = none).
  const char* interact_hint() const;
  bool has_save() const;
  bool half_res() const { return half_res_; }
  bool perf_overlay_on() const { return perf_overlay_; }

  Player& player_mut() { return player_; }
  Inventory& inventory_mut() { return inventory_; }
  void set_state(AppState s) { state_ = s; }
  void start_new_game(Difficulty d);
  // C2 layout "Tutorial" (Tutorial_events): started on first launch until the localStorage key
  // Tutorial_map_finished = "1" (settings.cfg tutorial_finished=1).
  void start_tutorial();
  bool tutorial_active() const { return tut_.active; }
  bool tutorial_finished() const { return tutorial_finished_; }
  void set_tutorial_finished(bool f) { tutorial_finished_ = f; }
  // Read-only tutorial view for HUD/tests.
  struct TutorialView {
    bool tip_visible; bool tip_two; int tip_text_id; int tip_id; bool camera_paused; bool camera_rolling;
    int ui_step, inv_step, perk_step, bleed_step; int final_step; bool menu_visible;
  };
  TutorialView tutorial_view() const;

private:
  friend struct GameTestAccess; // tests/test_selection.cpp
  // ---- C2 Tutorial (src/core/game_tutorial.cpp) ----
  struct TutorialRun {
    bool active;
    float t;                 // seconds since layout start (real time)
    int start_phase;         // layout-start Wait chain
    // C2 globals (Tutorial_events / Game_events)
    int moveblock, blocked_gui, onscreen;            // Tutor_hint_moveblock / _blocked_gui / _onscreen
    int ui_step, inv_step, perk_step, bleed_step;    // tutor_*_step
    int death;                                       // tutor_death
    bool found[12];                                  // Tutor_*_found (camera_roll ids 1..11)
    bool firearm_tip_done;                           // trigger once (Firearm_guide)
    bool encounter_on; float encounter_t;            // group Tutorial_encounter, every 1 s
    float bleed_t;                                   // Bleeding_guide every 1 s
    int experience_points;                           // expierence_points (tip 10 sets 316)
    int die_count;                                   // tutorDieCount
    // tip (gui_tutor_tip_bg + text t647 + fade_sprite + walk_marker)
    bool tip_visible; bool tip_two; bool tip_top; int tip_text; int tip_id;
    float dark;              // fade_sprite opacity 0..1
    int highlight;           // walk_marker target: 0 none, 1 interact, 2 inventory, 3 attack, 4 reload, 5 switch, 6 perks, 7 blood bar
    // camera_roll / camera_roll_back
    int roll_id; int roll_phase;   // 0 none, 1 to target, 2 paused at target, 3 back to player
    Vec2 cam; Vec2 roll_target; int roll_kind; int roll_index;
    float timescale;
    bool zeds_blind;
    // pl12 zones (IV0 1 perks, 2 final, 3 barrier)
    bool zone_alive[4]; Vec2 zone_pos[4]; Vec2 zone_size[4];
    // b_exit (boat) Level_menu
    bool menu_visible; int menu_kind;   // 1 = need wood (42), 3 = repair (46)
    int boat_frame;
    // Tutorial_script_final
    int final_step; float final_t; Vec2 bot[2]; int bot_anim[2]; float bot_face[2];
    int say_id; Vec2 say_pos; float say_t; float fade; int shots;
    bool zed_hit_prev;
  };
  TutorialRun tut_;
  bool tutorial_finished_;
  float tut_saved_aggro_[128];
  void update_tutorial(float dt, const InputState& in);
  void tut_show_tip(int p0, int id, int p2);
  void tut_close_tip();
  void tut_camera_roll(int id, Vec2 target, int kind, int index);
  void tut_camera_roll_back();
  void tut_set_zeds_blind(bool blind);
  void tut_update_final(float dt);
  void tut_start_final();
  bool tut_los(Vec2 a) const;
  Vec2 render_cam() const;
  void render_tutorial_world_top(float cx, float cy, float sc) const;
  void render_tutorial_overlay() const;
  void tut_leave_to_menu();
  // C2 trees: IV0 hit points (2 for tree types 1..6) -> fall at 0 (2x Wood sticks + 1 Wood piles).
  unsigned char tree_hp_[4096];     // tree IV0 (layout default 2)
  unsigned char tree_frame_[4096];  // tree anim frame: 0 standing, 1 stump (obst_base 10 -> 8)

  IPlatform* platform_;
  IAudio* audio_;
  AppState state_;
  bool quit_;
  Player player_;
  Inventory inventory_;
  ItemDatabase items_;
  World world_;
  WeatherState weather_;
  ZombieSystem zombies_;
  AnimalSystem animals_;
  BanditSystem bandits_;
  TeamCampSystem team_camps_;
  ZedRespawnChannels zed_resp_;
  VehicleSystem vehicles_;
  CampfireSystem campfires_;
  QuestState quest_;
  WorldTimers timers_;
  LootSystem loot_;
  MapGenerator mapgen_;
  MapProps c2_props_;  // create_map_element cats 3/4/5/6 + deer stands
  BulletSystem bullets_;
  BunkerMap bunker_;
  WorldTilemaps world_tiles_;
  CellTilemaps cell_tiles_;  // C2 create_map_location per-cell tilemaps
  WorldTilemaps menu_tiles_;
  LayoutSprites menu_sprites_;
  MenuCamera menu_cam_;
  CharAnims char_anims_;
  DispersionState dispersion_;
  Rng rng_;
  float state_timer_;
  double intro_t0_;
  void draw_intro(float progress);
  float attack_cd_;
  float reload_cd_;
  float player_anim_t_;   // locomotion clock
  float melee_anim_t_;    // >0 while playing axe/melee swing
  float fire_anim_t_;     // brief pistol/twohand pose after shooting
  float use_anim_t_;      // >0 while playing using_item
  float berry_t_ = 0.f;   // C2 bush search wait (3 s, IV21=1)
  int berry_idx_ = -1;
  int trunk_avail_ = -1;     // avalible_interaction = 3 target (gui_btn_interact frame 3)
  int trunk_walk_ = -1;      // C2 walk_marker: target car_loot_point (stage IV0 7 / 4)
  int trunk_walk_stage_ = 0; // walk_marker IV0: 7 = to wp1/wp2, 4 = to the point
  Vec2 trunk_walk_tgt_;      // walk_marker position
  bool trunk_walk_drive_ = false;  // AltMove SetSpeed 100 this tick
  bool tut_car_trunk_opened_ = false;  // C2 global tutorCarTrunkOpened
  int ach_gta_ = 0;          // C2 ach_gta IV2 (trunks opened, SEED_RUN = 0)
  bool try_trunk_interact();
  // ---- C2 waterpump (t435) + avalible_interaction (loot_checker2 sid1430881814617229) ----
  static const int kPumpDef = 3;          // kPropDefs index of the waterpump
  short pump_iv0_[6000];                  // waterpump IV0 (water units) per c2_props_ index
  int avail_interaction_ = 0;             // global avalible_interaction (gui_btn_interact frame)
  float avail_t_ = 0.f;                   // Every 0.5 s
  int avail_pump_ = -1;                   // nearest pump picked for frame 4
  int pump_walk_ = -1;                    // walk_marker IV0 = 5 target pump
  float pump_lock_t_ = 0.f;               // player_collision_base IV21 = 1 for Wait 2
  float pump_add_water_ = 0.f;            // IV14 added after the Wait
  void init_pump_water(Rng& rng, bool tutorial);
  void update_avalible_interaction(float dt);
  int nearest_pump(Vec2 p, float& d2) const;
  bool pump_overlaps_player(int k) const;
  void pump_drink(int k);
  // ---- Item 5: chopping (Hitting_obstacles sid5202978245239527 + walk_marker IV0 = 8) ----
  struct MeleeSwing { Vec2 pos; float ang; float life; float dmg; ItemId weapon; bool chop; bool crit; bool alive; };
  int melee_hits_ = 0;               // swings that hit an NPC (test counter)
  void swing_hard_ground(Vec2 at);
  static const int kMaxSwings = 4;
  MeleeSwing swings_[kMaxSwings];
  int chop_walk_ = -1;              // walk_marker IV0 = 8 target tree
  Vec2 chop_walk_tgt_;
  int ach_lumberjack_ = 0;          // ach_lumberjack IV2 (SEED_RUN = 0)
  int tutor_tree_chopped_ = 0;      // global tutorTreeChopped
  void reset_chop_state();
  bool tree_obst_contains(int ti, float px, float py) const;  // obst_base frame 10 / 8 poly
  Vec2 tree_object_pos(int ti) const;                         // tree sprite position (hotspot)
  int nearest_tree_obst(Vec2 p, float& d2) const;             // PickDistance obst_base (trees)
  void spawn_melee_swing(float ang_deg, bool chop, float dmg, bool crit, ItemId weapon);
  void update_melee_swings(float dt);
  void chop_tree_hit(int ti, Vec2 at);
  bool start_chop_walk();
  bool tick_chop_walk(InputState& in);
  float pump_respawn_start_t_ = -1.f;     // Start events sid6396414627891676: Wait 0.1 -> Global_respawn
  void global_respawn_pumps();            // LOOT Respawn sid703496838047803 (pump part)
  void tick_pump_respawn_start(float dt);
public:
  int avalible_interaction() const { return avail_interaction_; }
  int pump_water(int k) const { return (k >= 0 && k < 6000) ? pump_iv0_[k] : 0; }
  void set_pump_water_for_test(int k, int v) { if (k >= 0 && k < 6000) pump_iv0_[k] = (short)v; }
  int first_pump_for_test() const;
  void pump_drink_for_test(int k) { pump_drink(k); }
  void tick_pump_for_test(float dt);
  void global_respawn_pumps_for_test() { global_respawn_pumps(); }
  // Item 5 test accessors.
  int tree_count_for_test() const { return tree_count_; }
  bool blocks_for_test(Vec2 p) const { return building_blocks(p, p); }
  bool tree_above_for_test(int i) const { return tree_above_player(i); }
  int tree_sub_for_test(int i) const { return tree_sub_[i]; }
  int tree_iv0_for_test(int i) const { return tree_hp_[i]; }
  int tree_frame_for_test(int i) const { return tree_frame_[i]; }
  Vec2 tree_trunk_for_test(int i) const { return trees_[i]; }
  Vec2 tree_pos_for_test(int i) const { return tree_object_pos(i); }
  void swing_for_test(float ang_deg, bool chop) { spawn_melee_swing(ang_deg, chop, chop ? 50.f : 10.f, false, 0); }
  Zombie* spawn_zed_for_test(Vec2 p) { return zombies_.spawn(Zed_Normal, p); }
  void clear_zeds_for_test() { for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) z->alive = false; }
  void melee_attack_for_test() { try_melee_attack(); }
  bool unequip_for_test() { for (int s = 0; s < inventory_.slot_count(); ++s) if (inventory_.slot(s).empty()) { equipped_slot_ = s; return true; } return false; }
  int melee_hits_for_test() const { return melee_hits_; }
  float swing_dmg_for_test(int i) const { return swings_[i].alive ? swings_[i].dmg : -1.f; }
  void tick_swings_for_test(float dt) { update_melee_swings(dt); }
  int ach_lumberjack_for_test() const { return ach_lumberjack_; }
  bool start_chop_for_test() { return start_chop_walk(); }
  bool tick_chop_for_test(InputState& in) { return tick_chop_walk(in); }
  bool tree_blocks_for_test(int ti, Vec2 p) const { return tree_obst_contains(ti, p.x, p.y); }
  void equip_for_test(int id) { harness_give_item(id, 1); for (int s = 0; s < inventory_.slot_count(); ++s) if (inventory_.slot(s).id == id) { equipped_slot_ = s; break; } }
  int ground_count_for_test(int id) const { return harness_loot_count(id); }
  float attack_cd_for_test() const { return attack_cd_; }
  void set_attack_cd_for_test(float v) { attack_cd_ = v; }
  int pump_count_for_test() const;
  const MapProp& prop_for_test(int i) const { return c2_props_.at(i); }
  int pump_index_for_test(int n) const;
private:
  bool doorchecker_blocked(Vec2 a, Vec2 b) const;
public:
  bool trunk_available() const { return trunk_avail_ >= 0; }
private:
  void open_trunk(int k, bool walked);
  int spawn_drop_count(ItemId id);
public:
  // TEST HARNESS ONLY: nearest live car trunk / stand on it / count live trunks.
  int harness_trunk_near(float& d);
  bool harness_place_at_trunk(bool on_point);
  // Test-only (--shots-carhit): player 20 px south of the first car of create_map_element sub
  // (IV1 = 6); returns the car's polygon bottom y (or -1). debug_hitbox_ draws Solid polygons.
  float harness_place_at_car(int sub);
  bool debug_hitbox_ = false;
  float harness_player_y() const { return player_.pos.y; }
  int trunk_opened_ = 0, trunk_found_ = 0;   // test counters
  int ach_gta() const { return ach_gta_; }
  int harness_prop_frame(int k) const { return c2_props_.at(k).frame; }
  bool harness_trunk_opened(int k) const { return c2_props_.trunk_opened(k); }
  int harness_trunk_walk_stage() const { return trunk_walk_ >= 0 ? trunk_walk_stage_ : 0; }
  int harness_trunks_alive() const { int n = 0; for (int i = 0; i < c2_props_.count(); ++i) if (c2_props_.trunk_alive(i)) ++n; return n; }
private:
  float airdrop_alert_;
  float radio_t_; // seconds to show radio HUD after use
  float torch_battery_; // 0..100 — drains while torch/NVG on; Battery (22) refills
  float inv_full_flash_; // HUD flash when pickup fails
  float tent_warmth_t_;  // remaining shelter bonus seconds after placing tent
  float indoor_t_;       // POI interior search warmth timer
  float footstep_t_;     // cadence timer for run/walk SFX
  float wetness_;        // 0..100 clothes wetness (rain up / fire dry)
  float gasmask_filter_; // 0..100 filter life while wearing gasmask
  float adrenaline_t_;   // legacy placeholder — no C2 item_activate for morphine (45)
  float energy_sprint_t_; // C2 item_activate id63: EightDir max speed boost ~10s
  float vitamins_buff_t_; // C2 item_activate id29: IV32=1 for 60s
  float item_use_lock_t_; // C2 item_activate IV21 using-item wait (blocks actions)
  float melee_break_mul_; // duct tape reinforce reduces melee break chance
  bool has_headlamp_;    // worn/owned headlamp enables torch toggle without kit
  bool gun_jammed_;      // clear with V reload
  int mag_rounds_;       // rounds in current weapon chamber/mag
  ItemId mag_weapon_id_; // weapon the mag belongs to
  int equipped_slot_;
  bool in_bunker_;
  int harness_loc_ = -1;
  bool bunker_looted_;
  float bunker_reopen_t_; // C2 Bunker_erase → Reopen after 1800s
  bool show_bag_;
  InventoryUiState inventory_ui_;
  bool show_craft_menu_;
  bool show_controls_; // READ ONLY controls help (menu / pause)
  int craft_menu_index_;
  Difficulty menu_difficulty_;
  void draw_controls_help() const;
  // --- PSP UI state ---
  int menu_focus_;
  int pause_focus_;
  float ui_t_;
  float nav_hold_t_[4];
  bool nav_prev_[4];
  char toast_[48];
  float toast_t_;
  void compute_nav(InputState& in, float dt);
  void show_toast(const char* msg);
  void update_pause(float dt, const InputState& in);
  void update_inventory_ui(const InputState& in);
  void update_craft_menu(const InputState& in);
  bool load_game_from_disk();
  void save_game_to_disk();
  void load_settings();
  void save_settings();
  void toggle_half_res();
  void toggle_perf_overlay();
  bool perf_overlay_;          // settings.cfg perf_overlay (PSP only)
  mutable int has_save_cache_; // -1 unknown, 0/1 cached has_save()
  bool half_res_;              // settings.cfg half_res (PSP only)
  void use_equipped_item();
  void use_bag_slot(int slot);
  void equip_bag_slot(int slot);
  void try_drop_slot(int slot);
  void render_main_menu() const;
  void render_pause_menu() const;
  void render_dead_screen() const;
  Vec2 overworld_pos_;
  Vec2 last_airdrop_pos_;
  bool visited_sectors_[kMapSectorsX * kMapSectorsY];
  bool prev_scream_;
  char data_dir_[512];

  static const int kMaxTraps = 12;
  struct PlaceTrap {
    Vec2 pos;
    float life;     // seconds remaining
    int kind;       // 0=bear, 1=barbed, 2/5/6/7=fence (Craft_32/31), 3=claymore, 4=landmine
    bool active;
    bool sprung;    // bear trap already triggered
  };
  PlaceTrap traps_[kMaxTraps];
  int trap_count_;

  // Delayed crop plots (seed → grow → harvest)
  static const int kMaxCrops = 16;
  struct CropPlot {
    Vec2 pos;
    ItemId crop_id;   // result item when ready
    float grow_t;     // seconds remaining until ready
    bool ready;
    bool active;
  };
  CropPlot crops_[kMaxCrops];
  int crop_count_;
  void update_crops(float dt);
  bool try_plant_seed(ItemId seed_id);
  float howl_cd_;
  float cough_cd_;
  // C2 create_map_element category 1 (array-518 rows of generate_location): trunk point
  // (obst_base / big_obstacle_base at the tree's Imagepoint 1) + subtype 1..10.
  static const int kMaxTrees = 4096;
  Vec2 trees_[kMaxTrees];
  unsigned char tree_sub_[kMaxTrees];
  int tree_count_;
  // generate_location forest stamps (tile 0 cells) + map_element-sheet0 when exported sprite exists.
  static const int kMaxMapProps = 64;
  Vec2 map_props_[kMaxMapProps];
  int map_prop_count_;
  static const int kMaxBloodDrips = 48;
  struct BloodDrip { Vec2 pos; float life; };
  BloodDrip blood_drips_[kMaxBloodDrips];
  int blood_drip_count_;
  float bleed_drip_cd_;
  bool crouching_;
  float fuel_warn_cd_;
  float rain_sfx_cd_;
  float lightning_t_; // brief white flash after thunder
  float hit_effect_t_; // C2 Hit_effect layout flash after Player_get_hit
  DmgHintSystem dmg_hints_; // C2 dmg_hint floating "-N" text
  float thunder_cd_;
  float cough_alert_t_; // brief window after cough that pulls zeds
  float fps_;
  float fps_accum_;
  int fps_frames_;
  float wind_sfx_cd_;
  float flare_t_;
  Vec2 flare_pos_;
  float molotov_dmg_cd_;
  float molotov_reload_t_; // C2 molotov_reload timer 15s → player_weapons IV34
  float vss_bleed_unlock_cd_;
  WeaponUnlockState wpn_unlocks_;
  float wpn_unlock_toast_t_;
  ItemId wpn_unlock_toast_id_;
  C2SessionGlobals c2_;

  // Visual FX / corpses (presentation parity with C2)
  enum FxKind { Fx_Muzzle = 0, Fx_Hit, Fx_Explode, Fx_Shell, Fx_Blood, Fx_Fish };
  static const int kMaxFx = 32;
  struct FxSprite { Vec2 pos; float life; float angle; FxKind kind; bool active; };
  FxSprite fx_[kMaxFx];
  int fx_count_;

  enum CorpseKind { Corpse_Zed = 0, Corpse_Bandit, Corpse_Deer, Corpse_Wolf, Corpse_Dog, Corpse_Other };
  static const int kMaxCorpses = 48;
  struct Corpse {
    Vec2 pos;
    float life;
    float angle;
    CorpseKind kind;
    int variant;
    bool active;
    float die_anim_t;     // seconds left playing die1/die2 via char_anims
    int die_style;        // 1 or 2 (C2 random die pick)
    char anim_base[32];   // e.g. zed_normal_skin1, wolf_skin
  };
  Corpse corpses_[kMaxCorpses];
  int corpse_count_;

  void spawn_fx(FxKind kind, Vec2 pos, float angle_deg = 0.f, float life = 0.12f);
  void spawn_corpse(CorpseKind kind, Vec2 pos, float angle_deg, int variant = 0,
                    const char* anim_base = 0, float die_anim_sec = 0.f, int die_style = 1);
  void update_fx(float dt);

  void log_helicrash_coords();
  bool npc_forest_block(Vec2& pos, Vec2 prev) const;
  ItemId equipped_id_or_zero() const;
  static const char* zed_skin_base(ZedType t);
  LootSaveView load_loot_view_; // scratch for load (96 piles, kept off the stack)
  void mark_sector_visited();
  void update_menu(float dt, const InputState& in);
  void update_playing(float dt, const InputState& in);
  void update_dead(float dt, const InputState& in);
  void try_melee_attack();
  void try_ranged_attack();
  void try_pickup_loot();
  void try_craft();
  void toggle_craft_menu();
  void try_craft_menu_recipe();
  void try_place_campfire();
  bool try_place_trap(int kind);
  // build_fence1 (Craft_32) / build_fence2 (Craft_31): both take 5 of the stick stack (38)
  // and clear the woodpile (50) slot, then Building_mode(1) -> obst_barricade.
  bool try_build_fence(int variant);
  void try_cancel_quest();
  void on_quest_completed(int task_before);
  void update_traps(float dt);
  void try_drop_equipped();
  void try_mag_reload();
  void sync_mag_for_equipped();
  void try_throw_projectile();
  void try_bunker_enter_exit();
  void spawn_bunker_zeds(Vec2 at);   // C2 Bunker_spawn_zeds(x, y)
  void drop_gear_and_overflow(ItemId old);
  bool bullet_hits_obstacle(Vec2 a, Vec2 b) const;
  void install_los();
public:
  bool los_blocked_world(Vec2 a, Vec2 b) const; // buildings + big obstacles (C2 LOS)
private:
  bool building_blocks(Vec2 a, Vec2 b) const; // C2 *_int Solid polygons
  bool animal_swing_hit(int ai, Vec2 a, Vec2 b);
  bool tree_above_player(int ti) const;        // Obstacles_transparenting layer swap
  bool player_overlaps_prop(const void* prop, Vec2 o) const;
  bool location_above_player(int li, const char* sheet, Vec2 o, bool& roof_fade);
  unsigned char car_fg_[512];                  // t920 layer state (1 = Buildings), per location
  bool car_layer_tick_ = false;
  void update_house_layers(float dt);
  unsigned char house_roof_[512] = {0};        // t928 opacity 25 (Y < Imagepoint 1)
  unsigned char house_fg_[512] = {0};          // t928 on Buildings (1) / Buildings_bg (0)
  bool house_tick_ = false, house_any_ov_ = false;
  float house_t_ = 0.f;
  bool iv41_ = false;                          // player_collision_base IV41 -> t943 Outline                // Every 0.5 s (House_transparenting t920 rule)
  float car_layer_t_ = 0.f;
public:
  bool iv41_for_test() const { return iv41_; }
  void house_layers_tick_for_test() { house_any_ov_ = true; update_house_layers(0.1f); }
  bool location_above_for_test(int li, const char* sheet, Vec2 o, bool& roof) { car_layer_tick_ = true; house_tick_ = true; return location_above_player(li, sheet, o, roof); }
private:
  void zed_check_bunker();           // C2 Zed_check_bunker (every 3 s while inBunker)
  bool bunker_spawner_used_[32];
  float bunker_zed_check_t_;
  float molotov_tick_t_;
  void close_inventory();
  void sync_bag_overlay_from_ui();
  void resolve_bullet_hits();
  void explode_at(Vec2 pos, float damage, bool leave_fire = false);
  void tick_molotov_burn(float dt);
  void on_animal_killed(Animal& a);
  void on_zed_killed(Zombie& z, ItemId killer_weapon = 0);
  void notify_weapon_unlock(ItemId unlocked_loot_id);
  void hurt_zombie(Zombie& z, float amount, ItemId killer_weapon = 0);
  void hurt_zombie_bullet(Zombie& z, float amount, bool armor_pierce_iv8, ItemId killer_weapon = 0);
  void hurt_animal(Animal& a, float amount);
  void hurt_bandit(Bandit& b, float amount);
  void apply_incoming_damage(float amount, bool can_bleed,
                             PlayerDamageKind kind = PlayerDmg_Default,
                             const char* death_if_lethal = 0,
                             MeleeDefPercentTier melee_def_tier = MeleeDef_Heavy,
                             bool try_melee_def = false);
  void write_death_log();
  void spawn_poi_loot();
  void spawn_loot_for_poi(MapLocation& L);
  void spawn_hostiles_for_poi(const MapLocation& L);
  // C2 NPC_manager "Zed_check": cull far NPCs, then one random zed_resp_point in (1500, respawn_radius_npc].
  void zed_check();
  void zr_fill_point(int pi);
  bool zr_spawn_one(int pi, int kind, bool keep, float spread, bool choose_spread);
  short zr_iv0_[C2Map::kMaxZr];
  bool zr_iv1_[C2Map::kMaxZr];
  void sync_location_stream(LocationReloadMode mode);
  void maybe_spawn_quest_bandits();
  int bandits_alive_near(Vec2 pos, float radius) const;
  Vec2 location_reload_anchor_;
  float location_reload_t_;
  bool quest_bandits_spawned_;
  bool secret_spawn3_done_;
  int hammerspawned_; // C2 global cap 5
  int helispawned_;   // C2 instance cap 3 (helicrash POI budget)
  void maybe_add_hammer_loot(LootPile* pile);
  void update_army_shooters(float dt);

  // ---- C2 audio (game_audio.cpp, docs/AUDIO_AUDIT.md) ----
  struct PendingSfx {   // System Wait follow-up of an Audio action
    float t; const char* name; float db; bool positioned, loop; Vec2 pos;
    int follow;         // 0 fixed pos, 1 player, 100+i vehicle i
    char tag[32];
  };
  static const int kMaxPendingSfx = 8;
  PendingSfx pending_sfx_[kMaxPendingSfx] = {};
  Rng sfx_rng_;                    // choose() for sounds; keeps gameplay rng_ sequences intact
  bool snd_day_trig_ = false, snd_night_trig_ = false, snd_first_tick_ = false;
  bool snd_rain_on_ = false;
  float snd_roof_t_ = 0.f;
  bool snd_fire_on_[16] = {};
  bool snd_flare_on_ = false;
  int snd_car_idx_ = -1;
  bool snd_car_broken_ = false;
  bool snd_crash_latch_ = false;   // car collision edge (car_crash_* once per crash)
  Vec2 snd_car_prev_pos_;
  float snd_step_t_ = 0.f, snd_scream_t_ = 0.f, snd_buried_t_ = 0.f;
  AppState snd_prev_state_ = AppState_Loading;
  void sfx2d(const char* name, float db, const char* tag = "");
  void sfx_at(const char* name, float db, Vec2 at, const char* tag = "");
  void sfx_play(const C2Sfx& s, Vec2 at);
  void sfx_later(float delay, const char* name, float db, bool positioned, Vec2 at, int follow,
                 bool loop, const char* tag);
  void audio_layout_start();
  void audio_menu_start();
  void audio_bunker_enter();
  void audio_bunker_exit();
  bool audio_player_in_warmzone(int* floor) const;
  bool audio_player_in_house_area() const;
  void audio_footsteps(float dt, bool moving);
  void update_audio(float dt);
  void update_bandit_guns(float dt);
  void pack_crops_view(CropSaveView& out) const;
  void apply_crops_view(const CropSaveView& in);
  void pack_traps_view(TrapSaveView& out) const;
  void apply_traps_view(const TrapSaveView& in);
  void pack_fires_view(FireSaveView& out) const;
  void apply_fires_view(const FireSaveView& in);
  void pack_loot_view(LootSaveView& out) const;
  void apply_loot_view(const LootSaveView& in);
};

} // namespace mdaz
