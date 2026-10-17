#pragma once

#include "mdaz/animal.hpp"
#include "mdaz/audio.hpp"
#include "mdaz/bandit.hpp"
#include "mdaz/bullet.hpp"
#include "mdaz/bunker.hpp"
#include "mdaz/campfire.hpp"
#include "mdaz/combat.hpp"
#include "mdaz/inventory.hpp"
#include "mdaz/item_db.hpp"
#include "mdaz/loot.hpp"
#include "mdaz/mapgen.hpp"
#include "mdaz/platform.hpp"
#include "mdaz/player.hpp"
#include "mdaz/quest.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/timers.hpp"
#include "mdaz/types.hpp"
#include "mdaz/vehicle.hpp"
#include "mdaz/weather.hpp"
#include "mdaz/world.hpp"
#include "mdaz/zombie.hpp"

namespace mdaz {

class Game {
public:
  Game();

  bool init(IPlatform* platform, const char* data_dir);
  void set_audio(IAudio* audio) { audio_ = audio; }
  void shutdown();
  void update(float dt);
  void render();
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
  bool near_campfire() const { return campfires_.near_fire(player_.pos); }
  int equipped_slot() const { return equipped_slot_; }
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
  float limp_t() const { return limp_t_; }
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

  Player& player_mut() { return player_; }
  Inventory& inventory_mut() { return inventory_; }
  void set_state(AppState s) { state_ = s; }
  void start_new_game(Difficulty d);

private:
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
  VehicleSystem vehicles_;
  CampfireSystem campfires_;
  QuestState quest_;
  WorldTimers timers_;
  LootSystem loot_;
  MapGenerator mapgen_;
  BulletSystem bullets_;
  BunkerMap bunker_;
  DispersionState dispersion_;
  Rng rng_;
  float state_timer_;
  float attack_cd_;
  float reload_cd_;
  float airdrop_alert_;
  float radio_t_; // seconds to show radio HUD after use
  float torch_battery_; // 0..100 — drains while torch/NVG on; Battery (22) refills
  float inv_full_flash_; // HUD flash when pickup fails
  float tent_warmth_t_;  // remaining shelter bonus seconds after placing tent
  float indoor_t_;       // POI interior search warmth timer
  float footstep_t_;     // cadence timer for run/walk SFX
  float wetness_;        // 0..100 clothes wetness (rain up / fire dry)
  float gasmask_filter_; // 0..100 filter life while wearing gasmask
  float adrenaline_t_;   // morphine sprint boost remaining seconds
  float limp_t_;         // broken-leg stub: slows move while >0
  float melee_break_mul_; // duct tape reinforce reduces melee break chance
  bool has_headlamp_;    // worn/owned headlamp enables torch toggle without kit
  bool gun_jammed_;      // clear with V reload
  int mag_rounds_;       // rounds in current weapon chamber/mag
  ItemId mag_weapon_id_; // weapon the mag belongs to
  int equipped_slot_;
  bool in_bunker_;
  bool bunker_looted_;
  bool show_bag_;
  Difficulty menu_difficulty_;
  Vec2 overworld_pos_;
  Vec2 last_airdrop_pos_;
  bool visited_sectors_[kMapSectorsX * kMapSectorsY];
  bool prev_scream_;
  char data_dir_[512];

  static const int kMaxTraps = 12;
  struct PlaceTrap {
    Vec2 pos;
    float life;     // seconds remaining
    int kind;       // 0=bear, 1=barbed, 2=fence, 3=claymore, 4=landmine
    bool active;
    bool sprung;    // bear trap already triggered
  };
  PlaceTrap traps_[kMaxTraps];
  int trap_count_;
  float howl_cd_;
  float cough_cd_;
  static const int kMaxTrees = 96;
  Vec2 trees_[kMaxTrees];
  int tree_count_;
  static const int kMaxBloodDrips = 48;
  struct BloodDrip { Vec2 pos; float life; };
  BloodDrip blood_drips_[kMaxBloodDrips];
  int blood_drip_count_;
  float bleed_drip_cd_;
  bool crouching_;
  float fuel_warn_cd_;
  float rain_sfx_cd_;
  float lightning_t_; // brief white flash after thunder
  float thunder_cd_;
  float cough_alert_t_; // brief window after cough that pulls zeds
  float wind_sfx_cd_;
  float flare_t_;
  Vec2 flare_pos_;

  // Visual FX / corpses (presentation parity with C2)
  enum FxKind { Fx_Muzzle = 0, Fx_Hit, Fx_Explode, Fx_Shell, Fx_Blood, Fx_Fish };
  static const int kMaxFx = 32;
  struct FxSprite { Vec2 pos; float life; float angle; FxKind kind; bool active; };
  FxSprite fx_[kMaxFx];
  int fx_count_;

  enum CorpseKind { Corpse_Zed = 0, Corpse_Bandit, Corpse_Deer, Corpse_Wolf, Corpse_Dog, Corpse_Other };
  static const int kMaxCorpses = 48;
  struct Corpse { Vec2 pos; float life; float angle; CorpseKind kind; int variant; bool active; };
  Corpse corpses_[kMaxCorpses];
  int corpse_count_;

  void spawn_fx(FxKind kind, Vec2 pos, float angle_deg = 0.f, float life = 0.12f);
  void spawn_corpse(CorpseKind kind, Vec2 pos, float angle_deg, int variant = 0);
  void update_fx(float dt);

  void mark_sector_visited();
  void update_menu(float dt, const InputState& in);
  void update_playing(float dt, const InputState& in);
  void update_dead(float dt, const InputState& in);
  void try_melee_attack();
  void try_ranged_attack();
  void try_pickup_loot();
  void try_craft();
  void try_place_campfire();
  bool try_place_trap(int kind);
  void update_traps(float dt);
  void try_drop_equipped();
  void try_mag_reload();
  void sync_mag_for_equipped();
  void try_throw_projectile();
  void try_bunker_enter_exit();
  void resolve_bullet_hits();
  void explode_at(Vec2 pos, float damage, bool leave_fire = false);
  void on_animal_killed(Animal& a);
  void on_zed_killed(Zombie& z);
  void apply_incoming_damage(float amount, bool can_bleed);
  void write_death_log();
  void spawn_poi_loot();
  void update_army_shooters(float dt);
  void update_bandit_guns(float dt);
};

} // namespace mdaz
