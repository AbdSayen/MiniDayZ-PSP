#include "mdaz/layout_sprites.hpp"

#include "mdaz/platform.hpp"

#include <cstdio>
#include <cstring>
#include <string>

namespace mdaz {

LayoutSprites::LayoutSprites() : count_(0) {}

bool LayoutSprites::load(IPlatform& platform, const char* path) {
  count_ = 0;
  char* data = 0;
  std::size_t size = 0;
  if (!platform.read_file(path, &data, &size) || !data) return false;
  const std::string text(data, size);
  delete[] data;

  std::size_t pos = text.find('\n');
  while (pos != std::string::npos && count_ < kMax) {
    const std::size_t start = pos + 1;
    pos = text.find('\n', start);
    const std::string line = text.substr(start, pos == std::string::npos ? std::string::npos : pos - start);
    Inst s;
    int layer = 0;
    if (std::sscanf(line.c_str(), "%d %47s %d %d %d %d %f %f %f %f %f %f %f %f", &layer, s.image,
                    &s.sx, &s.sy, &s.sw, &s.sh, &s.x, &s.y, &s.w, &s.h, &s.angle,
                    &s.hot_x, &s.hot_y, &s.opacity) == 14)
      inst_[count_++] = s;
  }
  return count_ > 0;
}

void LayoutSprites::draw(IPlatform& platform, Vec2 cam, float scale, float view_w, float view_h) const {
  for (int i = 0; i < count_; ++i) {
    const Inst& s = inst_[i];
    const float dw = s.w * scale;
    const float dh = s.h * scale;
    const float left = view_w * 0.5f + (s.x - cam.x) * scale - s.hot_x * dw;
    const float top = view_h * 0.5f + (s.y - cam.y) * scale - s.hot_y * dh;
    if (left > view_w || top > view_h || left + dw < 0.f || top + dh < 0.f) continue;
    platform.draw_sprite_frame(s.image, left, top, dw, dh, s.sx, s.sy, s.sw, s.sh, s.angle, s.opacity);
  }
}

} // namespace mdaz
