#pragma once
// C2 create_map_element, map_element IV1 = 7 (buildings): per building subtype the *_int object,
// its Solid props (obst_base / big_obstacle_base at image points, with a fixed frame) and its
// spawn_loot_point positions + loot types. Generated from data.js (building_interior.inc).
#include "mdaz/types.hpp"

namespace mdaz {

struct InteriorProp {
  float x, y;      // offset from the *_int hotspot
  int big;         // 1 = big_obstacle_base (t339), 0 = obst_base (t336)
  int frame;
  int first_pt, pt_count;
};
struct InteriorLoot {
  float x, y;
  int loot_type;   // spawn_loot_point IV0 (Trigger_spawn table), -1 unknown
};
struct BuildingInterior {
  int subtype;     // map_element IV2
  int int_type;    // C2 object type of the *_int (== BuildingShape::c2_type)
  const char* int_sprite;
  int first_prop, prop_count;
  int first_loot, loot_count;
};

int building_interior_count();
const BuildingInterior* building_interior(int i);
const BuildingInterior* building_interior_by_int_type(int c2_type);
const InteriorProp& interior_prop(const BuildingInterior& b, int k);
const InteriorLoot& interior_loot(const BuildingInterior& b, int k);
// Movement a->b blocked by any Solid prop of building b placed at origin (hotspot).
bool interior_props_block(const BuildingInterior& b, Vec2 origin, Vec2 a, Vec2 bpos, float r);

} // namespace mdaz
