#include "mdaz/save_game.hpp"

#include <cstring>

namespace mdaz {

namespace {

void pack_visited(const bool* visited, int len, unsigned char out[32]) {
  std::memset(out, 0, 32);
  if (!visited || len <= 0) return;
  if (len > 256) len = 256;
  for (int i = 0; i < len; ++i) {
    if (visited[i]) out[i >> 3] |= static_cast<unsigned char>(1u << (i & 7));
  }
}

void unpack_visited(const unsigned char in[32], bool* visited, int len) {
  if (!visited || len <= 0) return;
  if (len > 256) len = 256;
  for (int i = 0; i < len; ++i) {
    visited[i] = (in[i >> 3] & (1u << (i & 7))) != 0;
  }
}

int pack_perks(const Player& p) {
  int f = 0;
  if (p.perk_metabolism) f |= 1;
  if (p.perk_hamster) f |= 2;
  if (p.perk_camel) f |= 4;
  if (p.perk_runner) f |= 8;
  if (p.perk_hunter) f |= 16;
  if (p.perk_farmer) f |= 32;
  if (p.perk_regen) f |= 64;
  if (p.perk_snowborn) f |= 128;
  return f;
}

void unpack_perks(Player& p, int f) {
  p.perk_metabolism = (f & 1) != 0;
  p.perk_hamster = (f & 2) != 0;
  p.perk_camel = (f & 4) != 0;
  p.perk_runner = (f & 8) != 0;
  p.perk_hunter = (f & 16) != 0;
  p.perk_farmer = (f & 32) != 0;
  p.perk_regen = (f & 64) != 0;
  p.perk_snowborn = (f & 128) != 0;
}

} // namespace

bool SaveSystem::write_to_buffer(const Player& player, const Inventory& inv,
                                 const WeatherState& weather, int equipped_slot,
                                 SaveBlob& out,
                                 const bool* visited, int visited_len,
                                 float torch_battery,
                                 int quest_active, int quest_progress,
                                 int mag_rounds, float wetness,
                                 float gasmask_filter, int fish_caught) {
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
  out.bandits_killed = player.bandits_killed;
  for (int i = 0; i < kInventorySlots; ++i) {
    out.inv_ids[i] = inv.slot(i).id;
    out.inv_counts[i] = inv.slot(i).count;
  }
  out.worn_count = inv.worn_count();
  if (out.worn_count > 6) out.worn_count = 6;
  for (int i = 0; i < out.worn_count; ++i) out.worn_ids[i] = inv.worn_at(i);
  out.day_phase = weather.day_phase;
  out.rain = weather.rain;
  out.infected = player.survival.infected ? 1 : 0;
  out.torch_on = (player.torch_on ? 1 : 0) | (player.nvg_on ? 2 : 0);
  out.equipped_slot = equipped_slot;
  pack_visited(visited, visited_len, out.visited);
  out.torch_battery = torch_battery;
  out.animals_killed = player.animals_killed;
  out.quest_active = quest_active;
  out.quest_progress = quest_progress;
  out.perk_flags = pack_perks(player);
  out.mag_rounds = mag_rounds;
  out.wetness = wetness;
  out.gasmask_filter = gasmask_filter;
  out.fish_caught = fish_caught;
  return true;
}

bool SaveSystem::read_from_buffer(const SaveBlob& in, Player& player, Inventory& inv,
                                  WeatherState& weather, int* equipped_slot,
                                  bool* visited, int visited_len,
                                  float* torch_battery,
                                  int* quest_active, int* quest_progress,
                                  int* mag_rounds, float* wetness,
                                  float* gasmask_filter, int* fish_caught) {
  if (in.magic != kMagic) return false;
  if (in.version < 2 || in.version > kVersion) return false;
  player.reset_new_game(in.difficulty);
  player.pos.x = in.pos_x;
  player.pos.y = in.pos_y;
  player.survival.food = in.food;
  player.survival.water = in.water;
  player.survival.blood = in.blood;
  player.survival.temperature = in.temperature;
  player.survival.bleeding = in.bleeding;
  player.survival.infected = in.infected != 0;
  player.torch_on = (in.torch_on & 1) != 0;
  player.nvg_on = (in.torch_on & 2) != 0;
  player.days_survived = in.days_survived;
  player.zeds_killed = in.zeds_killed;
  player.bandits_killed = in.bandits_killed;
  inv.clear();
  for (int i = 0; i < kInventorySlots; ++i) {
    inv.slot(i).id = in.inv_ids[i];
    inv.slot(i).count = in.inv_counts[i];
  }
  inv.restore_worn(in.worn_ids, in.worn_count);
  weather.day_phase = in.day_phase;
  weather.rain = in.rain;
  if (equipped_slot) *equipped_slot = in.equipped_slot;
  if (visited && visited_len > 0) {
    if (in.version >= 3) unpack_visited(in.visited, visited, visited_len);
    else for (int i = 0; i < visited_len; ++i) visited[i] = false;
  }
  if (in.version >= 4) {
    player.animals_killed = in.animals_killed;
    unpack_perks(player, in.perk_flags);
    if (torch_battery) *torch_battery = in.torch_battery;
    if (quest_active) *quest_active = in.quest_active;
    if (quest_progress) *quest_progress = in.quest_progress;
  } else {
    if (torch_battery) *torch_battery = 100.f;
    if (quest_active) *quest_active = 0;
    if (quest_progress) *quest_progress = 0;
  }
  if (in.version >= 5) {
    if (mag_rounds) *mag_rounds = in.mag_rounds;
    if (wetness) *wetness = in.wetness;
  } else {
    if (mag_rounds) *mag_rounds = 0;
    if (wetness) *wetness = 0.f;
  }
  if (in.version >= 6) {
    if (gasmask_filter) *gasmask_filter = in.gasmask_filter;
    if (fish_caught) *fish_caught = in.fish_caught;
    player.fish_caught = in.fish_caught;
  } else {
    if (gasmask_filter) *gasmask_filter = 100.f;
    if (fish_caught) *fish_caught = 0;
    player.fish_caught = 0;
  }
  return true;
}

bool SaveSystem::save_file(IPlatform& io, const char* path, const Player& player,
                           const Inventory& inv, const WeatherState& weather,
                           int equipped_slot,
                           const bool* visited, int visited_len,
                           float torch_battery,
                           int quest_active, int quest_progress,
                           int mag_rounds, float wetness,
                           float gasmask_filter, int fish_caught) {
  SaveBlob blob;
  if (!write_to_buffer(player, inv, weather, equipped_slot, blob, visited, visited_len,
                       torch_battery, quest_active, quest_progress, mag_rounds, wetness,
                       gasmask_filter, fish_caught))
    return false;
  return io.write_file(path, &blob, sizeof(blob));
}

bool SaveSystem::load_file(IPlatform& io, const char* path, Player& player,
                           Inventory& inv, WeatherState& weather,
                           int* equipped_slot,
                           bool* visited, int visited_len,
                           float* torch_battery,
                           int* quest_active, int* quest_progress,
                           int* mag_rounds, float* wetness,
                           float* gasmask_filter, int* fish_caught) {
  char* data = 0;
  std::size_t size = 0;
  if (!io.read_file(path, &data, &size)) return false;
  bool ok = false;
  // Accept current and older truncated blobs (v2..v6)
  if (size >= 64 && size <= sizeof(SaveBlob)) {
    SaveBlob blob;
    std::memset(&blob, 0, sizeof(blob));
    std::memcpy(&blob, data, size);
    ok = read_from_buffer(blob, player, inv, weather, equipped_slot, visited, visited_len,
                          torch_battery, quest_active, quest_progress, mag_rounds, wetness,
                          gasmask_filter, fish_caught);
  }
  delete[] data;
  return ok;
}

} // namespace mdaz
