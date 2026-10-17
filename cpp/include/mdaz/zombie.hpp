#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

struct Zombie {
  EntityId id;
  ZedType type;
  Vec2 pos;
  float hp;
  float max_hp;
  float speed;
  float aggro_radius;
  bool alive;
  float attack_cooldown;
  float ambush_boost; // Buried pop-up speed burst

  Zombie();
  void setup(ZedType t, Vec2 p, EntityId eid);
  void update_ai(float dt, Vec2 player_pos, bool& out_hit_player, float& out_damage,
                 float aggro_mul = 1.f);
};

static const int kMaxZombies = 128;

class ZombieSystem {
public:
  ZombieSystem();
  void clear();
  Zombie* spawn(ZedType t, Vec2 p);
  void update(float dt, Vec2 player_pos, float& damage_to_player, bool player_running = false,
              bool player_crouch = false, bool player_bleeding = false,
              bool player_coughing = false, bool is_night = false);
  int alive_count() const;
  bool scream_active() const { return scream_active_; }
  Vec2 scream_pos() const { return scream_pos_; }
  Zombie* begin() { return zombies_; }
  Zombie* end() { return zombies_ + count_; }
  const Zombie* begin() const { return zombies_; }
  const Zombie* end() const { return zombies_ + count_; }

private:
  Zombie zombies_[kMaxZombies];
  int count_;
  EntityId next_id_;
  bool scream_active_;
  Vec2 scream_pos_;
};

} // namespace mdaz
