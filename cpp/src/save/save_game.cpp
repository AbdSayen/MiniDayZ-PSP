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

void pack_vehicles(const VehicleSystem* vs, SaveBlob& out) {
  out.vehicle_count = 0;
  if (!vs) return;
  for (int i = 0; i < vs->count() && out.vehicle_count < 4; ++i) {
    const Vehicle* v = vs->at(i);
    if (!v || !v->active) continue;
    SavedVehicle& s = out.vehicles[out.vehicle_count++];
    s.type = static_cast<int>(v->type);
    s.x = v->pos.x;
    s.y = v->pos.y;
    s.facing = v->facing_deg;
    s.fuel = v->fuel;
    s.hp = v->hp;
    for (int j = 0; j < Vehicle::kStashSlots; ++j) {
      s.stash_ids[j] = v->stash_ids[j];
      s.stash_counts[j] = v->stash_counts[j];
    }
  }
}

void unpack_vehicles(const SaveBlob& in, VehicleSystem* vs) {
  if (!vs) return;
  vs->clear();
  if (in.version < 7) return;
  const int n = in.vehicle_count < 4 ? in.vehicle_count : 4;
  for (int i = 0; i < n; ++i) {
    const SavedVehicle& s = in.vehicles[i];
    if (s.type <= Vehicle_None || s.type > Vehicle_Truck) continue;
    Vehicle* v = vs->spawn(static_cast<VehicleType>(s.type), Vec2(s.x, s.y));
    if (!v) continue;
    v->facing_deg = s.facing;
    v->fuel = s.fuel;
    v->hp = s.hp;
    for (int j = 0; j < Vehicle::kStashSlots; ++j) {
      v->stash_ids[j] = s.stash_ids[j];
      v->stash_counts[j] = s.stash_counts[j];
    }
  }
}

void pack_crops(const CropSaveView* crops, SaveBlob& out) {
  out.crop_count = 0;
  if (!crops) return;
  const int n = crops->count < CropSaveView::kMax ? crops->count : CropSaveView::kMax;
  for (int i = 0; i < n; ++i) {
    if (crops->plots[i].crop_id <= 0) continue;
    out.crops[out.crop_count++] = crops->plots[i];
  }
}

void unpack_crops(const SaveBlob& in, CropSaveView* crops) {
  if (!crops) return;
  crops->count = 0;
  if (in.version < 8) return;
  const int n = in.crop_count < CropSaveView::kMax ? in.crop_count : CropSaveView::kMax;
  for (int i = 0; i < n; ++i) {
    if (in.crops[i].crop_id <= 0) continue;
    crops->plots[crops->count++] = in.crops[i];
  }
}

void pack_traps(const TrapSaveView* traps, SaveBlob& out) {
  out.trap_count = 0;
  if (!traps) return;
  const int n = traps->count < TrapSaveView::kMax ? traps->count : TrapSaveView::kMax;
  for (int i = 0; i < n; ++i) {
    out.traps[out.trap_count++] = traps->traps[i];
  }
}

void unpack_traps(const SaveBlob& in, TrapSaveView* traps) {
  if (!traps) return;
  traps->count = 0;
  if (in.version < 9) return;
  const int n = in.trap_count < TrapSaveView::kMax ? in.trap_count : TrapSaveView::kMax;
  for (int i = 0; i < n; ++i) {
    traps->traps[traps->count++] = in.traps[i];
  }
}

void pack_fires(const FireSaveView* fires, SaveBlob& out) {
  out.fire_count = 0;
  if (!fires) return;
  const int n = fires->count < FireSaveView::kMax ? fires->count : FireSaveView::kMax;
  for (int i = 0; i < n; ++i) {
    if (fires->fires[i].life <= 0.f) continue;
    out.fires[out.fire_count++] = fires->fires[i];
  }
}

void unpack_fires(const SaveBlob& in, FireSaveView* fires) {
  if (!fires) return;
  fires->count = 0;
  if (in.version < 11) return;
  const int n = in.fire_count < FireSaveView::kMax ? in.fire_count : FireSaveView::kMax;
  for (int i = 0; i < n; ++i) {
    if (in.fires[i].life <= 0.f) continue;
    fires->fires[fires->count++] = in.fires[i];
  }
}

void pack_loot(const LootSaveView* loot, SaveBlob& out) {
  out.loot_count = 0;
  out.loot_ext_count = 0;
  out.loot_respawn_t = 0.f;
  if (!loot) return;
  const int n = loot->count < LootSaveView::kMax ? loot->count : LootSaveView::kMax;
  for (int i = 0; i < n; ++i) {
    if (loot->piles[i].item_count <= 0) continue;
    if (out.loot_count < LootSaveView::kLegacyMax) {
      out.loot_piles[out.loot_count++] = loot->piles[i];
    } else if (out.loot_ext_count < LootSaveView::kExtMax) {
      out.loot_ext_piles[out.loot_ext_count++] = loot->piles[i];
    }
  }
}

void unpack_loot(const SaveBlob& in, LootSaveView* loot) {
  if (!loot) return;
  loot->count = 0;
  if (in.version < 14) return;
  const int n0 = in.loot_count < LootSaveView::kLegacyMax ? in.loot_count : LootSaveView::kLegacyMax;
  for (int i = 0; i < n0; ++i) {
    if (in.loot_piles[i].item_count <= 0) continue;
    if (loot->count >= LootSaveView::kMax) break;
    loot->piles[loot->count++] = in.loot_piles[i];
  }
  if (in.version >= 18) {
    const int ne =
        in.loot_ext_count < LootSaveView::kExtMax ? in.loot_ext_count : LootSaveView::kExtMax;
    for (int i = 0; i < ne; ++i) {
      if (in.loot_ext_piles[i].item_count <= 0) continue;
      if (loot->count >= LootSaveView::kMax) break;
      loot->piles[loot->count++] = in.loot_ext_piles[i];
    }
  }
}

} // namespace

bool SaveSystem::write_to_buffer(const Player& player, const Inventory& inv,
                                 const WeatherState& weather, int equipped_slot,
                                 SaveBlob& out,
                                 const bool* visited, int visited_len,
                                 float torch_battery,
                                 int quest_active, int quest_progress,
                                 int mag_rounds, float wetness,
                                 float gasmask_filter, int fish_caught,
                                 const VehicleSystem* vehicles,
                                 const CropSaveView* crops,
                                 const TrapSaveView* traps,
                                 const WorldExtras* extras,
                                 const FireSaveView* fires,
                                 const LootSaveView* loot) {
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
  for (int i = 0; i < out.worn_count; ++i) {
    out.worn_ids[i] = inv.worn_at(i);
    out.worn_iv6[i] = inv.worn_iv6(i);
  }
  for (int ri = 0; ri < ClothingRegion_Count; ++ri) {
    const ClothingRegion reg = static_cast<ClothingRegion>(ri);
    for (int si = 0; si < kClothingContainerSlots; ++si) {
      const int idx = ri * kClothingContainerSlots + si;
      out.cloth_ids[idx] = inv.clothing().slot(reg, si).id;
      out.cloth_counts[idx] = inv.clothing().slot(reg, si).count;
    }
  }
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
  pack_vehicles(vehicles, out);
  pack_crops(crops, out);
  pack_traps(traps, out);
  pack_fires(fires, out);
  pack_loot(loot, out);
  out.reserved_v10 = 0.f;
  if (extras) {
    out.airdrop_t = extras->airdrop_t;
    out.airdrop_x = extras->airdrop_x;
    out.airdrop_y = extras->airdrop_y;
    out.tent_warmth_t = extras->tent_warmth_t;
    out.adrenaline_t = extras->adrenaline_t;
    out.bunker_looted = extras->bunker_looted;
    out.melee_break_mul = extras->melee_break_mul > 0.f ? extras->melee_break_mul : 1.f;
    out.loot_respawn_t = extras->loot_respawn_t;
    out.wpn_aug_progress = extras->weapon_unlocks.aug_progress;
    out.wpn_mac10_progress = extras->weapon_unlocks.mac10_progress;
    out.wpn_sv98_progress = extras->weapon_unlocks.sv98_progress;
    out.wpn_saiga12_progress = extras->weapon_unlocks.saiga12_progress;
    out.wpn_vss_progress = extras->weapon_unlocks.vss_progress;
    out.wpn_unlock_flags = 0;
    if (extras->weapon_unlocks.aug_unlocked) out.wpn_unlock_flags |= 1;
    if (extras->weapon_unlocks.mac10_unlocked) out.wpn_unlock_flags |= 2;
    if (extras->weapon_unlocks.sv98_unlocked) out.wpn_unlock_flags |= 4;
    if (extras->weapon_unlocks.saiga12_unlocked) out.wpn_unlock_flags |= 8;
    if (extras->weapon_unlocks.vss_unlocked) out.wpn_unlock_flags |= 16;
  } else {
    out.airdrop_t = 360.f;
    out.airdrop_x = out.airdrop_y = 0.f;
    out.tent_warmth_t = 0.f;
    out.adrenaline_t = 0.f;
    out.bunker_looted = 0;
    out.melee_break_mul = 1.f;
    out.loot_respawn_t = 0.f;
  }
  return true;
}

bool SaveSystem::read_from_buffer(const SaveBlob& in, Player& player, Inventory& inv,
                                  WeatherState& weather, int* equipped_slot,
                                  bool* visited, int visited_len,
                                  float* torch_battery,
                                  int* quest_active, int* quest_progress,
                                  int* mag_rounds, float* wetness,
                                  float* gasmask_filter, int* fish_caught,
                                  VehicleSystem* vehicles,
                                  CropSaveView* crops,
                                  TrapSaveView* traps,
                                  WorldExtras* extras,
                                  FireSaveView* fires,
                                  LootSaveView* loot) {
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
  if (in.version >= 15)
    inv.restore_worn_iv6(in.worn_ids, in.worn_iv6, in.worn_count);
  else
    inv.restore_worn(in.worn_ids, in.worn_count);
  if (in.version >= 17) {
    for (int ri = 0; ri < ClothingRegion_Count; ++ri) {
      const ClothingRegion reg = static_cast<ClothingRegion>(ri);
      ItemId ids[kClothingContainerSlots];
      int counts[kClothingContainerSlots];
      for (int si = 0; si < kClothingContainerSlots; ++si) {
        const int idx = ri * kClothingContainerSlots + si;
        ids[si] = in.cloth_ids[idx];
        counts[si] = in.cloth_counts[idx];
      }
      inv.clothing().restore_region(reg, ids, counts, kClothingContainerSlots);
    }
  }
  weather.day_phase = in.day_phase;
  weather.rain = in.rain;
  weather.sync_from_day_phase();
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
  unpack_vehicles(in, vehicles);
  unpack_crops(in, crops);
  unpack_traps(in, traps);
  unpack_fires(in, fires);
  unpack_loot(in, loot);
  if (extras) {
    if (in.version >= 10) {
      extras->airdrop_t = in.airdrop_t;
      extras->airdrop_x = in.airdrop_x;
      extras->airdrop_y = in.airdrop_y;
    } else {
      extras->airdrop_t = 360.f;
      extras->airdrop_x = extras->airdrop_y = 0.f;
    }
    extras->tent_warmth_t = (in.version >= 11) ? in.tent_warmth_t : 0.f;
    extras->adrenaline_t = (in.version >= 12) ? in.adrenaline_t : 0.f;
    if (in.version >= 13) {
      extras->bunker_looted = in.bunker_looted;
      extras->melee_break_mul = in.melee_break_mul > 0.f ? in.melee_break_mul : 1.f;
    } else {
      extras->bunker_looted = 0;
      extras->melee_break_mul = 1.f;
    }
    if (in.version >= 14) {
      extras->loot_respawn_t = in.loot_respawn_t;
    } else {
      extras->loot_respawn_t = 0.f;
    }
    if (in.version >= 16) {
      extras->weapon_unlocks.aug_progress = in.wpn_aug_progress;
      extras->weapon_unlocks.mac10_progress = in.wpn_mac10_progress;
      extras->weapon_unlocks.sv98_progress = in.wpn_sv98_progress;
      extras->weapon_unlocks.saiga12_progress = in.wpn_saiga12_progress;
      extras->weapon_unlocks.vss_progress = in.wpn_vss_progress;
      extras->weapon_unlocks.aug_unlocked = (in.wpn_unlock_flags & 1) != 0;
      extras->weapon_unlocks.mac10_unlocked = (in.wpn_unlock_flags & 2) != 0;
      extras->weapon_unlocks.sv98_unlocked = (in.wpn_unlock_flags & 4) != 0;
      extras->weapon_unlocks.saiga12_unlocked = (in.wpn_unlock_flags & 8) != 0;
      extras->weapon_unlocks.vss_unlocked = (in.wpn_unlock_flags & 16) != 0;
    } else {
      extras->weapon_unlocks.reset();
    }
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
                           float gasmask_filter, int fish_caught,
                           const VehicleSystem* vehicles,
                           const CropSaveView* crops,
                           const TrapSaveView* traps,
                           const WorldExtras* extras,
                           const FireSaveView* fires,
                           const LootSaveView* loot) {
  SaveBlob blob;
  if (!write_to_buffer(player, inv, weather, equipped_slot, blob, visited, visited_len,
                       torch_battery, quest_active, quest_progress, mag_rounds, wetness,
                       gasmask_filter, fish_caught, vehicles, crops, traps, extras, fires,
                       loot))
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
                           float* gasmask_filter, int* fish_caught,
                           VehicleSystem* vehicles,
                           CropSaveView* crops,
                           TrapSaveView* traps,
                           WorldExtras* extras,
                           FireSaveView* fires,
                           LootSaveView* loot) {
  char* data = 0;
  std::size_t size = 0;
  if (!io.read_file(path, &data, &size)) return false;
  bool ok = false;
  // Accept current and older truncated blobs (v2..v14)
  if (size >= 64 && size <= sizeof(SaveBlob)) {
    SaveBlob blob;
    std::memset(&blob, 0, sizeof(blob));
    std::memcpy(&blob, data, size);
    ok = read_from_buffer(blob, player, inv, weather, equipped_slot, visited, visited_len,
                          torch_battery, quest_active, quest_progress, mag_rounds, wetness,
                          gasmask_filter, fish_caught, vehicles, crops, traps, extras, fires,
                          loot);
  }
  delete[] data;
  return ok;
}

} // namespace mdaz
