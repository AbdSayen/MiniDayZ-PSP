#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

enum AnimalType {
  Animal_Wolf = 0,
  Animal_Deer,
  Animal_Rabbit,
  Animal_Crow,
  Animal_Dog
};

struct Animal {
  EntityId id;
  AnimalType type;
  Vec2 pos;
  float hp;
  float speed;
  bool alive;
  float attack_cooldown;

  Animal();
  void setup(AnimalType t, Vec2 p, EntityId eid);
};

static const int kMaxAnimals = 64;

class AnimalSystem {
public:
  AnimalSystem();
  void clear();
  Animal* spawn(AnimalType t, Vec2 p);
  // Returns melee damage dealt to player this frame (wolves/dogs).
  // has_fire: campfire near player repels wolves/dogs. is_night: night hunt boost.
  float update(float dt, Vec2 player_pos, bool player_bleeding = false,
               Vec2 fire_pos = Vec2(), bool has_fire = false, bool is_night = false);
  int count() const { return count_; }
  int alive_count() const;
  int alive_of_type(AnimalType t) const;
  Animal* at(int i) { return &animals_[i]; }

private:
  Animal animals_[kMaxAnimals];
  int count_;
  EntityId next_id_;
};

float wolf_melee_damage(); // balance::Wolf_dmg

} // namespace mdaz
