#pragma once
// C2 create_map_location tilemaps: per mapgen cell ground tilemap (t475/681/682/771 by
// CurrentLevel, layer bg), ground_enviroment (t221, 30 px) and the optional Water tilemap (t199),
// each created at mapgen XY - 500 from its first layout instance (template) and then edited with
// SetTile / SetTileRange / tile-compare autotiling. Generated from data.js by
// tools/codegen_cml.py (src/world/create_map_location_tiles.inc).
#include "mdaz/types.hpp"

namespace mdaz {

struct C2Map;
class Rng;
class IPlatform;

class CellTilemaps {
public:
  static const int kSlots = 40;
  static const int kG = 18;   // 1050 / 60 -> 18 columns (last one half visible)
  static const int kE = 35;   // 1050 / 30
  struct Slot {
    short cell;          // mapgen cell index (ay * 16 + ax), -1 = free
    signed char gkind;   // 0..3 = ground_lvl1..4, -1 none
    bool has_env, has_water;
    float x, y;          // tilemap top-left (mapgen XY - 500)
    signed char g[kG * kG];
    signed char e[kE * kE];
    signed char w[kG * kG];
  };
  CellTilemaps();
  void clear();
  // C2 Reload_locations: cells within R of the player are created (create_map_location), cells
  // beyond R are erased (runtime). start = Reload_locations_start/_aftersave (create only).
  void sync(const C2Map& m, Vec2 player, float R, bool start, int current_level, Rng& rng);
  // Runs create_map_location's tilemap part for one cell into slot s (public for tests).
  void build_cell(const C2Map& m, int cell, Slot& s, int current_level, Rng& rng);
  int active_count() const;
  const Slot* slot_for_cell(int cell) const;
  // Tile under (wx, wy): layer 0 ground, 1 enviroment, 2 water; -1 = none.
  int tile_at(int layer, float wx, float wy) const;
  void draw(IPlatform& p, int layer, Vec2 cam, float scale, float view_w, float view_h) const;
private:
  Slot slots_[kSlots];
  double vars_[32];
  bool vars_init_;
};

} // namespace mdaz
