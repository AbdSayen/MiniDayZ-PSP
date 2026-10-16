#pragma once

#include <string>
#include <vector>

namespace mdaz {

// Loader for Construct 2 tilemap RLE used by original_docs/bunker_json
struct BunkerMap {
  int width;
  int height;
  std::vector<int> tiles; // row-major, length width*height

  BunkerMap();
  bool load_from_file(const char* path);
  bool load_from_c2_json(const char* json);
  int tile_at(int x, int y) const;
};

} // namespace mdaz
