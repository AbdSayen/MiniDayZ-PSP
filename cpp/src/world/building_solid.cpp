#include "mdaz/building_solid.hpp"

#include <cstring>

namespace mdaz {

namespace {
#include "building_solid.inc"

float cross(float ax, float ay, float bx, float by) { return ax * by - ay * bx; }

bool seg_intersect(Vec2 p1, Vec2 p2, Vec2 q1, Vec2 q2) {
  const float rx = p2.x - p1.x, ry = p2.y - p1.y;
  const float sx = q2.x - q1.x, sy = q2.y - q1.y;
  const float den = cross(rx, ry, sx, sy);
  if (den == 0.f) return false;
  const float qpx = q1.x - p1.x, qpy = q1.y - p1.y;
  const float t = cross(qpx, qpy, sx, sy) / den;
  const float u = cross(qpx, qpy, rx, ry) / den;
  return t >= 0.f && t <= 1.f && u >= 0.f && u <= 1.f;
}

float dist2_point_seg(Vec2 p, Vec2 a, Vec2 b) {
  const float abx = b.x - a.x, aby = b.y - a.y;
  const float l2 = abx * abx + aby * aby;
  float t = l2 > 0.f ? ((p.x - a.x) * abx + (p.y - a.y) * aby) / l2 : 0.f;
  if (t < 0.f) t = 0.f;
  if (t > 1.f) t = 1.f;
  const float dx = p.x - (a.x + abx * t), dy = p.y - (a.y + aby * t);
  return dx * dx + dy * dy;
}
} // namespace

int building_shape_count() { return kShapeCount; }

const BuildingShape* building_shape(int i) {
  return (i >= 0 && i < kShapeCount) ? &kShapes[i] : 0;
}

const BuildingShape* building_shape_by_int_sprite(const char* s) {
  if (!s) return 0;
  for (int i = 0; i < kShapeCount; ++i)
    if (std::strcmp(kShapes[i].int_sprite, s) == 0) return &kShapes[i];
  return 0;
}

Vec2 building_shape_pt(const BuildingShape& s, int k) {
  const int i = s.first_pt + (k % s.pt_count);
  return Vec2(kPts[i * 2], kPts[i * 2 + 1]);
}

bool building_blocks_move(const BuildingShape& s, Vec2 o, Vec2 a, Vec2 b, float r) {
  if (s.pt_count < 2) return false;
  // Cheap reject: b outside the interior frame (+margin) and a too.
  const float x0 = o.x - s.int_hx * s.int_w - 4.f, y0 = o.y - s.int_hy * s.int_h - 4.f;
  const float x1 = x0 + s.int_w + 8.f, y1 = y0 + s.int_h + 8.f;
  if ((b.x < x0 || b.x > x1 || b.y < y0 || b.y > y1) &&
      (a.x < x0 || a.x > x1 || a.y < y0 || a.y > y1))
    return false;
  bool inside = false;
  const float r2 = r * r;
  for (int k = 0; k < s.pt_count; ++k) {
    Vec2 p = building_shape_pt(s, k), q = building_shape_pt(s, k + 1);
    p.x += o.x; p.y += o.y; q.x += o.x; q.y += o.y;
    if (seg_intersect(a, b, p, q)) return true;
    if (dist2_point_seg(b, p, q) < r2) return true;
    if (((p.y > b.y) != (q.y > b.y)) &&
        (b.x < (q.x - p.x) * (b.y - p.y) / (q.y - p.y) + p.x))
      inside = !inside;
  }
  return inside;
}

bool building_in_warmzone(const BuildingShape& s, Vec2 o, Vec2 p) {
  return p.x >= o.x + s.wz_x0 && p.x <= o.x + s.wz_x1 && p.y >= o.y + s.wz_y0 &&
         p.y <= o.y + s.wz_y1;
}

} // namespace mdaz
