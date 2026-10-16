#include "mdaz/animal.hpp"
#include "mdaz/balance_globals.hpp"

#include <cmath>

namespace mdaz {

float wolf_melee_damage() {
  return static_cast<float>(balance::Wolf_dmg); // 10
}

Animal::Animal()
  : id(0), type(Animal_Deer), pos(), hp(0), speed(0), alive(false), attack_cooldown(0) {}

void Animal::setup(AnimalType t, Vec2 p, EntityId eid) {
  id = eid;
  type = t;
  pos = p;
  alive = true;
  attack_cooldown = 0.f;
  switch (t) {
  case Animal_Wolf:
    hp = 80.f;
    speed = 110.f;
    break;
  case Animal_Dog:
    hp = 60.f;
    speed = 100.f;
    break;
  case Animal_Deer:
    hp = 70.f;
    speed = 120.f;
    break;
  case Animal_Rabbit:
    hp = 20.f;
    speed = 90.f;
    break;
  case Animal_Crow:
    hp = 10.f;
    speed = 80.f;
    break;
  }
}

AnimalSystem::AnimalSystem() : count_(0), next_id_(1) {}

void AnimalSystem::clear() { count_ = 0; }

Animal* AnimalSystem::spawn(AnimalType t, Vec2 p) {
  if (count_ >= kMaxAnimals) return 0;
  Animal& a = animals_[count_++];
  a.setup(t, p, next_id_++);
  return &a;
}

float AnimalSystem::update(float dt, Vec2 player_pos) {
  float dmg = 0.f;
  for (int i = 0; i < count_; ++i) {
    Animal& a = animals_[i];
    if (!a.alive) continue;
    if (a.attack_cooldown > 0.f) a.attack_cooldown -= dt;

    const float dx = player_pos.x - a.pos.x;
    const float dy = player_pos.y - a.pos.y;
    const float dist = std::sqrt(dx * dx + dy * dy);

    if (a.type == Animal_Wolf || a.type == Animal_Dog) {
      if (dist > 1.f && dist < 400.f) {
        a.pos.x += (dx / dist) * a.speed * dt;
        a.pos.y += (dy / dist) * a.speed * dt;
      }
      if (dist < 26.f && a.attack_cooldown <= 0.f) {
        dmg += (a.type == Animal_Wolf) ? wolf_melee_damage() : wolf_melee_damage() * 0.7f;
        a.attack_cooldown = 1.1f;
      }
    } else {
      // Prey flees
      if (dist < 250.f && dist > 1.f) {
        a.pos.x -= (dx / dist) * a.speed * dt;
        a.pos.y -= (dy / dist) * a.speed * dt;
      }
    }
  }
  return dmg;
}

} // namespace mdaz
