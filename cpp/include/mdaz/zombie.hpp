#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

struct Zombie {
  EntityId id;
  short zr_src;   // C2 zed IV link to its zed_resp_point (-1 = none)
  bool zr_keep;   // C2 eyes IV6/IV7 true: never culled by Zed_check
  ZedType type;
  Vec2 pos;
  float hp;
  float max_hp;
  float speed;
  float aggro_radius;
  bool alive;
  float attack_cooldown;
  float ambush_boost; // Buried pop-up speed burst
  // C2 warn_zed: an idle zed that hears noice() turns toward the sound (+-50 px) and walks at
  // its b_warn Bullet speed random(80,90) for 2 s - no line of sight needed.
  float warn_t;
  // C2 eyes IV0 "attacking" + LineOfSight behaviours (data.js instance props):
  // MainLook range 335 (shooter 350) cone 180, ShortLook range 70 cone 360. Re-checked every 1 s.
  bool c2_aggro = false;
  float look_t = 0.f;
  Vec2 facing = Vec2(0.f, 1.f);
  Vec2 warn_dir;
  float warn_spd;
  float burn_t;       // C2 molotov 'burning' boolean: seconds left (5 s Wait in Molotov params)

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
              bool player_coughing = false);
  // C2 noice(x, y, r): zeds within r + 200 px get warn_zed toward (x, y).
  void noise(Vec2 at, float radius);
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
public:
  // C2 night rule: eyes see the player only if the player light overlaps him and dist <= 175.
  void set_vision(bool night, bool player_light) { vis_night_ = night; vis_light_ = player_light; }
  bool vis_night_ = false;
  bool vis_light_ = false;
private:
  Vec2 scream_pos_;
};

} // namespace mdaz
