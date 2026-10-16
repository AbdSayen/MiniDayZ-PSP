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
  float torch_battery() const { return torch_battery_; }
  float inv_full_flash() const { return inv_full_flash_; }
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
  float torch_battery_; // 0..100 — drains while torch/NVG on; Battery (22) refills
  float inv_full_flash_; // HUD flash when pickup fails
  float tent_warmth_t_;  // remaining shelter bonus seconds after placing tent
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

  void mark_sector_visited();
  void update_menu(float dt, const InputState& in);
  void update_playing(float dt, const InputState& in);
  void update_dead(float dt, const InputState& in);
  void try_melee_attack();
  void try_ranged_attack();
  void try_pickup_loot();
  void try_craft();
  void try_place_campfire();
  void try_drop_equipped();
  void try_mag_reload();
  void try_throw_projectile();
  void try_bunker_enter_exit();
  void resolve_bullet_hits();
  void explode_at(Vec2 pos, float damage, bool leave_fire = false);
  void on_animal_killed(Animal& a);
  void apply_incoming_damage(float amount, bool can_bleed);
  void write_death_log();
  void spawn_poi_loot();
  void update_army_shooters(float dt);
  void update_bandit_guns(float dt);
};

} // namespace mdaz
