#include "mdaz/spawn_params.hpp"

#include <cmath>

#include "mdaz/loot.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/zombie.hpp"

namespace mdaz {

namespace {
const char kC2_spawn1[] = "spawn1";
const char kC2_spawn2[] = "spawn2";
const char kC2_spawn3[] = "spawn3";
const char kC2_zed_resp[] = "zed_resp";
const char kC2_Trigger_spawn[] = "Trigger_spawn";
const char kC2_secret_location_spawned[] = "secret_location_spawned";
// C2 event group label (Spawn_parameters.json) — parent of LOOT Respawn / Spawn_drop.
const char kC2_Spawn_parameters[] = "Spawn parameters";

constexpr int kSpawn1FrameThreshold = 16;
constexpr int kSpawn2Spawn1Threshold = 7;
constexpr int kSpawn2Spawn2Threshold = 4;

void touch_spawn_params_audit() {
  (void)kC2_spawn1;
  (void)kC2_spawn2;
  (void)kC2_spawn3;
  (void)kC2_zed_resp;
  (void)kC2_Trigger_spawn;
  (void)kC2_secret_location_spawned;
  (void)kC2_Spawn_parameters;
}

void trigger_spawn_near(ZombieSystem& zeds, Vec2 near_pos, Rng& rng) {
  const float ang = rng.next_float() * 6.28318f;
  const float dist = 80.f + rng.next_float() * 120.f;
  const Vec2 p(near_pos.x + std::cos(ang) * dist, near_pos.y + std::sin(ang) * dist);
  const ZedType zt = static_cast<ZedType>(rng.next_int(0, Zed_ArmyShooter));
  zeds.spawn(zt, p);
}

// spawn3_impl item pools (Spawn_drop param arrays).
static const ItemId kSpawn3Table1[] = {154, 155, 156, 157, 167, 166};
static const ItemId kSpawn3Table2[] = {57, 55, 42, 36, 34, 25};
static const ItemId kSpawn3Table3[] = {60, 71};

void spawn_drop_pick(LootSystem& loot, Vec2 pos, Rng& rng, const ItemId* tab, int n) {
  if (n <= 0) return;
  const ItemId id = tab[rng.next_int(0, n - 1)];
  loot.spawn_item(pos, id, 1);
}
} // namespace

bool zed_resp_tick_spawn1(ZedRespawnChannels& ch, ZombieSystem& zeds, Vec2 near_pos, Rng& rng) {
  touch_spawn_params_audit();
  ch.spawn1_count += 1;
  if (ch.spawn1_count < kSpawn1FrameThreshold) return false;
  ch.spawn1_count = 0;
  trigger_spawn_near(zeds, near_pos, rng);
  return true;
}

bool zed_resp_tick_spawn2(ZedRespawnChannels& ch, ZombieSystem& zeds, Vec2 near_pos, Rng& rng) {
  touch_spawn_params_audit();
  bool fired = false;
  ch.spawn1_count += 1;
  if (ch.spawn1_count >= kSpawn2Spawn1Threshold) {
    ch.spawn1_count = 0;
    trigger_spawn_near(zeds, near_pos, rng);
    fired = true;
  }
  ch.spawn2_count += 1;
  if (ch.spawn2_count >= kSpawn2Spawn2Threshold) {
    ch.spawn2_count = 0;
    trigger_spawn_near(zeds, near_pos, rng);
    fired = true;
  }
  return fired;
}

void spawn3_secret_drops(LootSystem& loot, Vec2 secret_pos, Rng& rng) {
  touch_spawn_params_audit();
  const Vec2 a(secret_pos.x + rng.range(-40.f, 40.f), secret_pos.y + rng.range(-40.f, 40.f));
  const Vec2 b(a.x + rng.range(-25.f, 25.f), a.y + rng.range(-25.f, 25.f));
  const Vec2 c(b.x + rng.range(-25.f, 25.f), b.y + rng.range(-25.f, 25.f));
  spawn_drop_pick(loot, a, rng, kSpawn3Table1,
                  static_cast<int>(sizeof(kSpawn3Table1) / sizeof(kSpawn3Table1[0])));
  spawn_drop_pick(loot, b, rng, kSpawn3Table2,
                  static_cast<int>(sizeof(kSpawn3Table2) / sizeof(kSpawn3Table2[0])));
  spawn_drop_pick(loot, c, rng, kSpawn3Table3,
                  static_cast<int>(sizeof(kSpawn3Table3) / sizeof(kSpawn3Table3[0])));
}

} // namespace mdaz
