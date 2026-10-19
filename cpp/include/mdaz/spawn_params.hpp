#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

class LootSystem;
class Rng;
class ZombieSystem;

// C2 zed_resp array keys (obj209) → Trigger_spawn when IV frame threshold hit (spawn1/spawn2 handlers).
struct ZedRespawnChannels {
  int spawn1_count;
  int spawn2_count;

  ZedRespawnChannels() : spawn1_count(0), spawn2_count(0) {}
  void reset() { spawn1_count = spawn2_count = 0; }
};

// spawn1 handler: zed_resp["spawn1"] frame reaches 16 → Trigger_spawn (IV151 from obj209).
bool zed_resp_tick_spawn1(ZedRespawnChannels& ch, ZombieSystem& zeds, Vec2 near_pos, Rng& rng);
// spawn2 handler: spawn1 frame 7 and spawn2 frame 4 (decoded thresholds).
bool zed_resp_tick_spawn2(ZedRespawnChannels& ch, ZombieSystem& zeds, Vec2 near_pos, Rng& rng);

// spawn3: when secret_location_spawned flips 0→1, Spawn_drop tables at secret markers (obj696 IV59/60).
void spawn3_secret_drops(LootSystem& loot, Vec2 secret_pos, Rng& rng);

} // namespace mdaz
