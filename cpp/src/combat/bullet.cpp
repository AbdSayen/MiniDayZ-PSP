#include "mdaz/bullet.hpp"

#include <cmath>

namespace mdaz {

BulletSystem::BulletSystem() : count_(0) {}

void BulletSystem::clear() { count_ = 0; }

Bullet* BulletSystem::spawn(Vec2 pos, float angle_deg, float speed, float damage, float life,
                            bool from_player, bool explosive, bool leaves_fire) {
  if (count_ >= kMaxBullets) return 0;
  Bullet& b = bullets_[count_++];
  const float rad = angle_deg * 0.01745329252f;
  b.pos = pos;
  b.vel = Vec2(std::cos(rad) * speed, std::sin(rad) * speed);
  b.damage = damage;
  b.life = life;
  b.alive = true;
  b.from_player = from_player;
  b.explosive = explosive;
  b.leaves_fire = leaves_fire;
  b.is_arrow = false;
  return &b;
}

void BulletSystem::update(float dt) {
  for (int i = 0; i < count_; ++i) {
    Bullet& b = bullets_[i];
    if (!b.alive) continue;
    b.pos.x += b.vel.x * dt;
    b.pos.y += b.vel.y * dt;
    b.life -= dt;
    if (b.life <= 0.f) b.alive = false;
  }
}

} // namespace mdaz
