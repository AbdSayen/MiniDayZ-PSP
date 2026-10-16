#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

// Hostile survivors / bots — scaffold until full Bot_* C2 groups are ported.
struct Bandit {
  EntityId id;
  Vec2 pos;
  float hp;
  float max_hp;
  float speed;
  float aggro_radius;
  float attack_cooldown;
  bool alive;
  bool has_gun;

  Bandit();
  void setup(Vec2 p, EntityId eid, bool gun);
};

static const int kMaxBandits = 32;

class BanditSystem {
public:
  BanditSystem();
  void clear();
  Bandit* spawn(Vec2 p, bool gun);
  // Melee damage returned; gun shots handled by Game via attack_cooldown.
  float update(float dt, Vec2 player_pos);
  int count() const { return count_; }
  Bandit* at(int i) { return &bandits_[i]; }
  Bandit* begin() { return bandits_; }
  Bandit* end() { return bandits_ + count_; }
  const Bandit* begin() const { return bandits_; }
  const Bandit* end() const { return bandits_ + count_; }
  int alive_count() const;

private:
  Bandit bandits_[kMaxBandits];
  int count_;
  EntityId next_id_;
};

} // namespace mdaz
