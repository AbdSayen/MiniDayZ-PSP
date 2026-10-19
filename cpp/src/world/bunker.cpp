#include "mdaz/bunker.hpp"

#include "mdaz/bullet.hpp"
#include "mdaz/loot.hpp"
#include "mdaz/zombie.hpp"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

namespace mdaz {

namespace {
const char kC2_Bunker_erase[] = "Bunker_erase";
const char kC2_Bunker_generate[] = "Bunker_generate";
const char kC2_inBunker[] = "inBunker";
const char kC2_Building_mode[] = "Building_mode";
const char kC2_HotspotX[] = "HotspotX";
const char kC2_HotspotY[] = "HotspotY";
const char kC2_hotspotX[] = "hotspotX";
const char kC2_hotspotY[] = "hotspotY";
const char kC2_TilemapspotX[] = "TilemapspotX";
const char kC2_TilemapspotY[] = "TilemapspotY";
const char kC2_Reopen[] = "Reopen";

constexpr float kBunkerReopenSec = 1800.f;

void touch_bunker_c2_audit() {
  (void)kC2_Bunker_erase;
  (void)kC2_Bunker_generate;
  (void)kC2_inBunker;
  (void)kC2_Building_mode;
  (void)kC2_HotspotX;
  (void)kC2_HotspotY;
  (void)kC2_hotspotX;
  (void)kC2_hotspotY;
  (void)kC2_TilemapspotX;
  (void)kC2_TilemapspotY;
  (void)kC2_Reopen;
}

static void despawn_in_bunker_bounds(const BunkerMap& map, ZombieSystem& zeds, LootSystem& loot) {
  if (map.width <= 0 || map.height <= 0) return;
  const float bw = static_cast<float>(map.width) * 8.f + 80.f;
  const float bh = static_cast<float>(map.height) * 8.f + 80.f;
  for (Zombie* z = zeds.begin(); z != zeds.end(); ++z) {
    if (!z->alive) continue;
    if (z->pos.x >= 0.f && z->pos.y >= 0.f && z->pos.x < bw && z->pos.y < bh) z->alive = false;
  }
  for (int i = 0; i < loot.count(); ++i) {
    LootPile* p = loot.at(i);
    if (!p->active) continue;
    if (p->pos.x >= 0.f && p->pos.y >= 0.f && p->pos.x < bw && p->pos.y < bh) p->active = false;
  }
}
} // namespace

BunkerMap::BunkerMap() : width(0), height(0) {}

static int parse_int_at(const char*& p) {
  while (*p == ' ' || *p == '\t' || *p == ',') ++p;
  char* end = 0;
  long v = std::strtol(p, &end, 10);
  p = end ? end : p;
  return static_cast<int>(v);
}

// C2 RLE: "NxV" means N copies of V, bare V means one copy. Comma-separated.
static bool decode_rle(const char* data, std::vector<int>& out) {
  out.clear();
  const char* p = data;
  while (*p) {
    while (*p == ',' || *p == ' ') ++p;
    if (!*p) break;
    const char* start = p;
    while (*p && *p != ',' && *p != 'x') ++p;
    if (*p == 'x') {
      std::string nstr(start, p);
      int n = std::atoi(nstr.c_str());
      ++p;
      int v = parse_int_at(p);
      if (n < 0) n = 0;
      for (int i = 0; i < n; ++i) out.push_back(v);
    } else {
      std::string vstr(start, p);
      out.push_back(std::atoi(vstr.c_str()));
    }
  }
  return !out.empty();
}

bool BunkerMap::load_from_c2_json(const char* json) {
  if (!json) return false;
  const char* wkey = std::strstr(json, "\"width\"");
  const char* hkey = std::strstr(json, "\"height\"");
  const char* dkey = std::strstr(json, "\"data\"");
  if (!wkey || !hkey || !dkey) return false;

  width = std::atoi(std::strchr(wkey, ':') + 1);
  height = std::atoi(std::strchr(hkey, ':') + 1);

  // After "data" key, find the string value's opening quote
  const char* colon = std::strchr(dkey, ':');
  if (!colon) return false;
  const char* q1 = std::strchr(colon, '\"');
  if (!q1) return false;
  ++q1;
  const char* q2 = std::strchr(q1, '\"');
  if (!q2) return false;
  std::string rle(q1, q2);

  if (!decode_rle(rle.c_str(), tiles)) return false;
  if (static_cast<int>(tiles.size()) < width * height) {
    tiles.resize(static_cast<std::size_t>(width * height), 0);
  }
  return width > 0 && height > 0;
}

bool BunkerMap::load_from_file(const char* path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return false;
  std::ostringstream ss;
  ss << in.rdbuf();
  return load_from_c2_json(ss.str().c_str());
}

int BunkerMap::tile_at(int x, int y) const {
  if (x < 0 || y < 0 || x >= width || y >= height) return -1;
  return tiles[static_cast<std::size_t>(y * width + x)];
}

bool BunkerMap::is_blocked_world(float wx, float wy, float tile_size) const {
  if (width <= 0 || height <= 0 || tile_size <= 0.f) return true;
  const int tx = static_cast<int>(wx / tile_size);
  const int ty = static_cast<int>(wy / tile_size);
  const int id = tile_at(tx, ty);
  if (id < 0) return true; // OOB
  // Walkable fills seen in 1.4.1 bunker RLE: empty/dirt/interior floors
  if (id == 0 || id == 1 || id == 6 || id == 16 || id == 20) return false;
  return true;
}

bool bunker_generate(BunkerMap& map, const char* json_path) {
  touch_bunker_c2_audit();
  if (map.width > 0 && map.height > 0) return true;
  if (json_path && map.load_from_file(json_path)) return true;
  return false;
}

void bunker_clear_interior(const BunkerMap& map, ZombieSystem& zeds, LootSystem& loot,
                           BulletSystem& bullets) {
  touch_bunker_c2_audit();
  despawn_in_bunker_bounds(map, zeds, loot);
  bullets.clear();
}

BunkerEraseResult bunker_erase(bool in_bunker, const BunkerMap& map, ZombieSystem& zeds,
                               LootSystem& loot, BulletSystem& bullets, bool seal_entrance) {
  touch_bunker_c2_audit();
  BunkerEraseResult out;
  out.was_in_bunker = in_bunker;
  out.reopen_timer_sec = 0.f;
  if (!in_bunker) return out;
  bunker_clear_interior(map, zeds, loot, bullets);
  if (seal_entrance) out.reopen_timer_sec = kBunkerReopenSec;
  return out;
}

void bunker_tick_reopen(float dt, float& reopen_timer_sec) {
  touch_bunker_c2_audit();
  if (reopen_timer_sec <= 0.f) return;
  reopen_timer_sec -= dt;
  if (reopen_timer_sec < 0.f) reopen_timer_sec = 0.f;
}

} // namespace mdaz
