#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

struct WeatherState {
  float day_phase;      // 0..1 (Timer_ALL / 1440)
  float timer_all;      // C2 Timer_ALL minutes 0..1440
  float rain;           // 0..1 intensity when Rain global active
  float ambient_temp;   // 0..100 scale used by survival
  bool is_night;
  bool raining;         // C2 Rain global
  float rain_phase_;    // Rain_speed oscillator

  WeatherState();
  void reset();
  void sync_from_day_phase();
  void update(float dt);
  int timer_hours() const;
};

} // namespace mdaz
