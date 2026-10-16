#pragma once

#include "mdaz/balance_globals.hpp"
#include "mdaz/types.hpp"

namespace mdaz {

struct WeaponStats {
  float damage;
  float range;
  float fire_interval;
  float dispersion_deg;
  bool is_melee;
};

WeaponStats default_melee_stats();
WeaponStats default_pistol_stats();

float apply_difficulty_damage_reducer(float damage, float reducer);
float zed_melee_damage(ZedType type);

// Bullets_dmg_deal (verified):
//   dmg = round( bullet_base * random(bullet_base/4, …) ) — variance uses IV0/4.
// Implemented as round(base * U(0.75, 1.25)) ≈ ±25% which matches IV0±IV0/4.
float roll_bullet_damage(float base_damage, class Rng& rng);

struct DispersionState {
  float angle;
  float angle_max;
  float cooldown_step;
  float per_shot;
  float run_penalty;

  DispersionState();
  void reset_from_globals();
  void on_shot(bool running);
  void update(float dt);
  float sample_offset_deg(class Rng& rng) const;
};

} // namespace mdaz
