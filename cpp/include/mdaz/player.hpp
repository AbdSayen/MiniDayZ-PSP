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
  bool nvg_on;
  SurvivalState survival;
  Difficulty difficulty;
  int days_survived;
  int zeds_killed;
  int bandits_killed;
  int animals_killed;
  int fish_caught;
  int forage_count; // bush forages toward perk_farmer
  float hear_radius;

  Player();

  void reset_new_game(Difficulty d);
  void apply_input(float move_x, float move_y, bool want_run, float dt);
  void update_survival(float dt, float weather_temp, bool is_night, bool is_raining,
                       float clothing_insulation = 0.f, bool vitamins_buff = false);
  Aabb bounds() const;

  bool perk_camel;
  bool perk_hamster;
  bool perk_regen;
  bool perk_metabolism;
  bool perk_runner;
  bool perk_hunter; // extra meat on animal kills
  bool perk_farmer; // better bush forage odds
  bool perk_snowborn; // cold resistance after enduring freeze
  float cold_endure_t_; // seconds spent freezing toward snowborn
  float thirst_endure_t_; // seconds thirsty toward camel
  float starve_endure_t_; // seconds starving toward hamster
  float wound_endure_t_; // seconds low-blood toward regen
  float sprint_t_; // cumulative sprint toward runner
  float bleed_endure_t_; // cumulative bleeding toward metabolism
};

} // namespace mdaz
