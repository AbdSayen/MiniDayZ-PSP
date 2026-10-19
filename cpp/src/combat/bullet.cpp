#include "mdaz/bullet.hpp"

#include <cmath>

namespace mdaz {

BulletSystem::BulletSystem() : count_(0) {}

void BulletSystem::clear() { count_ = 0; }

Bullet* BulletSystem::spawn(Vec2 pos, float angle_deg, float speed, float damage, float life,
                            bool from_player, bool explosive, bool leaves_fire,
                            ItemId source_weapon, bool bandit_gun_sprite) {
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
  b.bandit_gun_sprite = bandit_gun_sprite;
  b.armor_pierce_iv8 = false;
  b.source_weapon = source_weapon;
  b.penetration_iv2 = from_player ? 3 : (bandit_gun_sprite ? 1 : 0);
  b.travel_px = 0.f;
  return &b;
}

void BulletSystem::update(float dt) {
  for (int i = 0; i < count_; ++i) {
    Bullet& b = bullets_[i];
    if (!b.alive) continue;
    const float dx = b.vel.x * dt;
    const float dy = b.vel.y * dt;
    b.pos.x += dx;
    b.pos.y += dy;
    b.travel_px += std::sqrt(dx * dx + dy * dy);
    // New_bullet / Range_less_power: at 100px IV2 3→2, at 220px IV2 2→1 (obstacle break tiers).
    if (b.penetration_iv2 >= 3 && b.travel_px >= 100.f) b.penetration_iv2 = 2;
    else if (b.penetration_iv2 >= 2 && b.travel_px >= 220.f) b.penetration_iv2 = 1;
    b.life -= dt;
    if (b.life <= 0.f) b.alive = false;
  }
}

} // namespace mdaz
