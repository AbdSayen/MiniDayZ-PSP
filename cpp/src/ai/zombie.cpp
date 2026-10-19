#include "mdaz/los.hpp"
#include "mdaz/zombie.hpp"
#include "mdaz/combat.hpp"
#include "mdaz/balance_globals.hpp"

#include <cmath>

namespace mdaz {

Zombie::Zombie()
  : id(0), type(Zed_Normal), pos(), hp(0), max_hp(0), speed(40.f),
    aggro_radius(220.f), alive(false), attack_cooldown(0.f), ambush_boost(0.f), warn_t(0.f), warn_dir(), warn_spd(0.f), burn_t(0.f) {}

void Zombie::setup(ZedType t, Vec2 p, EntityId eid) {
  id = eid;
  burn_t = 0.f;
  warn_t = 0.f;
  type = t;
  pos = p;
  alive = true;
  attack_cooldown = 0.f;
  ambush_boost = 0.f;
  // HP = skin IV0 set at spawn, chase speed = eyes "attack" (Bullet) speed set at spawn
  // (data.js Zeds sheet, 'Zed * Puncher' groups). The random(lo, hi) speed roll is taken from a
  // hash of the entity id so setup() stays deterministic without an RNG argument.
  // Aggro radii are still port values (MainLook/ShortLook ranges not ported yet).
  const unsigned hsh = static_cast<unsigned>(eid) * 2654435761u;
  const float u = static_cast<float>((hsh >> 8) & 0xFFFFu) / 65535.f;
  switch (t) {
  case Zed_Fast:     // zed_fast_skin1..3 IV0=80, speed random(120,140)
    max_hp = 80.f;
    speed = 120.f + 20.f * u;
    aggro_radius = 280.f;
    break;
  case Zed_Army:     // zed_army_skin1..3 IV0=100, speed random(90,100)
    max_hp = 100.f;
    speed = 90.f + 10.f * u;
    aggro_radius = 300.f;
    break;
  case Zed_ArmyShooter: // zed_shooter_skin3 IV0=100 (skin 125, skin2 175), speed 20+random(10)
    max_hp = 100.f;
    speed = 20.f + 10.f * u;
    aggro_radius = static_cast<float>(balance::Player_Hear_radius) * 0.45f;
    break;
  case Zed_Tank:     // zed_tank_skin IV0=1000, attack speed 110 (200 while rushing)
    max_hp = 1000.f;
    speed = 110.f;
    aggro_radius = 340.f;
    break;
  case Zed_Screamer: // zed_screamer_skin IV2=50, speed random(30,40)
    max_hp = 50.f;
    speed = 30.f + 10.f * u;
    aggro_radius = 360.f;
    break;
  case Zed_Buried:   // zed_buried_skin IV0=60, speed random(90,110)
    max_hp = 60.f;
    speed = 90.f + 20.f * u;
    aggro_radius = 160.f;
    break;
  case Zed_Normal:   // zed_normal_skin1..6 IV0=60 (skin7 80), speed random(90,100)
  default:
    max_hp = 60.f;
    speed = 90.f + 10.f * u;
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
  // Tank: slow but hard-hitting; larger melee reach
  float melee_reach = 28.f;
  if (type == Zed_Tank) {
    melee_reach = 36.f;
  }

  // C2 MainLook/ShortLook obstacles = buildings (t927) + big obstacles (t339).
  const bool sees = !los_blocked(pos, player_pos);
  if (sees && dist < hear && dist > 1.f) {
    pos.x += (dx / dist) * move_spd * dt;
    pos.y += (dy / dist) * move_spd * dt;
    warn_t = 0.f;
  } else if (warn_t > 0.f) {
    pos.x += warn_dir.x * warn_spd * dt;
    pos.y += warn_dir.y * warn_spd * dt;
    warn_t -= dt;
  }

  if (sees && dist < melee_reach && attack_cooldown <= 0.f) {
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
                          bool player_crouch, bool player_bleeding, bool player_coughing) {
  damage_to_player = 0.f;
  scream_active_ = false;
  float aggro_mul = 1.f;
  if (player_running) aggro_mul = 1.45f;
  else if (player_crouch) aggro_mul = 0.55f;
  if (player_bleeding) aggro_mul *= 1.35f; // blood scent pulls infected
  if (player_coughing) aggro_mul *= 1.2f;  // cough noise

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

void ZombieSystem::noise(Vec2 at, float radius) {
  const float r = radius + 200.f;
  for (int i = 0; i < count_; ++i) {
    Zombie& z = zombies_[i];
    if (!z.alive) continue;
    const float dx = at.x - z.pos.x, dy = at.y - z.pos.y;
    if (dx * dx + dy * dy > r * r) continue;
    // Target = noise point + random(-50, 50) per axis; speed random(80, 90). Rolls from a hash
    // of (id, noise point) to stay deterministic without an RNG argument.
    unsigned h = static_cast<unsigned>(z.id) * 2654435761u ^ static_cast<unsigned>(at.x * 7.f + at.y * 13.f);
    h = h * 1103515245u + 12345u; const float u1 = ((h >> 8) & 0xFFFF) / 65535.f;
    h = h * 1103515245u + 12345u; const float u2 = ((h >> 8) & 0xFFFF) / 65535.f;
    h = h * 1103515245u + 12345u; const float u3 = ((h >> 8) & 0xFFFF) / 65535.f;
    const float tx = at.x + (u1 * 100.f - 50.f) - z.pos.x, ty = at.y + (u2 * 100.f - 50.f) - z.pos.y;
    const float l = std::sqrt(tx * tx + ty * ty);
    if (l < 1.f) continue;
    z.warn_dir = Vec2(tx / l, ty / l);
    z.warn_spd = 80.f + u3 * 10.f;
    z.warn_t = 2.f;
  }
}

int ZombieSystem::alive_count() const {
  int n = 0;
  for (int i = 0; i < count_; ++i) if (zombies_[i].alive) ++n;
  return n;
}

} // namespace mdaz
