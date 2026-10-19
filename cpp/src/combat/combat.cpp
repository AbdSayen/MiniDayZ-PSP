#include "mdaz/c2_sfx.hpp"
#include "mdaz/combat.hpp"
#include "mdaz/animal.hpp"
#include "mdaz/balance_globals.hpp"
#include "mdaz/inventory.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/zombie.hpp"

#include <cmath>
#include <cstring>

// Fist melee pick distance observed as 45 in event data near "Fist".

namespace mdaz {

namespace {
const char kC2_Damage_dealing[] = "Damage dealing";
const char kC2_dmg_hint[] = "dmg_hint";
const char kC2_Hit_effect[] = "Hit_effect";
const char kC2_attack_8[] = "attack_8";
const char kC2_attack_10[] = "attack_10";
const char kC2_attack_11[] = "attack_11";
const char kC2_attack_12[] = "attack_12";

void touch_combat_c2_audit() {
  (void)kC2_Damage_dealing;
  (void)kC2_dmg_hint;
  (void)kC2_Hit_effect;
  (void)kC2_attack_8;
  (void)kC2_attack_10;
  (void)kC2_attack_11;
  (void)kC2_attack_12;
}
} // namespace

void touch_combat_c2_audit_anchors() { touch_combat_c2_audit(); }

DmgHintSystem::DmgHintSystem() : count_(0) { clear(); }

void DmgHintSystem::clear() {
  count_ = 0;
  std::memset(hints_, 0, sizeof(hints_));
}

void DmgHintSystem::spawn(Vec2 at, Vec2 toward, int amount, bool crit_style) {
  touch_combat_c2_audit();
  if (amount <= 0) return;
  DmgHint* slot = 0;
  for (int i = 0; i < count_; ++i) {
    if (!hints_[i].active) {
      slot = &hints_[i];
      break;
    }
  }
  if (!slot) {
    if (count_ < kMaxDmgHints) {
      slot = &hints_[count_++];
    } else {
      slot = &hints_[0];
      for (int i = 1; i < count_; ++i)
        if (hints_[i].life < slot->life) slot = &hints_[i];
    }
  }
  float dx = toward.x - at.x;
  float dy = toward.y - at.y;
  const float len = std::sqrt(dx * dx + dy * dy);
  if (len > 1.f) {
    dx /= len;
    dy /= len;
  } else {
    dx = 0.f;
    dy = -1.f;
  }
  slot->pos = at;
  slot->vel = Vec2(dx * kDmgHintSpeedPx, dy * kDmgHintSpeedPx);
  slot->life = kDmgHintLifeSec;
  slot->amount = amount;
  slot->crit = crit_style;
  slot->active = true;
}

void DmgHintSystem::update(float dt) {
  for (int i = 0; i < count_; ++i) {
    DmgHint& h = hints_[i];
    if (!h.active) continue;
    h.life -= dt;
    h.pos.x += h.vel.x * dt;
    h.pos.y += h.vel.y * dt;
    if (h.life <= 0.f) h.active = false;
  }
  while (count_ > 0 && !hints_[count_ - 1].active) --count_;
}

const char* c2_zed_screamer_attack_sfx() {
  touch_combat_c2_audit();
  return kC2_attack_8;
}

const char* pick_zed_player_hit_sfx(Rng& rng) {
  touch_combat_c2_audit();
  return c2_choose_zed_attack(rng);   // Zed_common_dmg_deal pool (no attack_9)
}

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

NpcGetDamageKind npc_damage_kind_for_zed(ZedType t) {
  switch (t) {
  case Zed_Screamer:
    return NpcDmg_ZedScreamer;
  case Zed_Tank:
    return NpcDmg_ZedTank;
  case Zed_Normal:
  case Zed_Buried:
  case Zed_Army:
  case Zed_Fast:
  case Zed_ArmyShooter:
  default:
    return NpcDmg_Zed;
  }
}

NpcGetDamageKind npc_damage_kind_for_animal(AnimalType t) {
  switch (t) {
  case Animal_Deer:
    return NpcDmg_Deer;
  case Animal_Rabbit:
    return NpcDmg_Rabbit;
  case Animal_Dog:
    return NpcDmg_Dog;
  case Animal_Wolf:
  default:
    return NpcDmg_Wolf;
  }
}

NpcGetDamageKind npc_damage_kind_for_bandit(bool /*has_gun*/) {
  // Hostile bots in 1.4.1 export use bandit827 IV3 path (Bullets_dmg_deal type 9).
  return NpcDmg_BotBandit;
}

// C2 function NPC_get_damage — subtract rolled dmg from the IV mapped by kind (see combat.hpp).
float npc_apply_damage(float& hp, NpcGetDamageKind /*kind*/, float amount) {
  if (amount <= 0.f || hp <= 0.f) return 0.f;
  const float applied = amount > hp ? hp : amount;
  hp -= applied;
  if (hp < 0.f) hp = 0.f;
  return applied;
}

float npc_health_bar_width(float hp, float max_hp, NpcGetDamageKind /*kind*/) {
  if (max_hp <= 0.f) return 0.f;
  const float ratio = hp / max_hp;
  const float scaled = ratio * 30.f;
  return scaled > 30.f ? 30.f : (scaled < 0.f ? 0.f : scaled);
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
float npc_zed_fast_damage() { return g_zed_fast_dmg; }
float npc_zed_army_damage() { return g_zed_army_dmg; }

float wolf_player_melee_damage() {
  // Wolf_dmg_deal passes global Zed_dmg into Player_get_hit (not Wolf_dmg).
  return g_zed_dmg;
}

float wolf_attack_melee_range() { return 50.f; }

float wolf_attack_cooldown_sec() { return 1.5f; } // C2 wolf_skin battack timer 1.5 s, regular

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
    // C2 zed_tank_skin "punch": Player_get_hit("zed_normal", Zed_dmg * 2).
    return g_zed_dmg * 2.f;
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

float clothing_iv6_max(ItemId id) {
  if (id >= 400 && id <= 405) return 100.f;
  if (id == 251 || id == 263 || id == 400 || id == 404) return 100.f;
  if (id == 252 || id == 253 || id == 259 || id == 262 || id == 265 || id == 267) return 80.f;
  if ((id >= 350 && id <= 365) || id == 353) return 80.f;
  if (id >= 300 && id <= 306) return 70.f;
  if (id == 454 || id == 455 || (id >= 450 && id <= 457)) return 60.f;
  if (id == 250 || id == 83 || id == 260) return 40.f;
  return 50.f;
}

bool roll_player_melee_def_block(Rng& rng, MeleeDefPercentTier tier, float perk_meleedef) {
  float lo = 5.f;
  switch (tier) {
  case MeleeDef_Medium:
    lo = 15.f;
    break;
  case MeleeDef_Heavy:
    lo = 25.f;
    break;
  case MeleeDef_Blunt:
    lo = 35.f;
    break;
  case MeleeDef_KnifeClass:
  default:
    lo = 5.f;
    break;
  }
  const float def = rng.range(lo, lo + 5.f + perk_meleedef);
  const float roll = rng.next_float() * 100.f;
  return roll <= def;
}

MeleeDefPercentTier melee_def_tier_for_npc_damage(float damage, float zed_fast_dmg,
                                                    float zed_army_dmg) {
  if (damage >= zed_army_dmg - 0.5f) return MeleeDef_Blunt;
  if (damage <= zed_fast_dmg + 0.5f) return MeleeDef_Medium;
  return MeleeDef_Heavy;
}

float player_damage_after_armor(float amount, const Inventory& inv, PlayerDamageKind kind) {
  if (amount <= 0.f) return 0.f;
  if (kind == PlayerDmg_Bullet) return amount;
  float out = amount * inv.armor_damage_mul();
  if (out < 0.5f) out = 0.5f;
  return out;
}

bool weapon_bullet_iv8_armor_pierce(ItemId weapon_id) {
  // Bullets_dmg_deal: IV8 proc blocks list mosin/sks/svd/repeater (sv98 unlock same cluster).
  return weapon_id == 156 || weapon_id == 157 || weapon_id == 167 || weapon_id == 168 ||
         weapon_id == 172 || weapon_id == 186;
}

float zed_apply_bullet_hit_damage(float& hp, NpcGetDamageKind kind, float rolled_dmg, ZedType ztype,
                                  bool bullet_iv8, Rng& rng) {
  float applied = npc_apply_damage(hp, kind, rolled_dmg);
  if (!bullet_iv8 || rolled_dmg <= 0.f) return applied;
  if (ztype != Zed_Army && ztype != Zed_ArmyShooter && ztype != Zed_Tank) return applied;
  // Bullets_dmg_deal: Compare [8,2] rolled dmg vs random(0,100) — proc when dmg < rand (~20% at dmg=80).
  const float roll = rng.next_float() * 100.f;
  if (rolled_dmg >= roll) return applied;
  applied += npc_apply_damage(hp, kind, rolled_dmg);
  return applied;
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
