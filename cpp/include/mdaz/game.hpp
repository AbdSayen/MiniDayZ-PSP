#pragma once

#include "mdaz/animal.hpp"
#include "mdaz/inventory.hpp"
#include "mdaz/item_db.hpp"
#include "mdaz/loot.hpp"
#include "mdaz/mapgen.hpp"
#include "mdaz/platform.hpp"
#include "mdaz/player.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/timers.hpp"
#include "mdaz/types.hpp"
#include "mdaz/weather.hpp"
#include "mdaz/world.hpp"
#include "mdaz/zombie.hpp"

namespace mdaz {

class Game {
public:
  Game();

  bool init(IPlatform* platform, const char* data_dir);
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

  // Test hooks
  Player& player_mut() { return player_; }
  Inventory& inventory_mut() { return inventory_; }
  void set_state(AppState s) { state_ = s; }
  void start_new_game(Difficulty d);

private:
  IPlatform* platform_;
  AppState state_;
  bool quit_;
  Player player_;
  Inventory inventory_;
  ItemDatabase items_;
  World world_;
  WeatherState weather_;
  ZombieSystem zombies_;
  AnimalSystem animals_;
  WorldTimers timers_;
  LootSystem loot_;
  MapGenerator mapgen_;
  Rng rng_;
  float state_timer_;
  char data_dir_[512];

  void update_menu(float dt, const InputState& in);
  void update_playing(float dt, const InputState& in);
  void update_dead(float dt, const InputState& in);
};

} // namespace mdaz
