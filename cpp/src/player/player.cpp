#include "mdaz/player.hpp"
#include "mdaz/balance_globals.hpp"

#include <cmath>

namespace mdaz {

Player::Player()
  : pos(kMapLayoutW * 0.5f, kMapLayoutH * 0.5f),
    vel(),
    facing_deg(0.f),
    move_speed(90.f),
    run_multiplier(1.55f),
    running(false),
    torch_on(false),
    nvg_on(false),
    difficulty(Difficulty_Novice),
    days_survived(1),
    zeds_killed(0),
    bandits_killed(0),
    animals_killed(0),
    fish_caught(0),
    forage_count(0),
    hear_radius(static_cast<float>(balance::Player_Hear_radius)),
    perk_camel(false),
    perk_hamster(false),
    perk_regen(false),
    perk_metabolism(false),
    perk_runner(false),
    perk_hunter(false),
    perk_farmer(false),
    perk_snowborn(false),
    cold_endure_t_(0.f),
    thirst_endure_t_(0.f),
    starve_endure_t_(0.f),
    wound_endure_t_(0.f),
    sprint_t_(0.f),
    bleed_endure_t_(0.f) {}

void Player::reset_new_game(Difficulty d) {
  difficulty = d;
  pos = Vec2(kMapLayoutW * 0.5f, kMapLayoutH * 0.5f);
  vel = Vec2();
  facing_deg = 0.f;
  running = false;
  torch_on = false;
  nvg_on = false;
  days_survived = static_cast<int>(balance::Day_survived);
  zeds_killed = 0;
  bandits_killed = 0;
  animals_killed = 0;
  fish_caught = 0;
  forage_count = 0;
  perk_camel = false;
  perk_hamster = false;
  perk_regen = false;
  perk_metabolism = false;
  perk_runner = false;
  perk_hunter = false;
  perk_farmer = false;
  perk_snowborn = false;
  cold_endure_t_ = 0.f;
  thirst_endure_t_ = 0.f;
  starve_endure_t_ = 0.f;
  wound_endure_t_ = 0.f;
  sprint_t_ = 0.f;
  bleed_endure_t_ = 0.f;
  survival.reset_full();
  hear_radius = static_cast<float>(balance::Player_Hear_radius);
}

void Player::apply_input(float move_x, float move_y, bool want_run, float dt) {
  running = want_run && (move_x != 0.f || move_y != 0.f);
  float len = move_x * move_x + move_y * move_y;
  if (len > 1.e-6f) {
    const float inv = 1.f / std::sqrt(len);
    move_x *= inv;
    move_y *= inv;
    facing_deg = std::atan2(move_y, move_x) * 57.2957795f;
  } else {
    move_x = 0.f;
    move_y = 0.f;
    running = false;
  }

  float run_mul = run_multiplier;
  if (perk_runner) run_mul += 0.2f; // stub until perk_runner extract
  const float spd = move_speed * (running ? run_mul : 1.f);
  vel = Vec2(move_x * spd, move_y * spd);
  pos.x += vel.x * dt;
  pos.y += vel.y * dt;

  if (pos.x < 0.f) pos.x = 0.f;
  if (pos.y < 0.f) pos.y = 0.f;
  if (pos.x > kMapLayoutW) pos.x = kMapLayoutW;
  if (pos.y > kMapLayoutH) pos.y = kMapLayoutH;
}

void Player::update_survival(float dt, float weather_temp, bool is_night, bool is_raining,
                             float clothing_insulation) {
  survival.update(dt, rates_for_difficulty(difficulty), weather_temp, running,
                  is_night, is_raining, perk_camel, perk_hamster, perk_regen, perk_metabolism,
                  clothing_insulation);
}

Aabb Player::bounds() const {
  Aabb a;
  a.x = pos.x - 12.f;
  a.y = pos.y - 12.f;
  a.w = 24.f;
  a.h = 24.f;
  return a;
}

} // namespace mdaz
