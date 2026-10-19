#include "mdaz/animal.hpp"
#include "mdaz/combat.hpp"

#include <cmath>

namespace mdaz {

float wolf_melee_damage() {
  return wolf_player_melee_damage();
}

Animal::Animal()
  : id(0), type(Animal_Deer), pos(), hp(0), speed(0), alive(false), attack_cooldown(0), burn_t(0) {}

void Animal::setup(AnimalType t, Vec2 p, EntityId eid) {
  id = eid;
  burn_t = 0.f;
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
    speed = 140.f; // flock flees fast
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

int AnimalSystem::alive_count() const {
  int n = 0;
  for (int i = 0; i < count_; ++i) if (animals_[i].alive) ++n;
  return n;
}

int AnimalSystem::alive_of_type(AnimalType t) const {
  int n = 0;
  for (int i = 0; i < count_; ++i) {
    if (animals_[i].alive && animals_[i].type == t) ++n;
  }
  return n;
}

float AnimalSystem::update(float dt, Vec2 player_pos, bool player_bleeding,
                           bool* wolf_melee_hit) {
  float dmg = 0.f;
  if (wolf_melee_hit) *wolf_melee_hit = false;
  const float wolf_range = wolf_attack_melee_range();
  const float wolf_cd = wolf_attack_cooldown_sec();
  // Pack howl: if any wolf is close, pull distant packmates in
  bool pack_alert = false;
  for (int i = 0; i < count_; ++i) {
    if (!animals_[i].alive) continue;
    if (animals_[i].type != Animal_Wolf && animals_[i].type != Animal_Dog) continue;
    const float dx = player_pos.x - animals_[i].pos.x;
    const float dy = player_pos.y - animals_[i].pos.y;
    if (dx * dx + dy * dy < 180.f * 180.f) { pack_alert = true; break; }
  }

  for (int i = 0; i < count_; ++i) {
    Animal& a = animals_[i];
    if (!a.alive) continue;
    if (a.attack_cooldown > 0.f) a.attack_cooldown -= dt;

    const float dx = player_pos.x - a.pos.x;
    const float dy = player_pos.y - a.pos.y;
    const float dist = std::sqrt(dx * dx + dy * dy);

    if (a.type == Animal_Wolf || a.type == Animal_Dog) {
      float range = pack_alert ? 520.f : 400.f;
      if (player_bleeding) range += 140.f; // blood scent
      float spd = a.speed * (pack_alert ? 1.15f : 1.f);
      if (player_bleeding) spd *= 1.1f;
      if (dist > 1.f && dist < range) {
        a.pos.x += (dx / dist) * spd * dt;
        a.pos.y += (dy / dist) * spd * dt;
      }
      if (dist < wolf_range && a.attack_cooldown <= 0.f) {
        if (a.type == Animal_Wolf) {
          dmg += wolf_melee_damage();
          if (wolf_melee_hit) *wolf_melee_hit = true;
        } else {
          dmg += wolf_melee_damage() * 0.7f; // dog — no C2 wolf_attack SFX path
        }
        a.attack_cooldown = wolf_cd;
      }
    } else if (a.type == Animal_Crow) {
      // Crows scatter farther / faster
      if (dist < 320.f && dist > 1.f) {
        a.pos.x -= (dx / dist) * a.speed * dt;
        a.pos.y -= (dy / dist) * a.speed * 1.15f * dt;
      }
    } else {
      // Prey flees; deer stampede slightly faster when pack-alert wolves nearby
      float flee_spd = a.speed;
      if (a.type == Animal_Deer && pack_alert) flee_spd *= 1.35f;
      if (dist < 250.f && dist > 1.f) {
        a.pos.x -= (dx / dist) * flee_spd * dt;
        a.pos.y -= (dy / dist) * flee_spd * dt;
      }
    }
  }
  return dmg;
}

} // namespace mdaz
