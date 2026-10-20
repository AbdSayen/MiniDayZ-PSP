#pragma once

#include "mdaz/mapgen.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/types.hpp"

namespace mdaz {

// Hostile survivors / bots — scaffold until full Bot_* C2 groups are ported.
struct Bandit {
  EntityId id;
  Vec2 pos;
  float hp;
  float max_hp;
  float speed;
  float aggro_radius;
  float attack_cooldown;
  bool alive;
  bool has_gun;
  float burn_t = 0.f; // C2 molotov burning seconds left
  bool is_ally; // C2 obj522 friend / team bot (Remove_bot_friend / Remove_team_bots)
  int weapon_iv14; // C2 obj522 IV14 — bot_reloading mag table key
  int weapon_iv11; // C2 obj827 IV11 — bandit827 firearm audio (data.js fire events)
  int mag_rounds;  // IV9 rounds in mag
  bool reloading;  // IV10 reload in progress
  float reload_timer;
  // C2 object: 827 bot_enemy_bandit (bandit_skin, layer player), 243 shooter / 540 puncher
  // (camp bots, layer Survivors), 522 friend (Survivors).
  int c2_obj = 827;
  // Bot_puncher_zindex: pinned t943 skin (IV1 = 1) on layer Survivors2 (10) instead of Survivors (22).
  bool layer2 = false;
  short ov928[4] = {-1, -1, -1, -1};   // t928 instances overlapped last tick (OnCollision edge)
  Bandit();
  void setup(Vec2 p, EntityId eid, bool gun, bool ally = false, int weapon_iv14 = 1,
             int weapon_iv11 = 0);
};

static const int kMaxBandits = 32;

class BanditSystem {
public:
  BanditSystem();
  void clear();
  Bandit* spawn(Vec2 p, bool gun, bool ally = false);
  // Melee damage returned; gun shots handled by Game via attack_cooldown.
  float update(float dt, Vec2 player_pos);
  int count() const { return count_; }
  Bandit* at(int i) { return &bandits_[i]; }
  Bandit* begin() { return bandits_; }
  Bandit* end() { return bandits_ + count_; }
  const Bandit* begin() const { return bandits_; }
  const Bandit* end() const { return bandits_ + count_; }
  int alive_count() const;

private:
  Bandit bandits_[kMaxBandits];
  int count_;
  EntityId next_id_;
};

// C2 Spawn_bandit_camp + Spawn_firearm_bot / Spawn_puncher_bot at camp anchor (obj245).
void spawn_bandit_camp(BanditSystem& bandits, Vec2 anchor, Rng& rng);
// C2 Remove_bandit_bots — quest camp cleanup near anchor.
void remove_bandit_bots(BanditSystem& bandits);

// C2 NPCs: destroy bot instances by role when they die or quest ends.
void remove_bot_firearm(Bandit* bot);
void remove_bot_puncher(Bandit* bot);
void remove_bot_friend(Bandit* bot);
void remove_team_bots(BanditSystem& bandits);

// C2 Spawn_friend_bot: obj245 marker within 2000px → obj522 "Veteran friend" (ally puncher).
Bandit* spawn_friend_bot(BanditSystem& bandits, Vec2 player_pos, Vec2 marker245, Rng& rng);

static const int kMaxTeamCamps = 8;
struct TeamCampMarker {
  Vec2 pos;
  bool active;
  int iv0;  // C2 obj507 IV0 (generate_mapgen_toelements / generate_array_tomap)
  bool iv5; // Spawn_bandit_camp sets IV5=1 when camp populated
  TeamCampMarker() : pos(), active(false), iv0(0), iv5(false) {}
};

// C2 Spawn_bandit_camp: eligible IV0 when IV3/IV4 == 0.
bool team_camp_iv0_allows_bandit_camp(int iv0);

// IV0 from generate_array_tomap sector tag (approx; full generate_mapgen_toelements blocked).
int team_camp_iv0_for_location(LocationType t, Rng& rng);

class TeamCampSystem {
public:
  TeamCampSystem();
  void clear();
  // C2 Spawn_team_camp (obj507): camp anchor at pos.
  void spawn_team_camp(Vec2 pos, int iv0 = 9);
  // C2 Clear_team_camps: destroy all obj507.
  void clear_team_camps();
  int active_count() const;
  int count() const { return count_; }
  TeamCampMarker* at(int i) { return &camps_[i]; }
  const TeamCampMarker* at(int i) const { return &camps_[i]; }

private:
  TeamCampMarker camps_[kMaxTeamCamps];
  int count_;
};

// C2 Bot_team_check_situation: obj549 IV127==3 → Spawn_team_camp.
void bot_team_check_situation(TeamCampSystem& camps, int team_iv127, Vec2 anchor, int camp_iv0);

// C2 Bot_bandits_check_situation (CurrentLevel>0 only): Count<=0 -> camp; quest active, not quest camp, nearest bandit >1200 -> Remove_bandit_bots.
void bot_bandits_check_situation(BanditSystem& bandits, TeamCampSystem& camps, Vec2 player_pos,
                                 int current_level, bool controller_iv2, int q_status, Rng& rng);

// C2 bot_reloading (obj522): when IV9==0, IV10=1, Wait, refill IV9 from IV14 weapon class.
int bot_mag_capacity_for_weapon_iv14(int weapon_iv14);
float bot_reload_duration_sec(int weapon_iv14);
void bot_tick_mag_reload(Bandit& b, float dt);

// C2 Bullets_dmg_deal: obj910 IV1 = -15 for bandit827 shots at player.
constexpr int kBotBanditBulletIv1 = -15;

// bandit827 IV11 → PlayAtObject fire name (original_docs/data.js, tag m4/akm).
const char* bot_bandit_fire_sfx(int weapon_iv14, int weapon_iv11, Rng& rng);

// Default IV11 for camp firearm bots paired with kCampIv14 spawn table.
int bot_bandit_iv11_for_iv14(int weapon_iv14);

} // namespace mdaz
