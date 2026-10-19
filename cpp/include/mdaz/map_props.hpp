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
  // Placed instances (layout "Tutorial"): def = kPropDefs index, frame = instance frame.
  void build_list(const MapProp* in, int n);
  int count() const { return count_; }
  void set_frame(int i, short f) { if (i >= 0 && i < count_) props_[i].frame = f; }
  // C2 berry bush (t287) item for a prop def: red 27, blue 74, yellow 73; 0 if not a bush.
  static int berry_item(short def) { return def == 0 ? 27 : def == 1 ? 74 : def == 2 ? 73 : 0; }
  const MapProp& at(int i) const { return props_[i]; }
  const PropFrame* frame_of(const MapProp& p) const;
  // Props whose origin lies in mapgen cell (ax, ay): [first, first + n).
  void cell_range(int ax, int ay, int& first, int& n) const;
  // Movement a->b blocked by a Solid base polygon of a prop near b.
  bool blocks(Vec2 a, Vec2 b, float r) const;
  int solid_count() const;
  // C2 car_loot_point (t232): spawned at the car's Imagepoint 1 when IV3 = 0 (Map) / at layout
  // start (Tutorial); destroyed when opened. Not saved (like bush frames).
  bool trunk_alive(int i) const { return i >= 0 && i < count_ && (trunk_[i >> 3] & (1u << (i & 7))) != 0; }
  void trunk_take(int i) { if (i >= 0 && i < count_) trunk_[i >> 3] &= static_cast<unsigned char>(~(1u << (i & 7))); }
  int trunk_frame(int i) const;            // car_loot_point frame 0..5, -1 if none
  Vec2 trunk_pos(int i) const;             // car_loot_point origin
  Vec2 trunk_loot_pos(int i) const;        // its "loot" image point (Spawn_drop position)
  bool trunk_overlaps(int i, Vec2 c, float r) const;  // circle vs car_loot_point box
  bool trunk_vertical(int i) const;        // car_loot_point IV1 (vertical cars)
  int nearest_trunk(Vec2 p, float& d2) const;
  // SetVisible(1) when it becomes the available interaction; never hidden again (no SetVisible 0
  // in data.js; the LOOT_respawn template instance is invisible).
  bool trunk_visible(int i) const { return i >= 0 && i < count_ && (trunk_vis_[i >> 3] & (1u << (i & 7))) != 0; }
  void trunk_show(int i) { if (i >= 0 && i < count_) trunk_vis_[i >> 3] |= static_cast<unsigned char>(1u << (i & 7)); }
  bool trunk_sprite(int i, const char*& sheet, int& sx, int& sy, int& w, int& h, float& hx, float& hy, bool& mirror) const;
private:
  void mark_trunks(bool tutorial);
  unsigned char trunk_[kMax / 8 + 1];
  unsigned char trunk_vis_[kMax / 8 + 1];
  MapProp props_[kMax];
  int count_;
  int cell_first_[257];
};

} // namespace mdaz
