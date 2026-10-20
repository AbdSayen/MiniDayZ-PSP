#include "mdaz/world_tilemaps.hpp"

#include "mdaz/map_decode_tables.hpp"
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

#include "mdaz/forest_collision.inc"

bool point_in_poly(const float* pts, int n, float x, float y) {
  bool in = false;
  for (int i = 0, j = n - 1; i < n; j = i++) {
    const float xi = pts[i * 2], yi = pts[i * 2 + 1];
    const float xj = pts[j * 2], yj = pts[j * 2 + 1];
    if (((yi > y) != (yj > y)) && (x < (xj - xi) * (y - yi) / (yj - yi) + xi)) in = !in;
  }
  return in;
}

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
    m.top = false;
    if (char* bar = std::strchr(m.image, '|')) { m.top = std::strcmp(bar, "|top") == 0; *bar = 0; }
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
  m.top = false;
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

bool WorldTilemaps::ensure_ground_overlay() {
  for (int i = static_count_; i < count_; ++i) {
    Map& m = maps_[i];
    if (std::strcmp(m.image, "ground_lvl1_tilemap") == 0 && m.cols >= 64 && m.rows >= 64)
      return true;
  }
  return alloc_map("ground_lvl1_tilemap", 0.f, 0.f, kOverlayCols, kOverlayRows,
                   kTile, kTile, 5, -1);
}

void WorldTilemaps::stamp_location_forest(const MapLocation& L) {
  // C2 generate_location: SetTile on tilemap_forest relative to POI hotspotX/Y.
  for (int i = 0; i < kForestStampCount; ++i) {
    const ForestStamp& st = kForestStamps[i];
    if (st.tile < 0) continue;
    stamp_tile("tilemap_forest", L.pos.x + static_cast<float>(st.dx),
                 L.pos.y + static_cast<float>(st.dy), st.tile, 150, 150, 3);
  }
}

void WorldTilemaps::stamp_location_footprint(const MapLocation& L) {
  if (!ensure_ground_overlay()) return;
  auto fill_ground = [&](float wx, float wy, int cols, int rows, int tile) {
    stamp_fill("ground_lvl1_tilemap", wx, wy, cols, rows, tile);
  };
  if (L.type == Loc_Hummer) return;  // wreck only: create_map_location stamps no pad
  int pad = 7;
  if (L.type == Loc_City || L.type == Loc_Military) pad = 15;
  else if (L.type == Loc_Hospital || L.type == Loc_Firestation) pad = 13;
  else if (L.type == Loc_Village || L.type == Loc_Police || L.type == Loc_Church) pad = 11;
  else if (L.type == Loc_GasStation || L.type == Loc_Helicrash) pad = 9;
  else if (L.type == Loc_BunkerEntrance || L.type == Loc_Secret) pad = 5;

  if (tile_at("ground_lvl1_tilemap", L.pos.x, L.pos.y) == 15) return;

  const float half = static_cast<float>(pad) * 0.5f * static_cast<float>(kTile);
  fill_ground(L.pos.x - half, L.pos.y - half, pad, pad, 15);
  fill_ground(L.pos.x - 8.5f * kTile, L.pos.y - kTile, 17, 2, 15);
  fill_ground(L.pos.x - kTile, L.pos.y - 8.5f * kTile, 2, 17, 15);
  fill_ground(L.pos.x - 4.5f * kTile, L.pos.y - kTile, 9, 2, 15);
  fill_ground(L.pos.x - kTile, L.pos.y - 4.5f * kTile, 2, 9, 15);

  if (L.type == Loc_Helicrash) {
    stamp_fill("tilemap_water", L.pos.x + 120.f, L.pos.y - 60.f, 3, 3, 4, 60, 60, 3);
  }
}

void WorldTilemaps::stamp_from_mapgen(const MapGenerator& gen) {
  clear_runtime();
  if (!ensure_ground_overlay()) return;

  auto set_ground = [&](float wx, float wy, int tile) {
    stamp_tile("ground_lvl1_tilemap", wx, wy, tile);
  };

  // C2 create_map_location: ground SetTile → 15; water → 4.
  // stamp_location_forest is NOT applied (2026-10-19): the decoded SetTile offsets are marked
  // approximate (truncated export) and, now that tilemap_forest is drawn and Solid, they put
  // isolated corner tiles around POIs. See KNOWN_ISSUES.md.
  for (int i = 0; i < gen.location_count(); ++i) {
    const MapLocation& L = gen.location(i);
    if (!L.active) continue;
    stamp_location_footprint(L);
  }

  // C2 generate_array_roads_1/_2 sector markers (12/13/14) → runtime dirt tile 15.
  {
    const float sw = kMapLayoutW / static_cast<float>(kMapSectorsX);
    const float sh = kMapLayoutH / static_cast<float>(kMapSectorsY);
    for (int sy = 0; sy < kMapSectorsY; ++sy) {
      for (int sx = 0; sx < kMapSectorsX; ++sx) {
        if (!gen.is_road_sector(sx, sy)) continue;
        const float cx = (static_cast<float>(sx) + 0.5f) * sw;
        const float cy = (static_cast<float>(sy) + 0.5f) * sh;
        for (int dy = -1; dy <= 1; ++dy) {
          for (int dx = -1; dx <= 1; ++dx) {
            set_ground(cx + static_cast<float>(dx) * kTile,
                       cy + static_cast<float>(dy) * kTile, 15);
          }
        }
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

bool WorldTilemaps::forest_solid_at(float wx, float wy) const {
  for (int i = count_ - 1; i >= 0; --i) {
    const Map& m = maps_[i];
    if (std::strcmp(m.image, "tilemap_forest") != 0) continue;
    const int c = static_cast<int>(std::floor((wx - m.x) / m.tile_w));
    const int r = static_cast<int>(std::floor((wy - m.y) / m.tile_h));
    if (c < 0 || r < 0 || c >= m.cols || r >= m.rows) continue;
    const int id = tiles_[m.first + r * m.cols + c];
    if (id < 0) continue;
    if (id >= kForestTilePolyCount) return true;
    const float lx = (wx - (m.x + c * m.tile_w)) / static_cast<float>(m.tile_w);
    const float ly = (wy - (m.y + r * m.tile_h)) / static_cast<float>(m.tile_h);
    const ForestTilePoly& tp = kForestTilePolys[id];
    if (point_in_poly(tp.pts, tp.n, lx, ly)) return true;
  }
  return false;
}

namespace {
#include "water_tile_polys.inc"
}
// Static Map-layout tilemap_water instances (t199, Solid): per-tile collision polys.
bool WorldTilemaps::water_solid_at(float wx, float wy) const {
  for (int i = count_ - 1; i >= 0; --i) {
    const Map& m = maps_[i];
    if (std::strcmp(m.image, "tilemap_water") != 0) continue;
    const int c = static_cast<int>(std::floor((wx - m.x) / m.tile_w));
    const int r = static_cast<int>(std::floor((wy - m.y) / m.tile_h));
    if (c < 0 || r < 0 || c >= m.cols || r >= m.rows) continue;
    const int id = tiles_[m.first + r * m.cols + c];
    if (water_tile_solid(id, (wx - (m.x + c * m.tile_w)) / static_cast<float>(m.tile_w),
                         (wy - (m.y + r * m.tile_h)) / static_cast<float>(m.tile_h))) return true;
  }
  return false;
}

void WorldTilemaps::draw(IPlatform& platform, const char* prefix, Vec2 cam, float scale,
                         float view_w, float view_h) const {
  draw_maps(platform, prefix, cam, scale, view_w, view_h, false);
}

void WorldTilemaps::draw_top(IPlatform& platform, const char* prefix, Vec2 cam, float scale,
                             float view_w, float view_h) const {
  draw_maps(platform, prefix, cam, scale, view_w, view_h, true);
}

void WorldTilemaps::draw_maps(IPlatform& platform, const char* prefix, Vec2 cam, float scale,
                              float view_w, float view_h, bool top) const {
  const std::size_t plen = std::strlen(prefix);
  const float half_w = view_w * 0.5f / scale;
  const float half_h = view_h * 0.5f / scale;
  for (int i = 0; i < count_; ++i) {
    const Map& m = maps_[i];
    if (m.top != top) continue;
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
