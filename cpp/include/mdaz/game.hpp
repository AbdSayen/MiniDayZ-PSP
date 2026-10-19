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

struct CropSaveView;
struct TrapSaveView;
struct FireSaveView;

class Game {
public:
  Game();

  bool init(IPlatform* platform, const char* data_dir);
  void set_audio(IAudio* audio) { audio_ = audio; }

  void shutdown();
  void update(float dt);
  void render();
  // TEST HARNESS ONLY (desktop --shots-building): puts the player inside (mode 0) or just south
  // of (mode 1) the first police/other Solid building. Never called by gameplay code.
  bool harness_place_at_building(int mode);
  // TEST HARNESS ONLY (--shots-wreck): 0 = south of the first Humvee wreck, 1 = heli wreck,
  // 2 = Zed_check from 1600 px of a frame-0 zed_resp_point, then stand at the point.
  bool harness_place_at_wreck(int mode);
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

private:
  friend struct GameTestAccess; // tests/test_selection.cpp
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
