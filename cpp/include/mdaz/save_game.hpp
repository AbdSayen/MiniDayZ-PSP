#pragma once

#include "mdaz/inventory.hpp"
#include "mdaz/platform.hpp"
#include "mdaz/player.hpp"
#include "mdaz/types.hpp"
#include "mdaz/weather.hpp"

namespace mdaz {

struct SaveBlob {
  int magic;
  int version;
  Difficulty difficulty;
  float pos_x, pos_y;
  float food, water, blood, temperature, bleeding;
  int days_survived;
  int zeds_killed;
  int bandits_killed;
  ItemId inv_ids[kInventorySlots];
  int inv_counts[kInventorySlots];
  ItemId worn_ids[6];
  int worn_count;
  float day_phase;
  float rain;
  int infected;
  int torch_on;
  int equipped_slot;
  // v3: 16×16 sector fog (256 bits)
  unsigned char visited[32];
};

class SaveSystem {
public:
  static const int kMagic = 0x4D44415A; // 'MDAZ'
  static const int kVersion = 3;

  static bool write_to_buffer(const Player& player, const Inventory& inv,
                              const WeatherState& weather, int equipped_slot,
                              SaveBlob& out,
                              const bool* visited = 0, int visited_len = 0);
  static bool read_from_buffer(const SaveBlob& in, Player& player, Inventory& inv,
                               WeatherState& weather, int* equipped_slot,
                               bool* visited = 0, int visited_len = 0);

  static bool save_file(IPlatform& io, const char* path, const Player& player,
                        const Inventory& inv, const WeatherState& weather,
                        int equipped_slot = 0,
                        const bool* visited = 0, int visited_len = 0);
  static bool load_file(IPlatform& io, const char* path, Player& player,
                        Inventory& inv, WeatherState& weather,
                        int* equipped_slot = 0,
                        bool* visited = 0, int visited_len = 0);
};

} // namespace mdaz
