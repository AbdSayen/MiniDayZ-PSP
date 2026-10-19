#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

class Rng;

// C2 globals Timer_ALL / Timer_Hours / Timer_Minutes / Day_mode / Rain / rains_level
// (see cpp/docs_gen/groups/Time_cyles_and_weather.json + var_assignments.json).
struct WeatherState {
  float day_phase;        // Timer_ALL / 1440
  float timer_all;        // 0..1440 game minutes
  int timer_minutes;      // Timer_Minutes = Timer_ALL mod 60
  int rains_level;        // 0..4 storm tier (Rain group + mapgen)
  float rain;             // 0..1 render intensity derived from rains_level
  float ambient_temp;     // legacy HUD hint (not C2 Player_temperature)
  bool day_mode;          // Day_mode 1 when 6 <= Timer_Hours <= 23
  bool is_night;          // !day_mode (Update_Stats Night_cold uses Day_mode==0)
  bool raining;           // Rain global 0/1
  float timer_step_accum_; // accumulates toward Every 1.5s tick
  float rain_timer_accum_; // Rain_speed seconds between Rain toggles
  bool day_rollover_pending_;

  WeatherState();
  void reset();
  void sync_from_day_phase();
  void sync_clock_from_timer_all();
  // Returns true when Timer_ALL wrapped (Day_survived++ in game loop).
  // rng: choose(1,2,3,4) for the Rain timer; in_bunker blocks rain start (C2 inBunker).
  bool update(float dt, Rng* rng = 0, bool in_bunker = false);
  int timer_hours() const;
  // Sleep-at-fire helper: advance in-game minutes (same units as Timer_ALL).
  void advance_game_minutes(float minutes);
  bool consume_day_rollover();
};

} // namespace mdaz
