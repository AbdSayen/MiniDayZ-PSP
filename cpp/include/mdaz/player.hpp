#pragma once

#include "mdaz/perks.hpp"
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
  bool nvg_on;
  SurvivalState survival;
  Difficulty difficulty;
  int days_survived;
  int zeds_killed;
  int bandits_killed;
  int animals_killed;
  int fish_caught;
  int forage_count;
  float hear_radius;

  Player();

  void reset_new_game(Difficulty d);
  void apply_input(float move_x, float move_y, bool want_run, float dt);
  void update_survival(float dt, float weather_temp, bool is_night, bool is_raining,
                       float clothing_insulation = 0.f, bool vitamins_buff = false);
  Aabb bounds() const;

  PerkState perks; // C2 expierence_points / perk_* / perks_learnt_count / perk_cost
};

} // namespace mdaz
