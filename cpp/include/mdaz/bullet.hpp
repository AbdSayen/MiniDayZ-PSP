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
  bool bandit_gun_sprite; // C2 m4_bullet_bot-sheet0 for obj827/obj243 firearm bots
  bool armor_pierce_iv8; // C2 bullet IV8 — extra IV0 subtract on armored zeds (Bullets_dmg_deal)
  ItemId source_weapon; // player weapon item id for kill credit (Bullets_dmg_deal clusters)
  int penetration_iv2;  // C2 obj910 IV2 — Range_less_power travel decay (3→2→1)
  float travel_px;      // distance moved since spawn (for IV2 thresholds)
};

static const int kMaxBullets = 64;

class BulletSystem {
public:
  BulletSystem();
  void clear();
  Bullet* spawn(Vec2 pos, float angle_deg, float speed, float damage, float life, bool from_player,
                bool explosive = false, bool leaves_fire = false,
                ItemId source_weapon = 0, bool bandit_gun_sprite = false);
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
