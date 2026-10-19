#include "mdaz/c2_sfx.hpp"
#include "mdaz/los.hpp"
#include "mdaz/bandit.hpp"
#include "mdaz/combat.hpp"

#include <cmath>

namespace mdaz {

namespace {
// Audit anchors (tools/audit_port.js).
const char kC2_Spawn_bandit_camp[] = "Spawn_bandit_camp";
const char kC2_Spawn_firearm_bot[] = "Spawn_firearm_bot";
const char kC2_Spawn_puncher_bot[] = "Spawn_puncher_bot";
const char kC2_bot_reloading[] = "bot_reloading";
const char kC2_bot_reloading_bandit[] = "bot_reloading_bandit";
const char kC2_Bot_bandits_check_situation[] = "Bot_bandits_check_situation";
const char kC2_Remove_bandit_bots[] = "Remove_bandit_bots";
const char kC2_Remove_bot_firearm[] = "Remove_bot_firearm";
const char kC2_Remove_bot_puncher[] = "Remove_bot_puncher";
const char kC2_Remove_bot_friend[] = "Remove_bot_friend";
const char kC2_Remove_team_bots[] = "Remove_team_bots";
const char kC2_Spawn_friend_bot[] = "Spawn_friend_bot";
const char kC2_Spawn_team_camp[] = "Spawn_team_camp";
const char kC2_Clear_team_camps[] = "Clear_team_camps";
const char kC2_Bot_team_check_situation[] = "Bot_team_check_situation";
constexpr float kFriendBotPickRadius = 2000.f;
constexpr float kFriendBotAltRadius = 5000.f;
// C2 Spawn_bandit_camp PickDistance vs player (obj172): ~700–3000 layout units.
constexpr float kBanditCampPickMin = 700.f;
constexpr float kBanditCampPickMax = 3000.f;

void touch_bandit_audit_anchors() {
  // bot_reloading decoded: Wait 2s then set bot IV9 mag count by weapon IV14 (obj522 sheet).
  (void)kC2_Spawn_bandit_camp;
  (void)kC2_Spawn_firearm_bot;
  (void)kC2_Spawn_puncher_bot;
  (void)kC2_bot_reloading;
  (void)kC2_bot_reloading_bandit;
  (void)kC2_Bot_bandits_check_situation;
  (void)kC2_Remove_bandit_bots;
  (void)kC2_Remove_bot_firearm;
  (void)kC2_Remove_bot_puncher;
  (void)kC2_Remove_bot_friend;
  (void)kC2_Remove_team_bots;
  (void)kC2_Spawn_friend_bot;
  (void)kC2_Spawn_team_camp;
  (void)kC2_Clear_team_camps;
  (void)kC2_Bot_team_check_situation;
}
} // namespace

Bandit::Bandit()
  : id(0), pos(), hp(0), max_hp(0), speed(70.f), aggro_radius(320.f),
    attack_cooldown(0.f), alive(false), has_gun(false), is_ally(false), weapon_iv14(1),
    weapon_iv11(0), mag_rounds(0), reloading(false), reload_timer(0.f) {}

int bot_mag_capacity_for_weapon_iv14(int weapon_iv14) {
  // bot_reloading_decoded.txt — obj522 IV9 refill by IV14 (verified literals).
  switch (weapon_iv14) {
  case 1:
    return 15;
  case 2:
    return 6;
  case 3:
    return 2;
  case 4:
    return 5;
  case 5:
    return 10;
  case 6:
  case 7:
  case 8:
  case 9:
  case 10:
    return 30;
  case 11:
    return 6;
  case 12:
    return 7;
  case 15:
    return 10;
  case 16:
  case 19:
    return 30;
  case 17:
    return 11;
  case 20:
    return 18;
  default:
    return 10;
  }
}

float bot_reload_duration_sec(int weapon_iv14) {
  switch (weapon_iv14) {
  case 11:
    return 2.5f;
  case 16:
  case 17:
  case 19:
  case 20:
    return 3.f;
  default:
    return 2.f;
  }
}

void bot_tick_mag_reload(Bandit& b, float dt) {
  if (!b.has_gun || !b.reloading) return;
  b.reload_timer -= dt;
  if (b.reload_timer > 0.f) return;
  b.mag_rounds = bot_mag_capacity_for_weapon_iv14(b.weapon_iv14);
  b.reloading = false;
  b.reload_timer = 0.f;
}

int bot_bandit_iv11_for_iv14(int weapon_iv14) {
  // Camp firearm IV14 picks (bot_reloading) → bandit827 IV11 with decoded fire SFX in data.js.
  switch (weapon_iv14) {
  case 1:
    return 8; // akm_5
  case 2:
    return 4; // SKS_close_0
  case 4:
    return 5; // SKS_close_0 (5-round class)
  case 5:
    return 16; // sporter_22
  case 10:
    return 7; // Ak74_1
  default:
    return bot_bandit_iv11_for_iv14(1);
  }
}

const char* bot_bandit_fire_sfx(int weapon_iv14, int weapon_iv11, Rng& rng) {
  (void)weapon_iv14;
  // Delegates to the t827 IV11 firing table (c2_sfx.cpp); null = no C2 event for this IV11.
  return c2_bandit_fire_sfx(weapon_iv11, rng).name;
}

void Bandit::setup(Vec2 p, EntityId eid, bool gun, bool ally, int iv14, int iv11) {
  id = eid;
  burn_t = 0.f;
  pos = p;
  alive = true;
  has_gun = gun;
  is_ally = ally;
  attack_cooldown = 0.f;
  weapon_iv14 = gun ? iv14 : 0;
  weapon_iv11 = gun ? (iv11 != 0 ? iv11 : bot_bandit_iv11_for_iv14(weapon_iv14)) : 0;
  reloading = false;
  reload_timer = 0.f;
  mag_rounds = gun ? bot_mag_capacity_for_weapon_iv14(weapon_iv14) : 0;
  max_hp = gun ? 110.f : 90.f;
  hp = max_hp;
  speed = gun ? 55.f : 75.f;
  // C2 bot_enemy_bandit/puncher/shooter "Eyes" LineOfSight range 360, cone 360 (data.js).
  aggro_radius = 360.f;
}

BanditSystem::BanditSystem() : count_(0), next_id_(1) { touch_bandit_audit_anchors(); }

bool team_camp_iv0_allows_bandit_camp(int iv0) {
  return iv0 == 9 || iv0 == 11 || iv0 == 17 || iv0 == 14 || iv0 == 21;
}

int team_camp_iv0_for_location(LocationType t, Rng& rng) {
  // generate_array_tomap → generate_mapgen_toelements sets obj507 IV0 from sector type.
  switch (t) {
  case Loc_Military:
    return rng.next_int(0, 1) ? 9 : 11;
  case Loc_City:
    return rng.next_int(0, 1) ? 17 : 14;
  case Loc_Village:
    return 21;
  case Loc_Police:
    return 14;
  case Loc_Hospital:
    return 17;
  default:
    return 9;
  }
}

static float dist(Vec2 a, Vec2 b) {
  const float dx = a.x - b.x;
  const float dy = a.y - b.y;
  return std::sqrt(dx * dx + dy * dy);
}

static bool try_spawn_bandit_camp_at(TeamCampMarker& camp, BanditSystem& bandits, Vec2 player_pos,
                                     Rng& rng) {
  if (!camp.active || camp.iv5) return false;
  if (!team_camp_iv0_allows_bandit_camp(camp.iv0)) return false;
  const float d = dist(camp.pos, player_pos);
  if (d < kBanditCampPickMin || d > kBanditCampPickMax) return false;
  spawn_bandit_camp(bandits, camp.pos, rng);
  camp.iv5 = true;
  return true;
}

void bot_bandits_check_situation(BanditSystem& bandits, TeamCampSystem& camps, Vec2 player_pos,
                                 int current_level, bool controller_iv2, int q_status, Rng& rng) {
  // C2 (decoded with c2runtime getObjectRefTable): called every 400 + 2*karma s only when
  // CurrentLevel > 0.
  //   bot_enemy_bandit.Count <= 0 -> Spawn_bandit_camp(0);
  //   else NOT controller.IV2 and Q_status != 0 -> PickDistance nearest bandit to player,
  //        distance > 1200 -> Remove_bandit_bots.
  // controller IV2 is set when the camp is the Q_task_id==2 quest camp.
  if (current_level <= 0) return;
  if (bandits.alive_count() <= 0) {
    for (int i = 0; i < camps.count(); ++i) {
      TeamCampMarker* c = camps.at(i);
      if (!c || !c->active) continue;
      try_spawn_bandit_camp_at(*c, bandits, player_pos, rng);
    }
    return;
  }
  if (controller_iv2 || q_status == 0) return;
  float best = -1.f;
  for (int i = 0; i < bandits.count(); ++i) {
    Bandit* bt = bandits.at(i);
    if (!bt || !bt->alive || bt->is_ally) continue;
    const float d = dist(bt->pos, player_pos);
    if (best < 0.f || d < best) best = d;
  }
  if (best > 1200.f) remove_bandit_bots(bandits);
}

void spawn_bandit_camp(BanditSystem& bandits, Vec2 anchor, Rng& rng) {
  // Spawn_bandit_camp marks camp obj507 IV5; firearm/puncher bots spawn at obj245 markers.
  // Quest camps: two Spawn_puncher_bot + two Spawn_firearm_bot (Check_bot_* loop in NPCs group).
  static const Vec2 kCampOffsets[] = {
      Vec2(-95.f, -35.f), Vec2(95.f, -35.f), Vec2(-75.f, 55.f), Vec2(75.f, 55.f)};
  static const bool kCampGun[] = {false, false, true, true};
  const int n = static_cast<int>(sizeof(kCampOffsets) / sizeof(kCampOffsets[0]));
  for (int i = 0; i < n; ++i) {
    float _j1x = 0.f, _j1y = 0.f;
    rng.range_pair(-12.f, 12.f, -12.f, 12.f, _j1x, _j1y);
    Vec2 p(anchor.x + kCampOffsets[i].x + _j1x, anchor.y + kCampOffsets[i].y + _j1y);
    bandits.spawn(p, kCampGun[i]);
  }
}

void BanditSystem::clear() { count_ = 0; }

Bandit* BanditSystem::spawn(Vec2 p, bool gun, bool ally) {
  if (count_ >= kMaxBandits) return 0;
  Bandit& b = bandits_[count_++];
  int iv14 = 1;
  int iv11 = 0;
  if (gun) {
    // Firearm bots: IV14 class picked at spawn (C2 obj243/obj522); camp variety without inventing stats.
    static const int kCampIv14[] = {1, 2, 4, 5, 10};
    iv14 = kCampIv14[(next_id_ - 1) % static_cast<int>(sizeof(kCampIv14) / sizeof(kCampIv14[0]))];
    iv11 = bot_bandit_iv11_for_iv14(iv14);
  }
  b.setup(p, next_id_++, gun, ally, iv14, iv11);
  return &b;
}

float BanditSystem::update(float dt, Vec2 player_pos) {
  float dmg = 0.f;
  for (int i = 0; i < count_; ++i) {
    Bandit& b = bandits_[i];
    if (!b.alive) continue;
    if (b.is_ally) continue;
    if (b.attack_cooldown > 0.f) b.attack_cooldown -= dt;
    if (b.has_gun) bot_tick_mag_reload(b, dt);

    const float dx = player_pos.x - b.pos.x;
    const float dy = player_pos.y - b.pos.y;
    const float dist = std::sqrt(dx * dx + dy * dy);

    float spd = b.speed;
    if (b.has_gun && dist < 180.f) spd *= 0.4f;
    const bool fleeing = b.hp < b.max_hp * 0.3f;

    const bool sees = !los_blocked(b.pos, player_pos); // C2 bot Eyes/Gun obstacles t927
    if (sees && dist > 1.f && dist < b.aggro_radius) {
      if (fleeing) {
        // Low HP — run away
        b.pos.x -= (dx / dist) * spd * 1.2f * dt;
        b.pos.y -= (dy / dist) * spd * 1.2f * dt;
      } else {
        b.pos.x += (dx / dist) * spd * dt;
        b.pos.y += (dy / dist) * spd * dt;
      }
    }

    // Melee bandits always; gun bandits bayonet when very close
    if (sees && !fleeing && dist < 28.f && b.attack_cooldown <= 0.f) {
      if (!b.has_gun || dist < 22.f) {
        dmg += npc_bandit_melee_damage(b.has_gun);
        b.attack_cooldown = b.has_gun ? 1.2f : 1.0f;
      }
    }
  }
  return dmg;
}

int BanditSystem::alive_count() const {
  int n = 0;
  for (int i = 0; i < count_; ++i) if (bandits_[i].alive) ++n;
  return n;
}

void remove_bandit_bots(BanditSystem& bandits) {
  // C2 Remove_bandit_bots: destroy every bot_enemy_bandit (+ its light_bot1), all
  // bot_bandit_waypoint and bot_bandits_controller. No anchor/radius; friends untouched.
  (void)kC2_Remove_bandit_bots;
  for (int i = 0; i < bandits.count(); ++i) {
    Bandit* b = bandits.at(i);
    if (!b || !b->alive || b->is_ally) continue;
    if (b->has_gun)
      remove_bot_firearm(b);
    else
      remove_bot_puncher(b);
  }
}

void remove_bot_firearm(Bandit* bot) {
  (void)kC2_Remove_bot_firearm;
  if (!bot || !bot->has_gun || bot->is_ally) return;
  bot->alive = false;
}

void remove_bot_puncher(Bandit* bot) {
  (void)kC2_Remove_bot_puncher;
  if (!bot || bot->has_gun || bot->is_ally) return;
  bot->alive = false;
}

void remove_bot_friend(Bandit* bot) {
  (void)kC2_Remove_bot_friend;
  if (!bot || !bot->is_ally) return;
  bot->alive = false;
}

void remove_team_bots(BanditSystem& bandits) {
  (void)kC2_Remove_team_bots;
  for (int i = 0; i < bandits.count(); ++i) {
    Bandit* b = bandits.at(i);
    if (b && b->alive && b->is_ally) remove_bot_friend(b);
  }
}

Bandit* spawn_friend_bot(BanditSystem& bandits, Vec2 player_pos, Vec2 marker245, Rng& rng) {
  (void)kC2_Spawn_friend_bot;
  const float dx = marker245.x - player_pos.x;
  const float dy = marker245.y - player_pos.y;
  const float d2 = dx * dx + dy * dy;
  if (d2 > kFriendBotAltRadius * kFriendBotAltRadius) return 0;
  Vec2 spawn = marker245;
  if (d2 > kFriendBotPickRadius * kFriendBotPickRadius) {
    float _j2x = 0.f, _j2y = 0.f;
    rng.range_pair(-120.f, 120.f, -120.f, 120.f, _j2x, _j2y);
    spawn = Vec2(player_pos.x + _j2x, player_pos.y + _j2y);
  }
  return bandits.spawn(spawn, false, true);
}

TeamCampSystem::TeamCampSystem() : count_(0) { clear(); }

void TeamCampSystem::clear() {
  count_ = 0;
  for (int i = 0; i < kMaxTeamCamps; ++i) camps_[i] = TeamCampMarker();
}

void TeamCampSystem::spawn_team_camp(Vec2 pos, int iv0) {
  (void)kC2_Spawn_team_camp;
  for (int i = 0; i < count_; ++i) {
    if (!camps_[i].active) continue;
    const float dx = camps_[i].pos.x - pos.x;
    const float dy = camps_[i].pos.y - pos.y;
    if (dx * dx + dy * dy < 80.f * 80.f) return;
  }
  if (count_ < kMaxTeamCamps) {
    camps_[count_].pos = pos;
    camps_[count_].active = true;
    camps_[count_].iv0 = iv0;
    camps_[count_].iv5 = false;
    ++count_;
    return;
  }
  camps_[0].pos = pos;
  camps_[0].active = true;
  camps_[0].iv0 = iv0;
  camps_[0].iv5 = false;
}

void TeamCampSystem::clear_team_camps() {
  (void)kC2_Clear_team_camps;
  clear();
}

int TeamCampSystem::active_count() const {
  int n = 0;
  for (int i = 0; i < count_; ++i) if (camps_[i].active) ++n;
  return n;
}

void bot_team_check_situation(TeamCampSystem& camps, int team_iv127, Vec2 anchor, int camp_iv0) {
  (void)kC2_Bot_team_check_situation;
  if (team_iv127 == 3 && camps.active_count() == 0) camps.spawn_team_camp(anchor, camp_iv0);
}

} // namespace mdaz
