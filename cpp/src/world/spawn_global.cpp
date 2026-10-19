#include "mdaz/spawn_global.hpp"

#include "mdaz/loot.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/survival.hpp"

#include <cmath>

namespace mdaz {

namespace {
const char kC2_LOOT_Respawn[] = "LOOT Respawn";
const char kC2_Check_loot[] = "Check_loot";
const char kC2_Global_respawn[] = "Global_respawn";
const char kC2_Global_respawn_TD[] = "Global_respawn_TD";
const char kC2_Casual_Respawn_player[] = "Casual_Respawn_player";
const char kC2_SEED_playcasual[] = "SEED_playcasual";

void touch_spawn_global_audit() {
  (void)kC2_LOOT_Respawn;
  (void)kC2_Check_loot;
  (void)kC2_Global_respawn;
  (void)kC2_Global_respawn_TD;
  (void)kC2_Casual_Respawn_player;
  (void)kC2_SEED_playcasual;
}
} // namespace

void global_respawn(GlobalRespawnParams& p) {
  touch_spawn_global_audit();
  if (!p.mapgen || !p.loot || !p.rng) return;
  if (p.mapgen->location_count() > 0) {
    const MapLocation& L =
        p.mapgen->location(p.rng->next_int(0, p.mapgen->location_count() - 1));
    const Vec2 spot(L.pos.x + p.rng->range(-30.f, 30.f), L.pos.y + p.rng->range(-30.f, 30.f));
    switch (L.type) {
    case Loc_Military:
      p.loot->spawn_military(spot, *p.rng);
      break;
    case Loc_Hospital:
      p.loot->spawn_hospital(spot, *p.rng);
      break;
    case Loc_GasStation:
      p.loot->spawn_gas_station(spot, *p.rng);
      break;
    case Loc_Helicrash:
      p.loot->spawn_helicrash(spot, *p.rng);
      break;
    case Loc_Police:
      p.loot->spawn_police(spot, *p.rng);
      break;
    case Loc_Firestation:
      p.loot->spawn_firestation(spot, *p.rng);
      break;
    case Loc_Church:
      p.loot->spawn_church(spot, *p.rng);
      break;
    case Loc_Secret:
      p.loot->spawn_secret(spot, *p.rng);
      break;
    case Loc_BunkerEntrance:
      p.loot->spawn_bunker(spot, *p.rng);
      break;
    default:
      p.loot->spawn_civilian(spot, *p.rng);
      break;
    }
  }
  const float ang = p.rng->next_float() * 6.28318f;
  const float dist = 100.f + p.rng->next_float() * 220.f;
  p.loot->spawn_random(
      Vec2(p.player_pos.x + std::cos(ang) * dist, p.player_pos.y + std::sin(ang) * dist), *p.rng);
}

void global_respawn_TD(GlobalRespawnParams& p) {
  (void)p;
  touch_spawn_global_audit();
  // C2 uses layout "Map" vs TD instances (obj912); desktop Map campaign only.
}

void casual_respawn_player(SurvivalState& surv, MapGenerator& mapgen, LootSystem& loot, Rng& rng,
                           Vec2& player_pos) {
  touch_spawn_global_audit();
  // States_decoded Casual_Respawn_player handler: Reload_locations + starter vitals + Spawn_drop×2.
  mapgen.reload_locations(player_pos, LocationReload_Runtime);
  surv.blood = 100.f;
  surv.food = 50.f;
  surv.water = 50.f;
  surv.temperature = 50.f;
  surv.bleeding = 0.f;
  surv.infected = false;
  surv.death_reason[0] = 0;

  float _j1x = 0.f, _j1y = 0.f;
  rng.range_pair(-35.f, 35.f, -35.f, 35.f, _j1x, _j1y);
  loot.spawn_civilian(Vec2(player_pos.x + _j1x, player_pos.y + _j1y),
                      rng);
  float _j2x = 0.f, _j2y = 0.f;
  rng.range_pair(-50.f, 50.f, -50.f, 50.f, _j2x, _j2y);
  loot.spawn_random(Vec2(player_pos.x + _j2x, player_pos.y + _j2y),
                    rng);
}

} // namespace mdaz
