#pragma once

#include "mdaz/inventory.hpp"
#include "mdaz/platform.hpp"
#include "mdaz/player.hpp"
#include "mdaz/types.hpp"
#include "mdaz/vehicle.hpp"
#include "mdaz/weather.hpp"

namespace mdaz {

struct SavedVehicle {
  int type; // VehicleType
  float x, y, facing, fuel, hp;
  ItemId stash_ids[Vehicle::kStashSlots];
  int stash_counts[Vehicle::kStashSlots];
};

struct SavedCrop {
  float x, y;
  ItemId crop_id;
  float grow_t;
  int ready; // 0/1
};

struct CropSaveView {
  static const int kMax = 16;
  int count;
  SavedCrop plots[kMax];
  CropSaveView() : count(0) {}
};

struct SavedTrap {
  float x, y;
  float life;
  int kind;
  int sprung; // 0/1
};

struct TrapSaveView {
  static const int kMax = 12;
  int count;
  SavedTrap traps[kMax];
  TrapSaveView() : count(0) {}
};

struct SavedFire {
  float x, y, life;
};

struct FireSaveView {
  static const int kMax = 8;
  int count;
  SavedFire fires[kMax];
  FireSaveView() : count(0) {}
};

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
  int torch_on; // bit0 torch, bit1 nvg
  int equipped_slot;
  // v3: 16×16 sector fog (256 bits)
  unsigned char visited[32];
  // v4 extras
  float torch_battery;
  int animals_killed;
  int quest_active;
  int quest_progress;
  int perk_flags; // packed hunter/farmer/regen/camel/hamster/runner/metabolism
  // v5 extras
  int mag_rounds;
  float wetness;
  // v6 extras
  float gasmask_filter;
  int fish_caught;
  // v7 extras — up to 4 vehicles + stash
  int vehicle_count;
  SavedVehicle vehicles[4];
  // v8 extras — crop plots
  int crop_count;
  SavedCrop crops[CropSaveView::kMax];
  // v9 extras — placed traps
  int trap_count;
  SavedTrap traps[TrapSaveView::kMax];
  // v10 extras — limp + airdrop timer/pos
  float limp_t;
  float airdrop_t;
  float airdrop_x, airdrop_y;
  // v11 extras — campfires + tent warmth
  int fire_count;
  SavedFire fires[FireSaveView::kMax];
  float tent_warmth_t;
  // v12 — adrenaline
  float adrenaline_t;
  // v13 — bunker loot flag + duct-tape melee durability mul
  int bunker_looted;
  float melee_break_mul;
};

struct WorldExtras {
  float limp_t;
  float airdrop_t;
  float airdrop_x, airdrop_y;
  float tent_warmth_t;
  float adrenaline_t;
  int bunker_looted;
  float melee_break_mul;
  WorldExtras()
    : limp_t(0.f), airdrop_t(360.f), airdrop_x(0.f), airdrop_y(0.f),
      tent_warmth_t(0.f), adrenaline_t(0.f), bunker_looted(0), melee_break_mul(1.f) {}
};

class SaveSystem {
public:
  static const int kMagic = 0x4D44415A; // 'MDAZ'
  static const int kVersion = 13;

  static bool write_to_buffer(const Player& player, const Inventory& inv,
                              const WeatherState& weather, int equipped_slot,
                              SaveBlob& out,
                              const bool* visited = 0, int visited_len = 0,
                              float torch_battery = 100.f,
                              int quest_active = 0, int quest_progress = 0,
                              int mag_rounds = 0, float wetness = 0.f,
                              float gasmask_filter = 100.f, int fish_caught = 0,
                              const VehicleSystem* vehicles = 0,
                              const CropSaveView* crops = 0,
                              const TrapSaveView* traps = 0,
                              const WorldExtras* extras = 0,
                              const FireSaveView* fires = 0);
  static bool read_from_buffer(const SaveBlob& in, Player& player, Inventory& inv,
                               WeatherState& weather, int* equipped_slot,
                               bool* visited = 0, int visited_len = 0,
                               float* torch_battery = 0,
                               int* quest_active = 0, int* quest_progress = 0,
                               int* mag_rounds = 0, float* wetness = 0,
                               float* gasmask_filter = 0, int* fish_caught = 0,
                               VehicleSystem* vehicles = 0,
                               CropSaveView* crops = 0,
                               TrapSaveView* traps = 0,
                               WorldExtras* extras = 0,
                               FireSaveView* fires = 0);

  static bool save_file(IPlatform& io, const char* path, const Player& player,
                        const Inventory& inv, const WeatherState& weather,
                        int equipped_slot = 0,
                        const bool* visited = 0, int visited_len = 0,
                        float torch_battery = 100.f,
                        int quest_active = 0, int quest_progress = 0,
                        int mag_rounds = 0, float wetness = 0.f,
                        float gasmask_filter = 100.f, int fish_caught = 0,
                        const VehicleSystem* vehicles = 0,
                        const CropSaveView* crops = 0,
                        const TrapSaveView* traps = 0,
                        const WorldExtras* extras = 0,
                        const FireSaveView* fires = 0);
  static bool load_file(IPlatform& io, const char* path, Player& player,
                        Inventory& inv, WeatherState& weather,
                        int* equipped_slot = 0,
                        bool* visited = 0, int visited_len = 0,
                        float* torch_battery = 0,
                        int* quest_active = 0, int* quest_progress = 0,
                        int* mag_rounds = 0, float* wetness = 0,
                        float* gasmask_filter = 0, int* fish_caught = 0,
                        VehicleSystem* vehicles = 0,
                        CropSaveView* crops = 0,
                        TrapSaveView* traps = 0,
                        WorldExtras* extras = 0,
                        FireSaveView* fires = 0);
};

} // namespace mdaz
