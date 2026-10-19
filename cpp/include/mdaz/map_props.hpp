#pragma once
// C2 create_map_element categories 3 (berry bush / waterpump), 4 (obstacles / ee_*),
// 5 (pillars), 6 (cars) and 7:1 (b_deer_stand): the visible object spawned by map_element at its
// origin, its frame (constant / IV6 / IV2 / choose(0,1); cars frame 1 when IV3 = 1) and the
// Solid obst_base / big_obstacle_base polygons it spawns at its image points. Tables generated
// from data.js (map_props.inc, gen_props.py).
#include "mdaz/types.hpp"

namespace mdaz {

struct C2Map;
class Rng;

struct PropFrame { const char* sheet; int x, y, w, h; float hx, hy; };
struct PropSheet { int first, count; };
struct PropBase { int mframe, big, frame; float ox, oy; int n; float pts[16]; };
struct PropDef { int cat, sub, iv6, sheet, fmode, fconst, f_iv3_1, first_base, base_count; };

struct MapProp {
  float x, y;
  short def;
  short frame;
};

class MapProps {
public:
  static const int kMax = 6000;
  MapProps() : count_(0) {}
  void build(const C2Map& m, Rng& rng);
  int count() const { return count_; }
  const MapProp& at(int i) const { return props_[i]; }
  const PropFrame* frame_of(const MapProp& p) const;
  // Props whose origin lies in mapgen cell (ax, ay): [first, first + n).
  void cell_range(int ax, int ay, int& first, int& n) const;
  // Movement a->b blocked by a Solid base polygon of a prop near b.
  bool blocks(Vec2 a, Vec2 b, float r) const;
  int solid_count() const;
private:
  MapProp props_[kMax];
  int count_;
  int cell_first_[257];
};

} // namespace mdaz
