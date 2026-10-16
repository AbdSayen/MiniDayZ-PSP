#include "mdaz/bandit.hpp"

#include <cmath>

namespace mdaz {

Bandit::Bandit()
  : id(0), pos(), hp(0), max_hp(0), speed(70.f), aggro_radius(320.f),
    attack_cooldown(0.f), alive(false), has_gun(false) {}

void Bandit::setup(Vec2 p, EntityId eid, bool gun) {
  id = eid;
  pos = p;
  alive = true;
  has_gun = gun;
  attack_cooldown = 0.f;
  max_hp = gun ? 110.f : 90.f;
  hp = max_hp;
  speed = gun ? 55.f : 75.f;
  aggro_radius = gun ? 380.f : 260.f;
}

BanditSystem::BanditSystem() : count_(0), next_id_(1) {}

void BanditSystem::clear() { count_ = 0; }

Bandit* BanditSystem::spawn(Vec2 p, bool gun) {
  if (count_ >= kMaxBandits) return 0;
  Bandit& b = bandits_[count_++];
  b.setup(p, next_id_++, gun);
  return &b;
}

float BanditSystem::update(float dt, Vec2 player_pos) {
  float dmg = 0.f;
  for (int i = 0; i < count_; ++i) {
    Bandit& b = bandits_[i];
    if (!b.alive) continue;
    if (b.attack_cooldown > 0.f) b.attack_cooldown -= dt;

    const float dx = player_pos.x - b.pos.x;
    const float dy = player_pos.y - b.pos.y;
    const float dist = std::sqrt(dx * dx + dy * dy);

    float spd = b.speed;
    if (b.has_gun && dist < 180.f) spd *= 0.4f;
    const bool fleeing = b.hp < b.max_hp * 0.3f;

    if (dist > 1.f && dist < b.aggro_radius) {
      if (fleeing) {
        // Low HP — run away
        b.pos.x -= (dx / dist) * spd * 1.2f * dt;
        b.pos.y -= (dy / dist) * spd * 1.2f * dt;
      } else {
        b.pos.x += (dx / dist) * spd * dt;
        b.pos.y += (dy / dist) * spd * dt;
      }
    }

    // Melee bandits always; gun bandits bayonet when very close
    if (!fleeing && dist < 28.f && b.attack_cooldown <= 0.f) {
      if (!b.has_gun || dist < 22.f) {
        dmg += b.has_gun ? 10.f : 12.f;
        b.attack_cooldown = b.has_gun ? 1.2f : 1.0f;
      }
    }
  }
  return dmg;
}

int BanditSystem::alive_count() const {
  int n = 0;
  for (int i = 0; i < count_; ++i) if (bandits_[i].alive) ++n;
  return n;
}

} // namespace mdaz
