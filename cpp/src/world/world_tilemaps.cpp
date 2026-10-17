#include "mdaz/world_tilemaps.hpp"

#include "mdaz/mapgen.hpp"
#include "mdaz/platform.hpp"
#include "mdaz/types.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace mdaz {

namespace {

bool next_int(const char*& p, const char* end, int& out) {
  while (p < end && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')) ++p;
  if (p >= end) return false;
  char* stop = 0;
  const long v = std::strtol(p, &stop, 10);
  if (stop == p) return false;
  p = stop;
  out = static_cast<int>(v);
  return true;
}

bool next_word(const char*& p, const char* end, char* out, int cap) {
  while (p < end && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')) ++p;
  int n = 0;
  while (p < end && *p != ' ' && *p != '\n' && *p != '\r' && *p != '\t') {
    if (n < cap - 1) out[n++] = *p;
    ++p;
  }
  out[n] = 0;
  return n > 0;
}

// Full-map runtime overlays (C2 resizes live tilemaps as SetTile lands).
const int kOverlayCols = 298; // ceil(17850/60)
const int kOverlayRows = 334; // ceil(20000/60)
const int kTile = 60;

} // namespace

WorldTilemaps::WorldTilemaps()
  : count_(0), static_count_(0), tile_count_(0), static_tile_count_(0) {}

bool WorldTilemaps::load(IPlatform& platform, const char* path) {
  count_ = 0;
  tile_count_ = 0;
  static_count_ = 0;
  static_tile_count_ = 0;
  char* data = 0;
  std::size_t size = 0;
  if (!platform.read_file(path, &data, &size) || !data) return false;
  const char* p = data;
  const char* end = data + size;
  int n = 0;
  bool ok = next_int(p, end, n);
  for (int i = 0; ok && i < n; ++i) {
    Map m;
    int x = 0, y = 0;
    ok = next_word(p, end, m.image, sizeof(m.image)) && next_int(p, end, x) && next_int(p, end, y) &&
         next_int(p, end, m.cols) && next_int(p, end, m.rows) && next_int(p, end, m.tile_w) &&
         next_int(p, end, m.tile_h) && next_int(p, end, m.atlas_cols);
    if (!ok) break;
    m.x = static_cast<float>(x);
    m.y = static_cast<float>(y);
    m.first = tile_count_;
    const int cells = m.cols * m.rows;
    const bool fits = count_ < kMaxMaps && tile_count_ + cells <= kMaxTiles && m.atlas_cols > 0;
    for (int k = 0; ok && k < cells; ++k) {
      int v = -1;
      ok = next_int(p, end, v);
      if (fits) tiles_[tile_count_ + k] = static_cast<short>(v < 0 ? -1 : (v & 0x1FFFFFFF));
    }
    if (ok && fits) {
      tile_count_ += cells;
      maps_[count_++] = m;
    }
  }
  delete[] data;
  mark_static();
  return ok && count_ > 0;
}

void WorldTilemaps::mark_static() {
  static_count_ = count_;
  static_tile_count_ = tile_count_;
}

void WorldTilemaps::clear_runtime() {
  count_ = static_count_;
  tile_count_ = static_tile_count_;
}

bool WorldTilemaps::alloc_map(const char* image, float x, float y, int cols, int rows,
                              int tile_w, int tile_h, int atlas_cols, int fill_id) {
  if (!image || cols <= 0 || rows <= 0 || atlas_cols <= 0) return false;
  if (count_ >= kMaxMaps) return false;
  const int cells = cols * rows;
  if (tile_count_ + cells > kMaxTiles) return false;
  Map& m = maps_[count_];
  std::strncpy(m.image, image, sizeof(m.image) - 1);
  m.image[sizeof(m.image) - 1] = 0;
  m.x = x;
  m.y = y;
  m.cols = cols;
  m.rows = rows;
  m.tile_w = tile_w;
  m.tile_h = tile_h;
  m.atlas_cols = atlas_cols;
  m.first = tile_count_;
  for (int i = 0; i < cells; ++i)
    tiles_[tile_count_ + i] = static_cast<short>(fill_id);
  tile_count_ += cells;
  ++count_;
  return true;
}

bool WorldTilemaps::stamp_fill(const char* image, float world_x, float world_y,
                               int cols, int rows, int tile_id,
                               int tile_w, int tile_h, int atlas_cols) {
  // Prefer writing into a full-map runtime overlay of the same image.
  for (int i = static_count_; i < count_; ++i) {
    Map& m = maps_[i];
    if (std::strcmp(m.image, image) != 0) continue;
    if (m.cols < 64 || m.rows < 64) continue; // skip tiny leftovers
    const int c0 = static_cast<int>(std::floor((world_x - m.x) / m.tile_w));
    const int r0 = static_cast<int>(std::floor((world_y - m.y) / m.tile_h));
    for (int r = 0; r < rows; ++r) {
      for (int c = 0; c < cols; ++c) {
        const int cc = c0 + c;
        const int rr = r0 + r;
        if (cc < 0 || rr < 0 || cc >= m.cols || rr >= m.rows) continue;
        tiles_[m.first + rr * m.cols + cc] = static_cast<short>(tile_id);
      }
    }
    return true;
  }
  return alloc_map(image, std::floor(world_x / tile_w) * tile_w,
                   std::floor(world_y / tile_h) * tile_h,
                   cols, rows, tile_w, tile_h, atlas_cols, tile_id);
}

bool WorldTilemaps::stamp_tile(const char* image, float world_x, float world_y, int tile_id,
                               int tile_w, int tile_h, int atlas_cols) {
  return stamp_fill(image, world_x, world_y, 1, 1, tile_id, tile_w, tile_h, atlas_cols);
}

void WorldTilemaps::stamp_from_mapgen(const MapGenerator& gen) {
  clear_runtime();
  // One full-map ground overlay (empty = -1) + one water overlay for puddles.
  // Cell budget: 298*334*2 ≈ 199k — too big for kMaxTiles 120k.
  // Use ground overlay only at full size; water as small local fills.
  if (!alloc_map("ground_lvl1_tilemap", 0.f, 0.f, kOverlayCols, kOverlayRows,
                 kTile, kTile, 5, -1)) {
    return;
  }

  auto set_ground = [&](float wx, float wy, int tile) {
    stamp_tile("ground_lvl1_tilemap", wx, wy, tile);
  };
  auto fill_ground = [&](float wx, float wy, int cols, int rows, int tile) {
    stamp_fill("ground_lvl1_tilemap", wx, wy, cols, rows, tile);
  };

  // C2 create_map_location: ground SetTile → 15; water → 4.
  for (int i = 0; i < gen.location_count(); ++i) {
    const MapLocation& L = gen.location(i);
    int pad = 7;
    if (L.type == Loc_City || L.type == Loc_Military) pad = 15;
    else if (L.type == Loc_Hospital || L.type == Loc_Firestation) pad = 13;
    else if (L.type == Loc_Village || L.type == Loc_Police || L.type == Loc_Church) pad = 11;
    else if (L.type == Loc_GasStation || L.type == Loc_Helicrash) pad = 9;
    else if (L.type == Loc_BunkerEntrance || L.type == Loc_Secret) pad = 5;

    const float half = static_cast<float>(pad) * 0.5f * static_cast<float>(kTile);
    fill_ground(L.pos.x - half, L.pos.y - half, pad, pad, 15);

    // Cross strips matching common SetTileRange sizes (17×2, 2×17, 9×2, 2×9).
    fill_ground(L.pos.x - 8.5f * kTile, L.pos.y - kTile, 17, 2, 15);
    fill_ground(L.pos.x - kTile, L.pos.y - 8.5f * kTile, 2, 17, 15);
    fill_ground(L.pos.x - 4.5f * kTile, L.pos.y - kTile, 9, 2, 15);
    fill_ground(L.pos.x - kTile, L.pos.y - 4.5f * kTile, 2, 9, 15);

    if (L.type == Loc_Helicrash) {
      stamp_fill("tilemap_water", L.pos.x + 120.f, L.pos.y - 60.f, 3, 3, 4, 60, 60, 3);
    }
  }

  // Dirt corridors ≈ generate_array_roads between nearby sector POIs.
  for (int i = 0; i < gen.location_count(); ++i) {
    const MapLocation& A = gen.location(i);
    for (int j = i + 1; j < gen.location_count(); ++j) {
      const MapLocation& B = gen.location(j);
      const int manh = std::abs(A.sector_x - B.sector_x) + std::abs(A.sector_y - B.sector_y);
      if (manh < 1 || manh > 3) continue;
      const float dx = B.pos.x - A.pos.x;
      const float dy = B.pos.y - A.pos.y;
      const float len = std::sqrt(dx * dx + dy * dy);
      if (len < 200.f) continue;
      const int steps = static_cast<int>(len / static_cast<float>(kTile));
      for (int s = 0; s <= steps; ++s) {
        const float t = static_cast<float>(s) / static_cast<float>(steps > 0 ? steps : 1);
        const float wx = A.pos.x + dx * t;
        const float wy = A.pos.y + dy * t;
        set_ground(wx - kTile, wy, 15);
        set_ground(wx, wy, 15);
        set_ground(wx + kTile, wy, 15);
        set_ground(wx, wy - kTile, 15);
        set_ground(wx, wy + kTile, 15);
      }
    }
  }

  // C2 map_element IV==4 + tilemap_water SetTile id 4; river locals unused in export — seed band.
  const RiverParams& R = gen.river();
  if (R.valid && R.radius_sectors > 0) {
    const float sw = kMapLayoutW / static_cast<float>(kMapSectorsX);
    const float sh = kMapLayoutH / static_cast<float>(kMapSectorsY);
    const float cx = (static_cast<float>(R.sector_x) + 0.5f) * sw;
    const float cy = (static_cast<float>(R.sector_y) + 0.5f) * sh;
    const int thick = R.radius_sectors * 2 + 1;
    if (R.horizontal) {
      const int cols = static_cast<int>(std::ceil(kMapLayoutW / static_cast<float>(kTile)));
      stamp_fill("tilemap_water", 0.f, cy - thick * kTile * 0.5f, cols, thick, 4, kTile, kTile, 3);
    } else {
      const int rows = static_cast<int>(std::ceil(kMapLayoutH / static_cast<float>(kTile)));
      stamp_fill("tilemap_water", cx - thick * kTile * 0.5f, 0.f, thick, rows, 4, kTile, kTile, 3);
    }
  }
}

int WorldTilemaps::tile_at(const char* image, float wx, float wy) const {
  // Prefer runtime overlays (later maps) so dirt stamps win over empty static.
  for (int i = count_ - 1; i >= 0; --i) {
    const Map& m = maps_[i];
    if (std::strcmp(m.image, image) != 0) continue;
    const int c = static_cast<int>(std::floor((wx - m.x) / m.tile_w));
    const int r = static_cast<int>(std::floor((wy - m.y) / m.tile_h));
    if (c < 0 || r < 0 || c >= m.cols || r >= m.rows) continue;
    const int id = tiles_[m.first + r * m.cols + c];
    if (id >= 0) return id;
  }
  return -1;
}

void WorldTilemaps::draw(IPlatform& platform, const char* prefix, Vec2 cam, float scale,
                         float view_w, float view_h) const {
  const std::size_t plen = std::strlen(prefix);
  const float half_w = view_w * 0.5f / scale;
  const float half_h = view_h * 0.5f / scale;
  for (int i = 0; i < count_; ++i) {
    const Map& m = maps_[i];
    if (std::strncmp(m.image, prefix, plen) != 0) continue;
    if (!platform.has_sprite(m.image)) continue;
    int c0 = static_cast<int>(std::floor((cam.x - half_w - m.x) / m.tile_w));
    int c1 = static_cast<int>(std::floor((cam.x + half_w - m.x) / m.tile_w));
    int r0 = static_cast<int>(std::floor((cam.y - half_h - m.y) / m.tile_h));
    int r1 = static_cast<int>(std::floor((cam.y + half_h - m.y) / m.tile_h));
    if (c0 < 0) c0 = 0;
    if (r0 < 0) r0 = 0;
    if (c1 >= m.cols) c1 = m.cols - 1;
    if (r1 >= m.rows) r1 = m.rows - 1;
    const float tw = m.tile_w * scale;
    const float th = m.tile_h * scale;
    for (int r = r0; r <= r1; ++r) {
      for (int c = c0; c <= c1; ++c) {
        const int id = tiles_[m.first + r * m.cols + c];
        if (id < 0) continue;
        const float sx = view_w * 0.5f + (m.x + c * m.tile_w - cam.x) * scale;
        const float sy = view_h * 0.5f + (m.y + r * m.tile_h - cam.y) * scale;
        platform.draw_sprite_frame(m.image, std::floor(sx), std::floor(sy),
                                   std::ceil(tw) + 1.f, std::ceil(th) + 1.f,
                                   (id % m.atlas_cols) * m.tile_w, (id / m.atlas_cols) * m.tile_h,
                                   m.tile_w, m.tile_h, 0.f, 1.f);
      }
    }
  }
}

} // namespace mdaz
