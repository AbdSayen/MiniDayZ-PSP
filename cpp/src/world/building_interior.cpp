#include "mdaz/building_interior.hpp"

namespace mdaz {
namespace {
#include "building_interior.inc"

float cross(Vec2 a, Vec2 b, Vec2 c) { return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x); }
bool seg_x(Vec2 a, Vec2 b, Vec2 c, Vec2 d) {
  const float d1 = cross(c, d, a), d2 = cross(c, d, b), d3 = cross(a, b, c), d4 = cross(a, b, d);
  return ((d1 > 0) != (d2 > 0)) && ((d3 > 0) != (d4 > 0));
}
float d2_seg(Vec2 p, Vec2 a, Vec2 b) {
  const float vx = b.x - a.x, vy = b.y - a.y, l2 = vx * vx + vy * vy;
  float t = l2 > 0.f ? ((p.x - a.x) * vx + (p.y - a.y) * vy) / l2 : 0.f;
  t = t < 0.f ? 0.f : (t > 1.f ? 1.f : t);
  const float dx = a.x + vx * t - p.x, dy = a.y + vy * t - p.y;
  return dx * dx + dy * dy;
}
} // namespace

int building_interior_count() { return kInteriorCount; }
const BuildingInterior* building_interior(int i) {
  return (i >= 0 && i < kInteriorCount) ? &kInteriors[i] : 0;
}
const BuildingInterior* building_interior_by_int_type(int t) {
  for (int i = 0; i < kInteriorCount; ++i)
    if (kInteriors[i].int_type == t) return &kInteriors[i];
  return 0;
}
const InteriorProp& interior_prop(const BuildingInterior& b, int k) { return kIntProps[b.first_prop + k]; }
const InteriorLoot& interior_loot(const BuildingInterior& b, int k) { return kIntLoot[b.first_loot + k]; }

bool interior_props_block(const BuildingInterior& bi, Vec2 o, Vec2 a, Vec2 b, float r,
                          bool big_only) {
  for (int k = 0; k < bi.prop_count; ++k) {
    const InteriorProp& p = kIntProps[bi.first_prop + k];
    if (big_only && !p.big) continue;
    if (p.pt_count < 3) continue;
    const Vec2 c(o.x + p.x, o.y + p.y);
    if ((b.x - c.x) * (b.x - c.x) + (b.y - c.y) * (b.y - c.y) > 160.f * 160.f) continue;
    bool inside = false;
    for (int i = 0; i < p.pt_count; ++i) {
      const int j = (i + 1) % p.pt_count;
      const Vec2 P(c.x + kIntPropPts[(p.first_pt + i) * 2], c.y + kIntPropPts[(p.first_pt + i) * 2 + 1]);
      const Vec2 Q(c.x + kIntPropPts[(p.first_pt + j) * 2], c.y + kIntPropPts[(p.first_pt + j) * 2 + 1]);
      if (seg_x(a, b, P, Q)) return true;
      if (d2_seg(b, P, Q) < r * r) return true;
      if (((P.y > b.y) != (Q.y > b.y)) && (b.x < (Q.x - P.x) * (b.y - P.y) / (Q.y - P.y) + P.x))
        inside = !inside;
    }
    if (inside) return true;
  }
  return false;
}

} // namespace mdaz

#include "mdaz/los.hpp"
namespace mdaz {
namespace { LosBlockedFn g_los_fn = 0; const void* g_los_ctx = 0; }
void set_los_blocker(LosBlockedFn fn, const void* ctx) { g_los_fn = fn; g_los_ctx = ctx; }
void clear_los_blocker(const void* ctx) { if (g_los_ctx == ctx) { g_los_fn = 0; g_los_ctx = 0; } }
bool los_blocked(Vec2 a, Vec2 b) { return g_los_fn ? g_los_fn(g_los_ctx, a, b) : false; }
} // namespace mdaz
