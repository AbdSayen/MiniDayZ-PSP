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

// Defaults until per-weapon tables are extracted from "Bullets parameters" / firearm events.
WeaponStats default_melee_stats();
WeaponStats default_pistol_stats();

float apply_difficulty_damage_reducer(float damage, float reducer);

// Zed melee damage from globals (verified init values).
float zed_melee_damage(ZedType type);

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
