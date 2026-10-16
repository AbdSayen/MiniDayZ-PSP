#include "mdaz/zombie.hpp"
#include "mdaz/combat.hpp"
#include "mdaz/balance_globals.hpp"

#include <cmath>

namespace mdaz {

Zombie::Zombie()
  : id(0), type(Zed_Normal), pos(), hp(0), max_hp(0), speed(40.f),
    aggro_radius(220.f), alive(false), attack_cooldown(0.f) {}

void Zombie::setup(ZedType t, Vec2 p, EntityId eid) {
  id = eid;
  type = t;
  pos = p;
  alive = true;
  attack_cooldown = 0.f;
  switch (t) {
  case Zed_Fast:
    max_hp = 60.f;
    speed = 95.f;
    aggro_radius = 260.f;
    break;
  case Zed_Army:
  case Zed_ArmyShooter:
    max_hp = 120.f;
    speed = 55.f;
    aggro_radius = 280.f;
    break;
  case Zed_Tank:
    max_hp = 400.f;
    speed = 35.f;
    aggro_radius = 300.f;
    break;
  case Zed_Screamer:
    max_hp = 80.f;
    speed = 50.f;
    aggro_radius = 320.f;
    break;
  case Zed_Buried:
    max_hp = 90.f;
    speed = 45.f;
    aggro_radius = 180.f;
    break;
  case Zed_Normal:
  default:
    max_hp = 100.f;
    speed = 48.f;
    aggro_radius = 220.f;
    break;
  }
  hp = max_hp;
}

void Zombie::update_ai(float dt, Vec2 player_pos, bool& out_hit_player, float& out_damage) {
  out_hit_player = false;
  out_damage = 0.f;
  if (!alive) return;
  if (attack_cooldown > 0.f) attack_cooldown -= dt;

  const float dx = player_pos.x - pos.x;
  const float dy = player_pos.y - pos.y;
  const float dist = std::sqrt(dx * dx + dy * dy);

  if (dist < aggro_radius && dist > 1.f) {
    pos.x += (dx / dist) * speed * dt;
    pos.y += (dy / dist) * speed * dt;
  }

  if (dist < 28.f && attack_cooldown <= 0.f) {
    out_hit_player = true;
    out_damage = zed_melee_damage(type);
    attack_cooldown = 1.0f;
  }
}

ZombieSystem::ZombieSystem() : count_(0), next_id_(1) {}

void ZombieSystem::clear() { count_ = 0; }

Zombie* ZombieSystem::spawn(ZedType t, Vec2 p) {
  if (count_ >= kMaxZombies) return 0;
  Zombie& z = zombies_[count_++];
  z.setup(t, p, next_id_++);
  return &z;
}

void ZombieSystem::update(float dt, Vec2 player_pos, float& damage_to_player) {
  damage_to_player = 0.f;
  for (int i = 0; i < count_; ++i) {
    bool hit = false;
    float dmg = 0.f;
    zombies_[i].update_ai(dt, player_pos, hit, dmg);
    if (hit) damage_to_player += dmg;
  }
}

int ZombieSystem::alive_count() const {
  int n = 0;
  for (int i = 0; i < count_; ++i) if (zombies_[i].alive) ++n;
  return n;
}

} // namespace mdaz
