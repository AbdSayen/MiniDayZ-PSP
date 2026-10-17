#include "mdaz/char_anims.hpp"

#include "mdaz/platform.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace mdaz {

CharAnims::CharAnims() : count_(0), frame_count_(0) {}

namespace {

bool next_int(const char*& p, const char* end, int& out) {
  while (p < end && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')) ++p;
  if (p >= end) return false;
  char* stop = 0;
  long v = std::strtol(p, &stop, 10);
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

bool CharAnims::load(IPlatform& platform, const char* path) {
  count_ = 0;
  frame_count_ = 0;
  char* data = 0;
  std::size_t size = 0;
  if (!platform.read_file(path, &data, &size) || !data) return false;
  const char* p = data;
  const char* end = data + size;
  int n = 0;
  bool ok = next_int(p, end, n);
  for (int i = 0; ok && i < n && count_ < kMaxAnims; ++i) {
    CharAnim a;
    int fc = 0, speed = 5, loop = 0;
    ok = next_word(p, end, a.base, sizeof(a.base)) &&
         next_word(p, end, a.name, sizeof(a.name)) &&
         next_int(p, end, fc) && next_int(p, end, speed) && next_int(p, end, loop);
    if (!ok) break;
    if (fc < 0) fc = 0;
    if (frame_count_ + fc > kMaxFrames) break;
    a.frame_count = fc;
    a.speed = speed > 0 ? speed : 5;
    a.loop = loop != 0;
    a.first = frame_count_;
    for (int f = 0; ok && f < fc; ++f) {
      CharAnimFrame fr;
      ok = next_int(p, end, fr.sheet) && next_int(p, end, fr.sx) && next_int(p, end, fr.sy) &&
           next_int(p, end, fr.sw) && next_int(p, end, fr.sh) && next_int(p, end, fr.duration);
      if (!ok) break;
      if (fr.duration < 1) fr.duration = 1;
      frames_[frame_count_++] = fr;
    }
    if (ok) anims_[count_++] = a;
  }
  delete[] data;
  return count_ > 0;
}

const CharAnim* CharAnims::find(const char* base, const char* anim_name) const {
  if (!base || !anim_name) return 0;
  for (int i = 0; i < count_; ++i) {
    if (std::strcmp(anims_[i].base, base) == 0 &&
        std::strcmp(anims_[i].name, anim_name) == 0)
      return &anims_[i];
  }
  return 0;
}

const CharAnimFrame* CharAnims::sample(const CharAnim* anim, float time_sec, int* out_index) const {
  if (!anim || anim->frame_count <= 0) return 0;
  // Construct: each frame lasts (duration / speed) seconds.
  float total = 0.f;
  for (int i = 0; i < anim->frame_count; ++i)
    total += static_cast<float>(frames_[anim->first + i].duration) / static_cast<float>(anim->speed);
  if (total <= 1.e-6f) total = 1.f;
  float t = time_sec;
  if (anim->loop) {
    t = std::fmod(t, total);
    if (t < 0.f) t += total;
  } else if (t >= total) {
    if (out_index) *out_index = anim->frame_count - 1;
    return &frames_[anim->first + anim->frame_count - 1];
  }
  float acc = 0.f;
  for (int i = 0; i < anim->frame_count; ++i) {
    const float dur = static_cast<float>(frames_[anim->first + i].duration) /
                      static_cast<float>(anim->speed);
    if (t < acc + dur) {
      if (out_index) *out_index = i;
      return &frames_[anim->first + i];
    }
    acc += dur;
  }
  if (out_index) *out_index = anim->frame_count - 1;
  return &frames_[anim->first + anim->frame_count - 1];
}

void CharAnims::draw(IPlatform& platform, const char* base, const char* anim_name,
                     float time_sec, float sx, float sy, float dw, float dh) const {
  const CharAnim* anim = find(base, anim_name);
  if (!anim) {
    // Fallback idle_down
    anim = find(base, "idle_down");
    if (!anim) return;
  }
  const CharAnimFrame* fr = sample(anim, time_sec);
  if (!fr) return;
  char sheet[64];
  std::snprintf(sheet, sizeof(sheet), "%s-sheet%d", base, fr->sheet);
  if (!platform.has_sprite(sheet)) return;
  platform.draw_sprite_frame(sheet, sx - dw * 0.5f, sy - dh * 0.5f, dw, dh,
                             fr->sx, fr->sy, fr->sw, fr->sh, 0.f, 1.f);
}

const char* CharAnims::dir_suffix(float facing_deg) {
  float deg = facing_deg;
  while (deg > 180.f) deg -= 360.f;
  while (deg < -180.f) deg += 360.f;
  if (deg >= -45.f && deg < 45.f) return "_right";
  if (deg >= 45.f && deg < 135.f) return "_down";
  if (deg >= -135.f && deg < -45.f) return "_up";
  return "_left";
}

void CharAnims::make_name(char* out, int cap, const char* prefix, float facing_deg) {
  if (!out || cap < 8 || !prefix) return;
  std::snprintf(out, cap, "%s%s", prefix, dir_suffix(facing_deg));
}

} // namespace mdaz
