#pragma once

#include "mdaz/animal.hpp"
#include "mdaz/balance_globals.hpp"
#include "mdaz/types.hpp"
#include "mdaz/inventory.hpp"
#include "mdaz/zombie.hpp"

namespace mdaz {

// C2 NPC_get_damage (obj180) — kind param selects which IV holds HP:
// 0 wolf469 IV1, 1 deer472 IV2, 2 rabbit609 IV2, 3 zed941 IV0,
// 4 screamer710 IV2, 5 tank847 IV0, 6 puncher540 IV4, 7 shooter243 IV3,
// 8 teammate549 IV4, 9 bandit827 IV3, 10 friend522 IV4, 11 dog894 IV1.
enum NpcGetDamageKind {
  NpcDmg_Wolf = 0,
  NpcDmg_Deer = 1,
  NpcDmg_Rabbit = 2,
  NpcDmg_Zed = 3,
  NpcDmg_ZedScreamer = 4,
  NpcDmg_ZedTank = 5,
  NpcDmg_BotPuncher = 6,
  NpcDmg_BotShooter = 7,
  NpcDmg_BotTeammate = 8,
  NpcDmg_BotBandit = 9,
  NpcDmg_BotFriend = 10,
  NpcDmg_Dog = 11
};

NpcGetDamageKind npc_damage_kind_for_zed(ZedType t);
NpcGetDamageKind npc_damage_kind_for_animal(AnimalType t);
NpcGetDamageKind npc_damage_kind_for_bandit(bool has_gun);

// Subtract damage from NPC HP (C2 SubInstanceVar on the mapped IV). Returns amount applied.
float npc_apply_damage(float& hp, NpcGetDamageKind kind, float amount);

// C2 health bar width: min(30, hp_iv * max_iv) — for HUD when max_hp known.
float npc_health_bar_width(float hp, float max_hp, NpcGetDamageKind kind);

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

// Apply C2 difficulty NPC damage tables (Zed_dmg / Zed_army_dmg / Wolf_dmg).
// Regular matches balance_globals init (9 / 13 / 10). Call from new-game start.
void set_npc_damage_for_difficulty(Difficulty d);
float npc_wolf_damage(); // player→wolf NPC_get_damage (Wolf_dmg global)
float npc_zed_fast_damage();
float npc_zed_army_damage();
float npc_bandit_melee_damage(bool has_gun);

// Wolf_dmg_deal → Player_get_hit(..., "wolf", Zed_dmg) — same table as normal zed melee.
float wolf_player_melee_damage();
float wolf_attack_melee_range();   // Wolf group IV1 = 50
float wolf_attack_cooldown_sec();  // ScrollTo duration = 3

// C2 zmb → Player_get_hit: Audio choose among attack_0/1/10/11/12/13 at zed position.
const char* pick_zed_player_hit_sfx(class Rng& rng);
// C2 screamer group: attack_8 SFX (attack_8_impl.json).
const char* c2_zed_screamer_attack_sfx();

// C2 event group under NPCs (routes to NPC_get_damage) — audit anchor only.
void touch_combat_c2_audit_anchors();

// Bullets_dmg_deal (verified):
//   dmg = round( bullet_base * random(bullet_base/4, …) ) — variance uses IV0/4.
// Implemented as round(base * U(0.75, 1.25)) ≈ ±25% which matches IV0±IV0/4.
float roll_bullet_damage(float base_damage, class Rng& rng);

// Player_get_hit: "bullet" param subtracts IV15 without flat vest mul;
// bullets wear clothing IV6 (dmg_randomizer 3–7). Melee uses def_percent block + IV6 wear.
enum PlayerDamageKind { PlayerDmg_Default = 0, PlayerDmg_Bullet = 1 };

// Player_get_hit IV4 tiers → def_percent random base (5 / 15 / 25 / 35 + perk).
enum MeleeDefPercentTier {
  MeleeDef_KnifeClass = 0,
  MeleeDef_Medium = 1,
  MeleeDef_Heavy = 2,
  MeleeDef_Blunt = 3
};

float player_damage_after_armor(float amount, const Inventory& inv, PlayerDamageKind kind);
// Returns true when def_percent >= random(0,100) — damage blocked (C2 compare [8,2] path skipped).
bool roll_player_melee_def_block(Rng& rng, MeleeDefPercentTier tier, float perk_meleedef);
MeleeDefPercentTier melee_def_tier_for_npc_damage(float damage, float zed_fast_dmg, float zed_army_dmg);
float clothing_iv6_max(ItemId id);
// C2 Hit_effect: layout effect param 0→100% for 0.2s then 0 (screen blood flash).
constexpr float kHitEffectPeakSec = 0.2f;
constexpr float kHitEffectFadeSec = 0.35f;

// C2 dmg_hint (obj529 Text on Rain layer): floating "-N" drifts toward viewer.
// Param2==1 → red 18px (crit-style); else orange (255,196,68). See dmg_hint_decoded.txt.
constexpr float kDmgHintLifeSec = 0.85f;
constexpr float kDmgHintSpeedPx = 55.f;
constexpr int kMaxDmgHints = 24;

struct DmgHint {
  Vec2 pos;
  Vec2 vel;
  float life;
  int amount;
  bool crit;
  bool active;
};

class DmgHintSystem {
public:
  DmgHintSystem();
  void clear();
  // Spawn at target; velocity aims toward `toward` (player↔target angle in C2).
  void spawn(Vec2 at, Vec2 toward, int amount, bool crit_style);
  void update(float dt);
  int count() const { return count_; }
  const DmgHint* begin() const { return hints_; }
  const DmgHint* end() const { return hints_ + count_; }

private:
  DmgHint hints_[kMaxDmgHints];
  int count_;
};

// Bullets_dmg_deal IV8 branch (mosin/sks/svd/repeater/sv98/sawed mosin): second IV0 subtract
// on zed941/847 when bullet IV8 and rolled dmg < random(0,100).
bool weapon_bullet_iv8_armor_pierce(ItemId weapon_id);
float zed_apply_bullet_hit_damage(float& hp, NpcGetDamageKind kind, float rolled_dmg, ZedType ztype,
                                  bool bullet_iv8, class Rng& rng);

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
  void update_settled(float dt, bool braced); // crouch settles aim faster
  float sample_offset_deg(class Rng& rng) const;
};

} // namespace mdaz
