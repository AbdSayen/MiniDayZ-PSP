#include "mdaz/combat.hpp"
#include "mdaz/rng.hpp"

// Fist melee pick distance observed as 45 in event data near "Fist".

namespace mdaz {

WeaponStats default_melee_stats() {
  WeaponStats w;
  // Fist: pick-distance 45 verified; damage candidate 10 from IV4 "Fist" window
  // (melee_damage_hints / melee_npc_dmg extract). Per-weapon values in weapons.cpp.
  w.damage = 10.f;
  w.range = 45.f;
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
    // Zed_tank_dmg_deal group exists; exact multiplier still extracting — use army as floor
    return static_cast<float>(balance::Zed_army_dmg) * 1.5f;
  case Zed_Screamer:
  case Zed_Buried:
  case Zed_Normal:
  default:
    return static_cast<float>(balance::Zed_dmg); // 9
  }
}

float roll_bullet_damage(float base_damage, Rng& rng) {
  // From Bullets_dmg_deal: uses bullet IV0 with random(IV0/4, …) variance.
  // Equivalent band: [base - base/4, base + base/4] = [0.75, 1.25] * base
  const float lo = base_damage * 0.75f;
  const float hi = base_damage * 1.25f;
  const float v = rng.range(lo, hi);
  // round() as in original
  return static_cast<float>(static_cast<int>(v + (v >= 0.f ? 0.5f : -0.5f)));
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
