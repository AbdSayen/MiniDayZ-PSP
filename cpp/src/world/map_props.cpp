#include "mdaz/map_props.hpp"
#include "mdaz/c2_mapgen.hpp"
#include "mdaz/rng.hpp"

#include <cmath>

namespace mdaz {
namespace {
#include "map_props.inc"

const PropDef* find_def(int cat, int sub, int iv6, int& idx) {
  for (int i = 0; i < kPropDefCount; ++i) {
    const PropDef& d = kPropDefs[i];
    if (d.cat != cat) continue;
    if (d.sub == -12 ? !(sub < 12) : d.sub != sub) continue;
    if (d.iv6 >= 0 && d.iv6 != iv6) continue;
    idx = i;
    return &d;
  }
  return 0;
}

bool seg_x(Vec2 a, Vec2 b, Vec2 c, Vec2 d) {
  const float d1 = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
  const float d2 = (b.x - a.x) * (d.y - a.y) - (b.y - a.y) * (d.x - a.x);
  const float d3 = (d.x - c.x) * (a.y - c.y) - (d.y - c.y) * (a.x - c.x);
  const float d4 = (d.x - c.x) * (b.y - c.y) - (d.y - c.y) * (b.x - c.x);
  return ((d1 > 0) != (d2 > 0)) && ((d3 > 0) != (d4 > 0));
}
float d2_seg(Vec2 p, Vec2 a, Vec2 b) {
  const float vx = b.x - a.x, vy = b.y - a.y, l = vx * vx + vy * vy;
  float t = l > 0.f ? ((p.x - a.x) * vx + (p.y - a.y) * vy) / l : 0.f;
  t = t < 0.f ? 0.f : (t > 1.f ? 1.f : t);
  const float dx = a.x + vx * t - p.x, dy = a.y + vy * t - p.y;
  return dx * dx + dy * dy;
}
bool poly_blocks(const PropBase& pb, Vec2 o, Vec2 a, Vec2 b, float r) {
  if (pb.n < 3) return false;
  bool inside = false;
  for (int k = 0; k < pb.n; ++k) {
    const int j = (k + 1) % pb.n;
    const Vec2 p(o.x + pb.ox + pb.pts[k * 2], o.y + pb.oy + pb.pts[k * 2 + 1]);
    const Vec2 q(o.x + pb.ox + pb.pts[j * 2], o.y + pb.oy + pb.pts[j * 2 + 1]);
    if (seg_x(a, b, p, q)) return true;
    if (r > 0.f && d2_seg(b, p, q) < r * r) return true;
    if (((p.y > b.y) != (q.y > b.y)) && (b.x < (q.x - p.x) * (b.y - p.y) / (q.y - p.y) + p.x))
      inside = !inside;
  }
  return inside;
}
int cell_of(float x, float y) {
  int ay = static_cast<int>(std::floor((x - kC2CellOriginX) / kC2CellSize));
  int ax = static_cast<int>(std::floor((y - kC2CellOriginY) / kC2CellSize));
  ay = ay < 0 ? 0 : (ay > 15 ? 15 : ay);
  ax = ax < 0 ? 0 : (ax > 15 ? 15 : ax);
  return ax * 16 + ay;
}
} // namespace

void MapProps::build(const C2Map& m, Rng& rng) {
  count_ = 0;
  // Bucket by mapgen cell so collision / draw only scan nearby cells.
  static MapProp tmp[kMax];
  static unsigned short tcell[kMax];
  int n = 0;
  for (int r = 0; r < m.row_count && n < kMax; ++r) {
    const C2ElementRow& row = m.rows[r];
    const bool want = row.cat == 3 || row.cat == 4 || row.cat == 5 || row.cat == 6 ||
                      (row.cat == 7 && row.sub == 1);
    if (!want) continue;
    int di = -1;
    const PropDef* d = find_def(row.cat, row.sub, row.c6, di);
    if (!d) continue;
    int f = d->fconst;
    if (d->fmode == 1) f = row.c6;
    else if (d->fmode == 2) f = row.sub;
    else if (d->fmode == 3 && row.c5 == 0) f = rng.next_int(0, 1);  // choose(0,1) when IV3 = 0
    if (d->f_iv3_1 >= 0 && row.c5 == 1) f = d->f_iv3_1;
    const PropSheet& s = kPropSheets[d->sheet];
    if (f < 0 || f >= s.count) f = 0;
    tmp[n].x = row.x; tmp[n].y = row.y;
    tmp[n].def = static_cast<short>(di); tmp[n].frame = static_cast<short>(f);
    tcell[n] = static_cast<unsigned short>(cell_of(row.x, row.y));
    ++n;
  }
  int cnt[256] = {0};
  for (int i = 0; i < n; ++i) ++cnt[tcell[i]];
  cell_first_[0] = 0;
  for (int c = 0; c < 256; ++c) cell_first_[c + 1] = cell_first_[c] + cnt[c];
  int fill[256];
  for (int c = 0; c < 256; ++c) fill[c] = cell_first_[c];
  for (int i = 0; i < n; ++i) props_[fill[tcell[i]]++] = tmp[i];
  count_ = n;
}

void MapProps::build_list(const MapProp* in, int n_in) {
  // Placed layout instances (C2 layout "Tutorial"): same cell bucketing as build().
  static MapProp tmp[kMax];
  static unsigned short tcell[kMax];
  int n = 0;
  for (int i = 0; i < n_in && n < kMax; ++i) {
    if (in[i].def < 0 || in[i].def >= static_cast<int>(sizeof(kPropDefs) / sizeof(kPropDefs[0])))
      continue;
    tmp[n] = in[i];
    const PropSheet& s = kPropSheets[kPropDefs[in[i].def].sheet];
    if (tmp[n].frame < 0 || tmp[n].frame >= s.count) tmp[n].frame = 0;
    tcell[n] = static_cast<unsigned short>(cell_of(in[i].x, in[i].y));
    ++n;
  }
  int cnt[256] = {0};
  for (int i = 0; i < n; ++i) ++cnt[tcell[i]];
  cell_first_[0] = 0;
  for (int c = 0; c < 256; ++c) cell_first_[c + 1] = cell_first_[c] + cnt[c];
  int fill[256];
  for (int c = 0; c < 256; ++c) fill[c] = cell_first_[c];
  for (int i = 0; i < n; ++i) props_[fill[tcell[i]]++] = tmp[i];
  count_ = n;
}

const PropFrame* MapProps::frame_of(const MapProp& p) const {
  const PropSheet& s = kPropSheets[kPropDefs[p.def].sheet];
  return &kPropFrames[s.first + p.frame];
}

void MapProps::cell_range(int ax, int ay, int& first, int& n) const {
  if (ax < 0 || ay < 0 || ax > 15 || ay > 15 || count_ == 0) { first = 0; n = 0; return; }
  const int c = ax * 16 + ay;
  first = cell_first_[c];
  n = cell_first_[c + 1] - first;
}

bool MapProps::blocks(Vec2 a, Vec2 b, float r) const {
  if (count_ == 0) return false;
  const int c = cell_of(b.x, b.y), cax = c / 16, cay = c % 16;
  for (int ax = cax - 1; ax <= cax + 1; ++ax)
    for (int ay = cay - 1; ay <= cay + 1; ++ay) {
      int f = 0, n = 0;
      cell_range(ax, ay, f, n);
      for (int i = f; i < f + n; ++i) {
        const MapProp& p = props_[i];
        const float dx = b.x - p.x, dy = b.y - p.y;
        if (dx * dx + dy * dy > 300.f * 300.f) continue;
        const PropDef& d = kPropDefs[p.def];
        for (int k = 0; k < d.base_count; ++k) {
          const PropBase& pb = kPropBases[d.first_base + k];
          if (pb.mframe >= 0 && pb.mframe != p.frame) continue;
          if (poly_blocks(pb, Vec2(p.x, p.y), a, b, r)) return true;
        }
      }
    }
  return false;
}

int MapProps::solid_count() const {
  int s = 0;
  for (int i = 0; i < count_; ++i) {
    const PropDef& d = kPropDefs[props_[i].def];
    for (int k = 0; k < d.base_count; ++k) {
      const PropBase& pb = kPropBases[d.first_base + k];
      if (pb.mframe < 0 || pb.mframe == props_[i].frame) { ++s; break; }
    }
  }
  return s;
}

} // namespace mdaz
