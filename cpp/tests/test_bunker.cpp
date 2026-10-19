#include "mdaz/bunker.hpp"

#include <cstdio>

int test_bunker() {
  std::printf("--- test_bunker ---\n");
  mdaz::BunkerMap map;
  const char* paths[] = {
    "data/bunker_json",
    "cpp/data/bunker_json",
    "cpp/build/data/bunker_json",
    "../data/bunker_json",
    "../../original_docs/bunker_json",
    "../original_docs/bunker_json",
    "original_docs/bunker_json",
    0
  };
  bool loaded = false;
  for (int i = 0; paths[i]; ++i) {
    if (map.load_from_file(paths[i])) {
      loaded = true;
      std::printf("OK   loaded from %s\n", paths[i]);
      break;
    }
  }
  if (!loaded) {
    // Inline minimal RLE sample matching C2 format
    const char* sample = "{\"c2tilemap\":true,\"width\":4,\"height\":2,\"data\":\"2x6,8,9,3x11\"}";
    if (!map.load_from_c2_json(sample)) {
      std::printf("FAIL cannot load bunker\n");
      return 1;
    }
    std::printf("OK   loaded inline sample\n");
  }

  if (map.width <= 0 || map.height <= 0) {
    std::printf("FAIL dimensions\n");
    return 1;
  }
  std::printf("OK   size %dx%d tiles=%u\n", map.width, map.height, (unsigned)map.tiles.size());

  if (static_cast<int>(map.tiles.size()) < map.width * map.height) {
    std::printf("FAIL tile count\n");
    return 1;
  }

  // Original bunker is 225x74
  if (map.width == 225 && map.height == 74) {
    std::printf("OK   matches original bunker dimensions\n");
  } else {
    std::printf("NOTE dimensions are sample/not original (%dx%d)\n", map.width, map.height);
  }

  int fail = 0;
  if (map.is_blocked_world(-8.f, 0.f)) {
    std::printf("OK   OOB blocked\n");
  } else {
    std::printf("FAIL OOB should block\n");
    ++fail;
  }
  bool found_floor = false, found_wall = false;
  int nonzero = 0;
  for (int y = 0; y < map.height; ++y) {
    for (int x = 0; x < map.width; ++x) {
      const int id = map.tile_at(x, y);
      if (id != 0) ++nonzero;
      const bool blocked = map.is_blocked_world(x * 8.f + 4.f, y * 8.f + 4.f);
      if (!blocked) found_floor = true;
      else if (id > 0) found_wall = true;
    }
  }
  if (nonzero > 1000) std::printf("OK   decoded non-empty tiles (%d)\n", nonzero);
  else { std::printf("FAIL RLE decode sparse nonzero=%d\n", nonzero); ++fail; }
  if (found_floor) std::printf("OK   floor walkable\n");
  else { std::printf("FAIL no walkable floor\n"); ++fail; }
  if (found_wall) std::printf("OK   wall solid\n");
  else { std::printf("FAIL no solid wall sample\n"); ++fail; }

  return fail ? 1 : 0;
}
