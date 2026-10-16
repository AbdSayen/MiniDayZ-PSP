#include "mdaz/campfire.hpp"

namespace mdaz {

CampfireSystem::CampfireSystem() : count_(0) {}

void CampfireSystem::clear() { count_ = 0; }

Campfire* CampfireSystem::place(Vec2 pos, float life) {
  // Reuse dead slot
  for (int i = 0; i < count_; ++i) {
    if (!fires_[i].active) {
      fires_[i].pos = pos;
      fires_[i].life = life;
      fires_[i].active = true;
      return &fires_[i];
    }
  }
  if (count_ >= kMaxCampfires) return 0;
  Campfire& f = fires_[count_++];
  f.pos = pos;
  f.life = life;
  f.active = true;
  return &f;
}

void CampfireSystem::update(float dt) {
  for (int i = 0; i < count_; ++i) {
    if (!fires_[i].active) continue;
    fires_[i].life -= dt;
    if (fires_[i].life <= 0.f) fires_[i].active = false;
  }
}

float CampfireSystem::warmth_near(Vec2 pos, float radius) const {
  float best = 0.f;
  const float r2 = radius * radius;
  for (int i = 0; i < count_; ++i) {
    if (!fires_[i].active) continue;
    const float dx = fires_[i].pos.x - pos.x;
    const float dy = fires_[i].pos.y - pos.y;
    const float d2 = dx * dx + dy * dy;
    if (d2 > r2) continue;
    const float t = 1.f - (d2 / r2);
    if (t > best) best = t;
  }
  return best;
}

bool CampfireSystem::near_fire(Vec2 pos, float radius) const {
  return warmth_near(pos, radius) > 0.05f;
}

} // namespace mdaz
