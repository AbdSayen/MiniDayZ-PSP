#pragma once
// C2 building collision: every *_int object (layer Buildings_base) has the Solid behaviour and a
// single collision polygon per frame; the exterior roof (layer Buildings) is spawned at the
// interior's image point 0 (= its origin) and has no Solid. Data generated from data.js.
#include "mdaz/types.hpp"
#include "mdaz/mapgen.hpp"

namespace mdaz {

struct BuildingShape {
  const char* int_sprite;
  const char* ext_sprite;
  int int_w, int_h;
  float int_hx, int_hy;     // hotspot (fraction of frame)
  int ext_w, ext_h;
  float ext_hx, ext_hy;
  int first_pt, pt_count;   // into the point table (pixel offsets from the hotspot)
  int c2_type;
  float wz_x0, wz_y0, wz_x1, wz_y1; // warmzone rect (w1..w2 image points), offsets from hotspot
  int has_wz;
};

int building_shape_count();
const BuildingShape* building_shape(int i);
const BuildingShape* building_shape_by_int_sprite(const char* int_sprite);
// Polygon vertex k of shape s (offset from the hotspot, C2 px).
Vec2 building_shape_pt(const BuildingShape& s, int k);

// True if moving a point from a to b (both world) hits the Solid polygon of shape s placed with
// its hotspot at origin: the step crosses an edge, b lies within `r` px of an edge, or b lies
// inside the polygon. r = 1 matches the C2 player_base collision (2x2 circle, AltMove).
bool building_blocks_move(const BuildingShape& s, Vec2 origin, Vec2 a, Vec2 b, float r);
// Point inside the warmzone rectangle (C2 indoor zone).
bool building_in_warmzone(const BuildingShape& s, Vec2 origin, Vec2 p);

} // namespace mdaz
