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
      fires_[i].molotov_burn = false;
      return &fires_[i];
    }
  }
  if (count_ >= kMaxCampfires) return 0;
  Campfire& f = fires_[count_++];
  f.pos = pos;
  f.life = life;
  f.active = true;
  f.molotov_burn = false;
  return &f;
}

Campfire* CampfireSystem::place_molotov(Vec2 pos, float life) {
  Campfire* f = place(pos, life);
  if (f) f->molotov_burn = true;
  return f;
}

bool CampfireSystem::boost_near(Vec2 pos, float add_life, float radius) {
  int best = -1;
  float best_d2 = radius * radius;
  for (int i = 0; i < count_; ++i) {
    if (!fires_[i].active) continue;
    const float dx = fires_[i].pos.x - pos.x;
    const float dy = fires_[i].pos.y - pos.y;
    const float d2 = dx * dx + dy * dy;
    if (d2 < best_d2) { best_d2 = d2; best = i; }
  }
  if (best < 0) return false;
  fires_[best].life += add_life;
  if (fires_[best].life > 600.f) fires_[best].life = 600.f;
  return true;
}

void CampfireSystem::update(float dt, float rain01) {
  for (int i = 0; i < count_; ++i) {
    if (!fires_[i].active) continue;
    float drain = dt;
    if (rain01 > 0.7f) drain += dt * (2.5f + rain01 * 4.f); // storm kills open fire
    fires_[i].life -= drain;
    if (fires_[i].life <= 0.f) fires_[i].active = false;
  }
}

float CampfireSystem::warmth_near(Vec2 pos, float radius) const {
  float best = 0.f;
  const float r2 = radius * radius;
  for (int i = 0; i < count_; ++i) {
    if (!fires_[i].active || fires_[i].molotov_burn) continue;
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

bool CampfireSystem::in_molotov_burn(Vec2 pos, float radius) const {
  const float r2 = radius * radius;
  for (int i = 0; i < count_; ++i) {
    if (!fires_[i].active || !fires_[i].molotov_burn) continue;
    const float dx = fires_[i].pos.x - pos.x;
    const float dy = fires_[i].pos.y - pos.y;
    if (dx * dx + dy * dy <= r2) return true;
  }
  return false;
}

} // namespace mdaz
