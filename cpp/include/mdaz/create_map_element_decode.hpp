// Verified from original_docs/data.js — create_map_element event (mapgen_props_decode.txt).
// Trigger: tilemap obj518 layer "zed_resp" tile id → obj501 IV10 before CallFunction recurse.
#pragma once

namespace mdaz {

struct MapElementIv10Row {
  int zed_resp_tile;
  int iv10;
};

static const int kMapElementIv10Count = 6;
static const MapElementIv10Row kMapElementIv10[kMapElementIv10Count] = {
    {0, 1},
    {1, 2},
    {2, 3},
    {3, 3},
    {5, 6},
    {6, 6},
};

inline int map_element_iv10_for_zed_resp_tile(int tile) {
  for (int i = 0; i < kMapElementIv10Count; ++i) {
    if (kMapElementIv10[i].zed_resp_tile == tile) return kMapElementIv10[i].iv10;
  }
  return 0;
}

// C2 map_element default animation (t501).
static const char kC2_map_element_sprite[] = "map_element-sheet0";

} // namespace mdaz
