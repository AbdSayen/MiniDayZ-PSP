#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

struct WeatherState {
  float day_phase;      // 0..1 (0=midnight-ish cycle position)
  float rain;           // 0..1
  float ambient_temp;   // 0..100 scale used by survival
  bool is_night;

  WeatherState();
  void reset();
  void update(float dt);
};

} // namespace mdaz
