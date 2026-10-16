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
  return 0;
}
