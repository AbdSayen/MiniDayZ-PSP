#pragma once

#include <string>
#include <vector>

namespace mdaz {

class BulletSystem;
class LootSystem;
class ZombieSystem;

// C2 Bunker_erase: inBunker=0, clear bunker instances, Timer 1800 → Reopen.
struct BunkerEraseResult {
  bool was_in_bunker;
  float reopen_timer_sec; // 1800 when entrance sealed (obj780 Timer)
};

// Loader for Construct 2 tilemap RLE used by original_docs/bunker_json
struct BunkerMap {
  int width;
  int height;
  std::vector<int> tiles; // row-major, length width*height

  BunkerMap();
  bool load_from_file(const char* path);
  bool load_from_c2_json(const char* json);
  int tile_at(int x, int y) const;
  // Non-zero tiles treated as solid walls (0 = walkable floor / empty).
  bool is_blocked_world(float wx, float wy, float tile_size = 8.f) const;
};

// Bunker_generate: ensure bunker tilemap loaded (C2 creates bg layers + HotspotX/Y).
bool bunker_generate(BunkerMap& map, const char* json_path);

void bunker_clear_interior(const BunkerMap& map, ZombieSystem& zeds, LootSystem& loot,
                           BulletSystem& bullets);

// seal_entrance: C2 Bunker_erase sets Timer 1800 → Reopen (death / Reload_locations).
BunkerEraseResult bunker_erase(bool in_bunker, const BunkerMap& map, ZombieSystem& zeds,
                               LootSystem& loot, BulletSystem& bullets, bool seal_entrance);

void bunker_tick_reopen(float dt, float& reopen_timer_sec);

} // namespace mdaz
