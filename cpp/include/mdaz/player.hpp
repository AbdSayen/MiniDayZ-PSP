#pragma once

#include "mdaz/survival.hpp"
#include "mdaz/types.hpp"

namespace mdaz {

struct Player {
  Vec2 pos;
  Vec2 vel;
  float facing_deg;
  float move_speed;
  float run_multiplier;
  bool running;
  bool torch_on;
  SurvivalState survival;
  Difficulty difficulty;
  int days_survived;
  int zeds_killed;
  int bandits_killed;
  float hear_radius;

  Player();

  void reset_new_game(Difficulty d);
  void apply_input(float move_x, float move_y, bool want_run, float dt);
  void update_survival(float dt, float weather_temp);
  Aabb bounds() const;
};

} // namespace mdaz
