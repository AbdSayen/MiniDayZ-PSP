#include "mdaz/world.hpp"

#include <cstring>

namespace mdaz {

World::World() {
  cell_w_ = kMapLayoutW / static_cast<float>(kGridW);
  cell_h_ = kMapLayoutH / static_cast<float>(kGridH);
  reset();
}

void World::reset() {
  std::memset(blocked_, 0, sizeof(blocked_));
}

SectorCoord World::sector_at(Vec2 world_pos) const {
  SectorCoord c;
  c.sx = static_cast<int>(world_pos.x / sector_width());
  c.sy = static_cast<int>(world_pos.y / sector_height());
  if (c.sx < 0) c.sx = 0;
  if (c.sy < 0) c.sy = 0;
  if (c.sx >= kMapSectorsX) c.sx = kMapSectorsX - 1;
  if (c.sy >= kMapSectorsY) c.sy = kMapSectorsY - 1;
  return c;
}

Vec2 World::sector_origin(SectorCoord c) const {
  return Vec2(static_cast<float>(c.sx) * sector_width(),
              static_cast<float>(c.sy) * sector_height());
}

bool World::is_blocked(float x, float y) const {
  int cx = static_cast<int>(x / cell_w_);
  int cy = static_cast<int>(y / cell_h_);
  if (cx < 0 || cy < 0 || cx >= kGridW || cy >= kGridH) return true;
  return blocked_[cy * kGridW + cx];
}

void World::set_blocked(int cell_x, int cell_y, bool blocked) {
  if (cell_x < 0 || cell_y < 0 || cell_x >= kGridW || cell_y >= kGridH) return;
  blocked_[cell_y * kGridW + cell_x] = blocked;
}

void World::set_blocked_world(float x, float y, bool blocked) {
  int cx = static_cast<int>(x / cell_w_);
  int cy = static_cast<int>(y / cell_h_);
  set_blocked(cx, cy, blocked);
}

} // namespace mdaz
