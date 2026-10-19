#pragma once

#include "mdaz/mapgen.hpp"
#include "mdaz/types.hpp"

namespace mdaz {

class LootSystem;
class Rng;
struct SurvivalState;

// C2 LOOT Respawn group: Timer Check_loot → Global_respawn (Map layout POI loot refresh).
struct GlobalRespawnParams {
  MapGenerator* mapgen;
  LootSystem* loot;
  Rng* rng;
  Vec2 player_pos;
};

void global_respawn(GlobalRespawnParams& p);
// TD layout variant (audit anchor; desktop campaign uses Map only).
void global_respawn_TD(GlobalRespawnParams& p);

// C2 States / Reload_locations: Casual_Respawn_player after death when SEED_playcasual≠0.
void casual_respawn_player(SurvivalState& surv, MapGenerator& mapgen, LootSystem& loot, Rng& rng,
                           Vec2& player_pos);

} // namespace mdaz
