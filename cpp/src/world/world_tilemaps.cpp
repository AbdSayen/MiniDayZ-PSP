#include "mdaz/world_tilemaps.hpp"

#include "mdaz/platform.hpp"

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

} // namespace

WorldTilemaps::WorldTilemaps() : count_(0), tile_count_(0) {}

bool WorldTilemaps::load(IPlatform& platform, const char* path) {
  count_ = 0;
  tile_count_ = 0;
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
      // C2 stores flip/rotate flags in the top bits of a tile id.
      if (fits) tiles_[tile_count_ + k] = static_cast<short>(v < 0 ? -1 : (v & 0x1FFFFFFF));
    }
    if (ok && fits) {
      tile_count_ += cells;
      maps_[count_++] = m;
    }
  }
  delete[] data;
  return ok && count_ > 0;
}

int WorldTilemaps::tile_at(const char* image, float wx, float wy) const {
  for (int i = 0; i < count_; ++i) {
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
        // +1 px overlap hides seams from float→int rounding.
        platform.draw_sprite_frame(m.image, std::floor(sx), std::floor(sy),
                                   std::ceil(tw) + 1.f, std::ceil(th) + 1.f,
                                   (id % m.atlas_cols) * m.tile_w, (id / m.atlas_cols) * m.tile_h,
                                   m.tile_w, m.tile_h, 0.f, 1.f);
      }
    }
  }
}

} // namespace mdaz
