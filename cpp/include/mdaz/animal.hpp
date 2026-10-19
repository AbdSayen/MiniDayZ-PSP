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
  float burn_t; // C2 molotov burning (wolf only in data.js)

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
  // wolf_melee_hit set when a wolf (not dog) lands a Wolf_dmg_deal hit.
  float update(float dt, Vec2 player_pos, bool player_bleeding = false,
               bool* wolf_melee_hit = 0);
  int count() const { return count_; }
  int alive_count() const;
  int alive_of_type(AnimalType t) const;
  Animal* at(int i) { return &animals_[i]; }

private:
  Animal animals_[kMaxAnimals];
  int count_;
  EntityId next_id_;
};

// Wolf→player hit damage (Zed_dmg via Wolf_dmg_deal); see wolf_player_melee_damage().
float wolf_melee_damage();

} // namespace mdaz
