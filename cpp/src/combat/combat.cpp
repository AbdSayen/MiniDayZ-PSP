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

namespace {
// Mutable NPC dmg — Regular defaults = balance_globals; overridden per difficulty
// from C2 States difficulty branches (weapon_hints / var_assignments).
float g_zed_dmg = static_cast<float>(balance::Zed_dmg);         // 9
float g_zed_fast_dmg = static_cast<float>(balance::Zed_fast_dmg); // 6
float g_zed_army_dmg = static_cast<float>(balance::Zed_army_dmg); // 13
float g_wolf_dmg = static_cast<float>(balance::Wolf_dmg);       // 10
float g_bandit_melee = 12.f;  // unarmed bot bayonet/fist stub
float g_bandit_gun_melee = 10.f;
} // namespace

void set_npc_damage_for_difficulty(Difficulty d) {
  // Extracted SetVariable triples near difficulty: Novice 6/9/6, Regular 9/13/10,
  // Veteran 10/15/10 (Wolf mid-path showed 8 once — prefer globals 10 for Regular).
  if (d == Difficulty_Novice) {
    g_zed_dmg = 6.f;
    g_zed_fast_dmg = 4.f;
    g_zed_army_dmg = 9.f;
    g_wolf_dmg = 6.f;
    g_bandit_melee = 9.f;
    g_bandit_gun_melee = 8.f;
  } else if (d == Difficulty_Veteran) {
    g_zed_dmg = 10.f;
    g_zed_fast_dmg = 8.f;
    g_zed_army_dmg = 15.f;
    g_wolf_dmg = 10.f;
    g_bandit_melee = 14.f;
    g_bandit_gun_melee = 12.f;
  } else {
    g_zed_dmg = static_cast<float>(balance::Zed_dmg);
    g_zed_fast_dmg = static_cast<float>(balance::Zed_fast_dmg);
    g_zed_army_dmg = static_cast<float>(balance::Zed_army_dmg);
    g_wolf_dmg = static_cast<float>(balance::Wolf_dmg);
    g_bandit_melee = 12.f;
    g_bandit_gun_melee = 10.f;
  }
}

float npc_wolf_damage() { return g_wolf_dmg; }

float npc_bandit_melee_damage(bool has_gun) {
  return has_gun ? g_bandit_gun_melee : g_bandit_melee;
}

float zed_melee_damage(ZedType type) {
  switch (type) {
  case Zed_Fast:
    return g_zed_fast_dmg;
  case Zed_Army:
  case Zed_ArmyShooter:
    return g_zed_army_dmg;
  case Zed_Tank:
    // Zed_tank_dmg_deal group exists; exact multiplier still extracting — use army as floor
    return g_zed_army_dmg * 1.5f;
  case Zed_Screamer:
  case Zed_Buried:
  case Zed_Normal:
  default:
    return g_zed_dmg;
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

void DispersionState::update_settled(float dt, bool braced) {
  // Crouch / stand-still settles aim faster (dispersion_bonus global unused=0)
  float step = cooldown_step * (braced ? 2.2f : 1.f);
  angle -= step * dt;
  if (angle < 0.f) angle = 0.f;
}

float DispersionState::sample_offset_deg(Rng& rng) const {
  if (angle <= 0.f) return 0.f;
  return rng.range(-angle, angle);
}

} // namespace mdaz
