#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

struct SectorCoord {
  int sx;
  int sy;
};

class World {
public:
  World();

  void reset();
  SectorCoord sector_at(Vec2 world_pos) const;
  Vec2 sector_origin(SectorCoord c) const;
  float sector_width() const { return kMapLayoutW / static_cast<float>(kMapSectorsX); }
  float sector_height() const { return kMapLayoutH / static_cast<float>(kMapSectorsY); }

  // Simple obstacle grid stub (solid cells). Full map gen from C2 "Map generation" TBD.
  bool is_blocked(float x, float y) const;
  void set_blocked(int cell_x, int cell_y, bool blocked);
  void set_blocked_world(float x, float y, bool blocked);

  static const int kGridW = 256;
  static const int kGridH = 256;

private:
  bool blocked_[kGridW * kGridH];
  float cell_w_;
  float cell_h_;
};

} // namespace mdaz
