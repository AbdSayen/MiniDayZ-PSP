#include "mdaz/los.hpp"
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
  zr_src = -1; zr_keep = false;
  burn_t = 0.f;
  type = t;
  pos = p;
  alive = true;
  attack_cooldown = 0.f;
  // data.js: HP = skin default IV (wolf IV1=50, dog IV1=250, deer IV2=80, rabbit IV2=5); speed
  // = spawn-time Bullet/run speed (wolf random(90,110), dog 140, deer random(110,130), rabbit
  // random(180,200)). Random rolls hashed from the entity id (deterministic setup).
  {
    const unsigned hsh = static_cast<unsigned>(eid) * 2654435761u;
    const float u = static_cast<float>((hsh >> 8) & 0xFFFFu) / 65535.f;
    switch (t) {
    case Animal_Wolf:
      hp = 50.f;
      speed = 90.f + 20.f * u;
      break;
    case Animal_Dog:
      hp = 250.f;
      speed = 140.f;
      break;
    case Animal_Deer:
      hp = 80.f;
      speed = 110.f + 20.f * u;
      break;
    case Animal_Rabbit:
      hp = 5.f;
      speed = 180.f + 20.f * u;
      break;
    case Animal_Crow:
      hp = 10.f;   // port value: crow_skin has no HP IV (Не исследована)
      speed = 200.f; // crow_skin Bullet speed set to 200 on take-off
      break;
    }
  }
}

AnimalSystem::AnimalSystem() : count_(0), next_id_(1) {}

void AnimalSystem::clear() { count_ = 0; }

Animal* AnimalSystem::spawn(AnimalType t, Vec2 p) {
  int slot = -1;
  for (int i = 0; i < count_; ++i) if (!animals_[i].alive) { slot = i; break; }
  if (slot < 0) { if (count_ >= kMaxAnimals) return 0; slot = count_++; }
  Animal& a = animals_[slot];
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
  (void)player_bleeding; // no blood-scent rule in data.js

  for (int i = 0; i < count_; ++i) {
    Animal& a = animals_[i];
    if (!a.alive) continue;
    if (a.attack_cooldown > 0.f) a.attack_cooldown -= dt;

    const float dx = player_pos.x - a.pos.x;
    const float dy = player_pos.y - a.pos.y;
    const float dist = std::sqrt(dx * dx + dy * dy);

    if (a.type == Animal_Wolf || a.type == Animal_Dog) {
      // C2: wolf_eyes Turret/LineOfSight range 335, dog_eyes 200 (data.js instance props).
      // Removed invented pack-alert (x1.15, 520) and blood-scent (+140, x1.1) boosts.
      const float range = a.type == Animal_Dog ? 200.f : 335.f;
      const float spd = a.speed;
      const bool sees = !los_blocked(a.pos, player_pos); // wolf/dog LineOfSight t927+t339
      a.snd_target = sees && dist < range;
      if (sees && dist > 1.f && dist < range) {
        a.pos.x += (dx / dist) * spd * dt;
        a.pos.y += (dy / dist) * spd * dt;
      }
      if (sees && dist < wolf_range && a.attack_cooldown <= 0.f) {
        if (a.type == Animal_Wolf) {
          dmg += wolf_melee_damage();
          if (wolf_melee_hit) *wolf_melee_hit = true;
          a.hit_tick = true;
        }
        // C2 dog (t892/t894, gui_dog_control) is the player's companion (Dog_check_situation
        // follows the player); it never calls Player_get_hit. Invented dog damage removed.
        a.attack_cooldown = wolf_cd;
      }
    } else if (a.type == Animal_Crow) {
      // Crows scatter farther / faster
      if (dist < 320.f && !a.flown) { a.flown = true; a.flew_evt = true; }
      if (dist < 320.f && dist > 1.f) {
        a.pos.x -= (dx / dist) * a.speed * dt;
        a.pos.y -= (dy / dist) * a.speed * dt;
      }
    } else {
      // Prey flees; deer stampede slightly faster when pack-alert wolves nearby
      // C2 Deer/Rabbit groups: every 1 s LineOfSight range 300 (deer) / 250 (rabbit) -> flee.
      const float flee_spd = a.speed;
      const float flee_r = a.type == Animal_Deer ? 300.f : 250.f;
      if (dist < flee_r && dist > 1.f) {
        a.pos.x -= (dx / dist) * flee_spd * dt;
        a.pos.y -= (dy / dist) * flee_spd * dt;
      }
    }
  }
  return dmg;
}

} // namespace mdaz
