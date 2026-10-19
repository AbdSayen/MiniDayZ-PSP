#include "mdaz/cell_tilemaps.hpp"
#include "mdaz/c2_mapgen.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/platform.hpp"
#include "mdaz/balance_globals.hpp"
#include <cmath>
#include <cstring>

namespace mdaz {
namespace {

struct CmlCtx {
  Rng* rng;
  const C2Map* m;
  const C2Cell* cell;
  CellTilemaps::Slot* s;
  double cell_x, cell_y, uid;
  double rnd() { return static_cast<double>(rng->next_float()); }
  double rnd_range(double lo, double hi) { return lo + rnd() * (hi - lo); }
  int pick(int n) { int i = static_cast<int>(rnd() * n); return i >= n ? n - 1 : i; }
  double iv(int k) const {
    if (k == 0) return cell->type;
    if (k == 1) return cell->ay;
    if (k == 2) return cell->ax;
    if (k == 9) return cell->iv9 ? 1 : 0;
    return 0;
  }
  double grid(double x, double y) const {
    const int ix = static_cast<int>(std::floor(x)), iy = static_cast<int>(std::floor(y));
    if (ix < 0 || iy < 0 || ix >= kC2Grid || iy >= kC2Grid) return 0;
    return m->arr[ix][iy];
  }
  void set(double, double, double) {}  // array 518 write: not part of the tilemap pass
  // k: 0..3 ground (created kind), 9 = family pl14 ground (picked by mapgen IV6), 4 env, 5 water.
  signed char* layer(int k, int& n) {
    if (k == 4) { n = CellTilemaps::kE; return s->has_env ? s->e : 0; }
    if (k == 5) { n = CellTilemaps::kG; return s->has_water ? s->w : 0; }
    n = CellTilemaps::kG;
    return s->gkind >= 0 ? s->g : 0;
  }
  void create(int k, double x, double y) {
    s->x = static_cast<float>(x); s->y = static_cast<float>(y);
    if (k <= 3) {
      // Templates: t475 = Polygon layout instance (tile 6 over 18x18 visible cells);
      // t681/682/771 = Map layout instances (17x17 tile 6, rest empty).
      s->gkind = static_cast<signed char>(k);
      const int lim = (k == 0) ? CellTilemaps::kG : 17;
      for (int r = 0; r < CellTilemaps::kG; ++r)
        for (int c = 0; c < CellTilemaps::kG; ++c)
          s->g[r * CellTilemaps::kG + c] = (r < lim && c < lim) ? 6 : -1;
    } else if (k == 4) {
      s->has_env = true;  // Map layout template: all empty
      std::memset(s->e, -1, sizeof(s->e));
    } else if (k == 5) {
      s->has_water = true;  // template is resized to 1050 and erased 17x17 right after
      std::memset(s->w, -1, sizeof(s->w));
    }
  }
  void set_tile(int k, double x, double y, double t) {
    int n = 0; signed char* L = layer(k, n);
    if (!L) return;
    const int ix = static_cast<int>(std::floor(x)), iy = static_cast<int>(std::floor(y));
    if (ix < 0 || iy < 0 || ix >= n || iy >= n) return;
    L[iy * n + ix] = static_cast<signed char>(t);
  }
  void set_range(int k, double x, double y, double w, double h, double t) {
    const int iw = static_cast<int>(w), ih = static_cast<int>(h);
    for (int dy = 0; dy < ih; ++dy)
      for (int dx = 0; dx < iw; ++dx) set_tile(k, x + dx, y + dy, t);
  }
  void erase_range(int k, double x, double y, double w, double h) { set_range(k, x, y, w, h, -1); }
  void set_size(int, double, double) {}
  double tile(int k, double x, double y) {
    int n = 0; signed char* L = layer(k, n);
    if (!L) return -1;
    const int ix = static_cast<int>(std::floor(x)), iy = static_cast<int>(std::floor(y));
    if (ix < 0 || iy < 0 || ix >= n || iy >= n) return -1;
    return L[iy * n + ix];
  }
};

double c2mod(double a, double b) { return b == 0 ? 0 : std::fmod(a, b); }
double c2int(double v) { return std::trunc(v); }

#include "../world/create_map_location_tiles.inc"

}  // namespace

CellTilemaps::CellTilemaps() : vars_init_(false) { clear(); }

void CellTilemaps::clear() {
  for (int i = 0; i < kSlots; ++i) { slots_[i].cell = -1; slots_[i].gkind = -1; }
  vars_init_ = false;
}

void CellTilemaps::build_cell(const C2Map& m, int cell, Slot& s, int current_level, Rng& rng) {
  if (!vars_init_) {
    for (int i = 0; i < 32; ++i) vars_[i] = 0;
    vars_[13] = balance::karma_points;
    vars_[12] = balance::Q_task_id;
    vars_[9] = balance::camps_bot_spawn_on;
    vars_init_ = true;
  }
  vars_[2] = current_level;
  s.cell = static_cast<short>(cell);
  s.gkind = -1; s.has_env = false; s.has_water = false;
  CmlCtx G;
  G.rng = &rng; G.m = &m; G.cell = &m.cells[cell]; G.s = &s;
  G.cell_x = G.cell->wx(); G.cell_y = G.cell->wy(); G.uid = cell;
  create_map_location_tiles(G, vars_);
}

void CellTilemaps::sync(const C2Map& m, Vec2 player, float R, bool start, int current_level, Rng& rng) {
  const float r2 = R * R;
  for (int ci = 0; ci < kC2Grid * kC2Grid; ++ci) {
    const C2Cell& c = m.cells[ci];
    const float dx = c.wx() - player.x, dy = c.wy() - player.y;
    const bool in = dx * dx + dy * dy <= r2;
    int have = -1;
    for (int i = 0; i < kSlots; ++i) if (slots_[i].cell == ci) { have = i; break; }
    if (in && have < 0) {
      int free = -1;
      for (int i = 0; i < kSlots; ++i) if (slots_[i].cell < 0) { free = i; break; }
      if (free >= 0) build_cell(m, ci, slots_[free], current_level, rng);
    } else if (!in && have >= 0 && !start) {
      slots_[have].cell = -1;  // erase_map_location
    }
  }
}

int CellTilemaps::active_count() const {
  int n = 0;
  for (int i = 0; i < kSlots; ++i) if (slots_[i].cell >= 0) ++n;
  return n;
}

const CellTilemaps::Slot* CellTilemaps::slot_for_cell(int cell) const {
  for (int i = 0; i < kSlots; ++i) if (slots_[i].cell == cell) return &slots_[i];
  return 0;
}

int CellTilemaps::tile_at(int layer, float wx, float wy) const {
  // Later-created slots draw on top; scan all and keep the last hit.
  int res = -1;
  for (int i = 0; i < kSlots; ++i) {
    const Slot& s = slots_[i];
    if (s.cell < 0) continue;
    const float tw = layer == 1 ? 30.f : 60.f;
    const int n = layer == 1 ? kE : kG;
    const signed char* L = layer == 0 ? (s.gkind >= 0 ? s.g : 0) : layer == 1 ? (s.has_env ? s.e : 0)
                                                                   : (s.has_water ? s.w : 0);
    if (!L) continue;
    if (wx < s.x || wy < s.y || wx >= s.x + 1050.f || wy >= s.y + 1050.f) continue;
    const int c = static_cast<int>((wx - s.x) / tw), r = static_cast<int>((wy - s.y) / tw);
    if (c >= n || r >= n) continue;
    if (L[r * n + c] >= 0) res = L[r * n + c];
  }
  return res;
}

void CellTilemaps::draw(IPlatform& p, int layer, Vec2 cam, float scale, float view_w, float view_h) const {
  static const char* const kG_img[4] = {"ground_lvl1_tilemap", "ground_lvl2_tilemap",
                                        "ground_lvl3_tilemap", "ground_lvl4_tilemap"};
  const float half_w = view_w * 0.5f / scale, half_h = view_h * 0.5f / scale;
  for (int i = 0; i < kSlots; ++i) {
    const Slot& s = slots_[i];
    if (s.cell < 0) continue;
    const char* img; const signed char* L; int n, cols; float tw;
    if (layer == 0) { if (s.gkind < 0) continue; img = kG_img[s.gkind]; L = s.g; n = kG; tw = 60.f; cols = 5; }
    else if (layer == 1) { if (!s.has_env) continue; img = "ground_enviroment_tilemap"; L = s.e; n = kE; tw = 30.f; cols = 5; }
    else { if (!s.has_water) continue; img = "tilemap_water"; L = s.w; n = kG; tw = 60.f; cols = 3; }
    if (!p.has_sprite(img)) continue;
    int c0 = static_cast<int>(std::floor((cam.x - half_w - s.x) / tw));
    int c1 = static_cast<int>(std::floor((cam.x + half_w - s.x) / tw));
    int r0 = static_cast<int>(std::floor((cam.y - half_h - s.y) / tw));
    int r1 = static_cast<int>(std::floor((cam.y + half_h - s.y) / tw));
    if (c0 < 0) c0 = 0;
    if (r0 < 0) r0 = 0;
    if (c1 >= n) c1 = n - 1;
    if (r1 >= n) r1 = n - 1;
    for (int r = r0; r <= r1; ++r)
      for (int c = c0; c <= c1; ++c) {
        const int id = L[r * n + c];
        if (id < 0) continue;
        // Visible width is 1050: the last (half) column / row is clipped.
        const float cw = (c * tw + tw > 1050.f) ? 1050.f - c * tw : tw;
        const float rh = (r * tw + tw > 1050.f) ? 1050.f - r * tw : tw;
        const float sx = view_w * 0.5f + (s.x + c * tw - cam.x) * scale;
        const float sy = view_h * 0.5f + (s.y + r * tw - cam.y) * scale;
        p.draw_sprite_frame(img, std::floor(sx), std::floor(sy), std::ceil(cw * scale) + 1.f,
                            std::ceil(rh * scale) + 1.f, (id % cols) * static_cast<int>(tw),
                            (id / cols) * static_cast<int>(tw), static_cast<int>(cw),
                            static_cast<int>(rh), 0.f, 1.f);
      }
  }
}

}  // namespace mdaz
