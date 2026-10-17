#include "mdaz/zombie.hpp"
#include "mdaz/combat.hpp"
#include "mdaz/balance_globals.hpp"

#include <cmath>

namespace mdaz {

Zombie::Zombie()
  : id(0), type(Zed_Normal), pos(), hp(0), max_hp(0), speed(40.f),
    aggro_radius(220.f), alive(false), attack_cooldown(0.f), ambush_boost(0.f) {}

void Zombie::setup(ZedType t, Vec2 p, EntityId eid) {
  id = eid;
  type = t;
  pos = p;
  alive = true;
  attack_cooldown = 0.f;
  ambush_boost = 0.f;
  // Speeds / HP are stubs pending full Zeds_common extract.
  // Aggro uses Player_Hear_radius (1000) as upper sight budget for shooters;
  // melee types use shorter radii from observed behavior groups.
  switch (t) {
  case Zed_Fast:
    max_hp = 60.f;
    speed = 95.f;
    aggro_radius = 280.f;
    break;
  case Zed_Army:
    max_hp = 120.f;
    speed = 55.f;
    aggro_radius = 300.f;
    break;
  case Zed_ArmyShooter:
    max_hp = 110.f;
    speed = 50.f;
    aggro_radius = static_cast<float>(balance::Player_Hear_radius) * 0.45f;
    break;
  case Zed_Tank:
    max_hp = 400.f;
    speed = 35.f;
    aggro_radius = 340.f;
    break;
  case Zed_Screamer:
    max_hp = 80.f;
    speed = 50.f;
    aggro_radius = 360.f; // alerts others — scream radius TBD
    break;
  case Zed_Buried:
    max_hp = 90.f;
    speed = 45.f;
    aggro_radius = 160.f; // ambush — jumps when close
    break;
  case Zed_Normal:
  default:
    max_hp = 100.f;
    speed = 48.f;
    aggro_radius = 220.f;
    break;
  }
  hp = max_hp;
}

void Zombie::update_ai(float dt, Vec2 player_pos, bool& out_hit_player, float& out_damage,
                       float aggro_mul) {
  out_hit_player = false;
  out_damage = 0.f;
  if (!alive) return;
  if (attack_cooldown > 0.f) attack_cooldown -= dt;

  const float dx = player_pos.x - pos.x;
  const float dy = player_pos.y - pos.y;
  const float dist = std::sqrt(dx * dx + dy * dy);
  const float hear = aggro_radius * (aggro_mul > 0.1f ? aggro_mul : 1.f);

  // Buried: stay put until player enters ambush radius, then rush with boost.
  if (type == Zed_Buried && dist > hear) {
    return;
  }
  if (type == Zed_Buried && ambush_boost <= 0.f && dist <= hear) {
    ambush_boost = 1.6f; // pop-up sprint
    // Flag just-sprung via brief boost > 1.55 (Game listens for audio)
  }
  if (ambush_boost > 0.f) ambush_boost -= dt;

  // Army shooter prefers standoff — slow approach, shots handled in Game.
  float move_spd = speed;
  if (type == Zed_ArmyShooter && dist < 200.f) move_spd *= 0.35f;
  if (type == Zed_Buried && ambush_boost > 0.f) move_spd *= 2.4f;
  // Tank: slow but hard-hitting; larger melee reach
  float melee_reach = 28.f;
  if (type == Zed_Tank) {
    move_spd *= 0.85f;
    melee_reach = 36.f;
  }

  if (dist < hear && dist > 1.f) {
    pos.x += (dx / dist) * move_spd * dt;
    pos.y += (dy / dist) * move_spd * dt;
  }

  if (dist < melee_reach && attack_cooldown <= 0.f) {
    out_hit_player = true;
    out_damage = zed_melee_damage(type);
    if (type == Zed_Fast) attack_cooldown = 0.7f;
    else if (type == Zed_Tank) attack_cooldown = 1.4f;
    else attack_cooldown = 1.0f;
  }
}

ZombieSystem::ZombieSystem() : count_(0), next_id_(1), scream_active_(false), scream_pos_() {}

void ZombieSystem::clear() { count_ = 0; scream_active_ = false; scream_pos_ = Vec2(); }

Zombie* ZombieSystem::spawn(ZedType t, Vec2 p) {
  if (count_ >= kMaxZombies) return 0;
  Zombie& z = zombies_[count_++];
  z.setup(t, p, next_id_++);
  return &z;
}

void ZombieSystem::update(float dt, Vec2 player_pos, float& damage_to_player, bool player_running,
                          bool player_crouch, bool player_bleeding, bool player_coughing,
                          bool is_night) {
  damage_to_player = 0.f;
  scream_active_ = false;
  float aggro_mul = 1.f;
  if (player_running) aggro_mul = 1.45f;
  else if (player_crouch) aggro_mul = 0.55f;
  if (player_bleeding) aggro_mul *= 1.35f; // blood scent pulls infected
  if (player_coughing) aggro_mul *= 1.2f;  // cough noise
  if (is_night) aggro_mul *= 1.25f;        // night hunters

  // Screamer alert: if a screamer is within its aggro of the player, pull nearby zeds in.
  Vec2 scream_pos;
  for (int i = 0; i < count_; ++i) {
    if (!zombies_[i].alive || zombies_[i].type != Zed_Screamer) continue;
    const float dx = player_pos.x - zombies_[i].pos.x;
    const float dy = player_pos.y - zombies_[i].pos.y;
    const float ar = zombies_[i].aggro_radius * aggro_mul;
    if (dx * dx + dy * dy < ar * ar) {
      scream_active_ = true;
      scream_pos = zombies_[i].pos;
      break;
    }
  }
  if (scream_active_) {
    for (int i = 0; i < count_; ++i) {
      if (!zombies_[i].alive) continue;
      const float dx = scream_pos.x - zombies_[i].pos.x;
      const float dy = scream_pos.y - zombies_[i].pos.y;
      if (dx * dx + dy * dy < 420.f * 420.f) {
        // Temporarily enlarge aggro so they path toward player
        if (zombies_[i].aggro_radius < 400.f) zombies_[i].aggro_radius = 400.f;
      }
    }
  }
  scream_pos_ = scream_pos;

  for (int i = 0; i < count_; ++i) {
    bool hit = false;
    float dmg = 0.f;
    zombies_[i].update_ai(dt, player_pos, hit, dmg, aggro_mul);
    if (hit) damage_to_player += dmg;
  }
}

int ZombieSystem::alive_count() const {
  int n = 0;
  for (int i = 0; i < count_; ++i) if (zombies_[i].alive) ++n;
  return n;
}

} // namespace mdaz
