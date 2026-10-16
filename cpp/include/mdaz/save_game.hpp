#pragma once

#include "mdaz/inventory.hpp"
#include "mdaz/platform.hpp"
#include "mdaz/player.hpp"
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
  ItemId inv_ids[kInventorySlots];
  int inv_counts[kInventorySlots];
  float day_phase;
  float rain;
};

class SaveSystem {
public:
  static const int kMagic = 0x4D44415A; // 'MDAZ'
  static const int kVersion = 1;

  static bool write_to_buffer(const Player& player, const Inventory& inv,
                              const WeatherState& weather, SaveBlob& out);
  static bool read_from_buffer(const SaveBlob& in, Player& player, Inventory& inv,
                               WeatherState& weather);

  static bool save_file(IPlatform& io, const char* path, const Player& player,
                        const Inventory& inv, const WeatherState& weather);
  static bool load_file(IPlatform& io, const char* path, Player& player,
                        Inventory& inv, WeatherState& weather);
};

} // namespace mdaz
