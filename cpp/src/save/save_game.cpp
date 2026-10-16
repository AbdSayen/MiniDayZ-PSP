#include "mdaz/save_game.hpp"

#include <cstring>

namespace mdaz {

bool SaveSystem::write_to_buffer(const Player& player, const Inventory& inv,
                                 const WeatherState& weather, SaveBlob& out) {
  std::memset(&out, 0, sizeof(out));
  out.magic = kMagic;
  out.version = kVersion;
  out.difficulty = player.difficulty;
  out.pos_x = player.pos.x;
  out.pos_y = player.pos.y;
  out.food = player.survival.food;
  out.water = player.survival.water;
  out.blood = player.survival.blood;
  out.temperature = player.survival.temperature;
  out.bleeding = player.survival.bleeding;
  out.days_survived = player.days_survived;
  out.zeds_killed = player.zeds_killed;
  for (int i = 0; i < kInventorySlots; ++i) {
    out.inv_ids[i] = inv.slot(i).id;
    out.inv_counts[i] = inv.slot(i).count;
  }
  out.day_phase = weather.day_phase;
  out.rain = weather.rain;
  return true;
}

bool SaveSystem::read_from_buffer(const SaveBlob& in, Player& player, Inventory& inv,
                                  WeatherState& weather) {
  if (in.magic != kMagic || in.version != kVersion) return false;
  player.reset_new_game(in.difficulty);
  player.pos.x = in.pos_x;
  player.pos.y = in.pos_y;
  player.survival.food = in.food;
  player.survival.water = in.water;
  player.survival.blood = in.blood;
  player.survival.temperature = in.temperature;
  player.survival.bleeding = in.bleeding;
  player.days_survived = in.days_survived;
  player.zeds_killed = in.zeds_killed;
  inv.clear();
  for (int i = 0; i < kInventorySlots; ++i) {
    inv.slot(i).id = in.inv_ids[i];
    inv.slot(i).count = in.inv_counts[i];
  }
  weather.day_phase = in.day_phase;
  weather.rain = in.rain;
  return true;
}

bool SaveSystem::save_file(IPlatform& io, const char* path, const Player& player,
                           const Inventory& inv, const WeatherState& weather) {
  SaveBlob blob;
  if (!write_to_buffer(player, inv, weather, blob)) return false;
  return io.write_file(path, &blob, sizeof(blob));
}

bool SaveSystem::load_file(IPlatform& io, const char* path, Player& player,
                           Inventory& inv, WeatherState& weather) {
  char* data = 0;
  std::size_t size = 0;
  if (!io.read_file(path, &data, &size)) return false;
  bool ok = false;
  if (size == sizeof(SaveBlob)) {
    SaveBlob blob;
    std::memcpy(&blob, data, sizeof(blob));
    ok = read_from_buffer(blob, player, inv, weather);
  }
  delete[] data;
  return ok;
}

} // namespace mdaz
