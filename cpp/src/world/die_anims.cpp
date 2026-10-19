#include "mdaz/die_anims.hpp"
#include <cstring>
namespace mdaz {
#include "die_anims.inc"
const DieAnim* die_anim_find(const char* key, int style) {
  if (!key) return 0;
  for (unsigned i = 0; i < sizeof(kDieAnims) / sizeof(kDieAnims[0]); ++i)
    if (kDieAnims[i].style == style && std::strcmp(kDieAnims[i].key, key) == 0) return &kDieAnims[i];
  return 0;
}
float die_anim_seconds(const DieAnim* a) {
  return (a && a->speed > 0) ? static_cast<float>(a->count) / static_cast<float>(a->speed) : 0.f;
}
const DieFrame* die_anim_frame(const DieAnim* a, float elapsed) {
  if (!a || a->count <= 0) return 0;
  int f = a->speed > 0 ? static_cast<int>(elapsed * static_cast<float>(a->speed)) : 0;
  if (f < 0) f = 0;
  if (f >= a->count) f = a->count - 1;
  return &a->f[f];
}
} // namespace mdaz
