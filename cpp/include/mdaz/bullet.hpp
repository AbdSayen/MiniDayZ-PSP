#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

struct Bullet {
  Vec2 pos;
  Vec2 vel;
  float damage;
  float life;
  bool alive;
  bool from_player;
  bool explosive;  // F1 / molotov splash on death or hit
  bool leaves_fire; // molotov places campfire puddle
  bool is_arrow;    // bow / crossbow projectile sprite
  bool armor_pierce_iv8; // C2 bullet IV8 — extra IV0 subtract on armored zeds (Bullets_dmg_deal)
};

static const int kMaxBullets = 64;

class BulletSystem {
public:
  BulletSystem();
  void clear();
  Bullet* spawn(Vec2 pos, float angle_deg, float speed, float damage, float life, bool from_player,
                bool explosive = false, bool leaves_fire = false);
  void update(float dt);
  int count() const { return count_; }
  Bullet* begin() { return bullets_; }
  Bullet* end() { return bullets_ + count_; }
  const Bullet* begin() const { return bullets_; }
  const Bullet* end() const { return bullets_ + count_; }

private:
  Bullet bullets_[kMaxBullets];
  int count_;
};

} // namespace mdaz
