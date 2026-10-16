#include "mdaz/combat.hpp"
#include "mdaz/rng.hpp"

namespace mdaz {

WeaponStats default_melee_stats() {
  WeaponStats w;
  w.damage = 20.f;
  w.range = 40.f;
  w.fire_interval = 0.55f;
  w.dispersion_deg = 0.f;
  w.is_melee = true;
  return w;
}

WeaponStats default_pistol_stats() {
  WeaponStats w;
  w.damage = 25.f;
  w.range = 350.f;
  w.fire_interval = 0.35f;
  w.dispersion_deg = 2.f;
  w.is_melee = false;
  return w;
}

float apply_difficulty_damage_reducer(float damage, float reducer) {
  // Global Difficulty_damage_reducer init=0; when non-zero, original subtracts/scales.
  if (reducer <= 0.f) return damage;
  const float out = damage - reducer;
  return out > 0.f ? out : 0.f;
}

float zed_melee_damage(ZedType type) {
  switch (type) {
  case Zed_Fast:
    return static_cast<float>(balance::Zed_fast_dmg); // 6
  case Zed_Army:
  case Zed_ArmyShooter:
    return static_cast<float>(balance::Zed_army_dmg); // 13
  case Zed_Tank:
    return static_cast<float>(balance::Zed_army_dmg) * 1.5f; // tank group separate; scale TBD
  case Zed_Screamer:
  case Zed_Buried:
  case Zed_Normal:
  default:
    return static_cast<float>(balance::Zed_dmg); // 9
  }
}

DispersionState::DispersionState() { reset_from_globals(); }

void DispersionState::reset_from_globals() {
  angle = static_cast<float>(balance::dispersion_default);
  angle_max = static_cast<float>(balance::dispersion_angle_max);
  cooldown_step = static_cast<float>(balance::dispersion_cooldown_step);
  per_shot = static_cast<float>(balance::dispersion_pershot);
  run_penalty = static_cast<float>(balance::dispersion_run_penalty);
}

void DispersionState::on_shot(bool running) {
  angle += per_shot;
  if (running) angle += run_penalty;
  if (angle > angle_max) angle = angle_max;
}

void DispersionState::update(float dt) {
  angle -= cooldown_step * dt;
  if (angle < 0.f) angle = 0.f;
}

float DispersionState::sample_offset_deg(Rng& rng) const {
  if (angle <= 0.f) return 0.f;
  return rng.range(-angle, angle);
}

} // namespace mdaz
