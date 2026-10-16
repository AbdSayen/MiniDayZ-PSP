#include "mdaz/weather.hpp"
#include "mdaz/balance_globals.hpp"

namespace mdaz {

WeatherState::WeatherState() { reset(); }

void WeatherState::reset() {
  day_phase = 0.35f; // morning-ish
  rain = 0.f;
  ambient_temp = 55.f;
  is_night = false;
}

void WeatherState::update(float dt) {
  // Day_Night_speed=720 and Rain_speed=360 are Construct timeline units.
  // Mapping: treat as "full cycle units per in-game day-equivalent".
  // Until the exact C2 expression is extracted, advance phase so that
  // higher speed => faster cycle. Documented as APPROXIMATE.
  const float day_speed = static_cast<float>(balance::Day_Night_speed);
  const float rain_speed = static_cast<float>(balance::Rain_speed);

  day_phase += (day_speed / 720.f) * (dt / 600.f); // ~10 min real => one cycle at default
  while (day_phase >= 1.f) day_phase -= 1.f;
  while (day_phase < 0.f) day_phase += 1.f;

  is_night = (day_phase < 0.25f || day_phase > 0.75f);

  // Simple rain pulse using rain_speed relative to day_speed
  rain += (rain_speed / 360.f) * 0.01f * dt;
  if (rain > 1.5f) rain = 0.f;
  const float rain_amount = (rain > 0.7f && rain < 1.2f) ? 1.f : 0.f;

  ambient_temp = is_night ? 30.f : 60.f;
  ambient_temp -= rain_amount * 15.f;
  // Incorporate original cold globals as offsets when active
  if (rain_amount > 0.f) ambient_temp -= static_cast<float>(balance::Rain_cold);
  if (is_night) ambient_temp -= static_cast<float>(balance::Night_cold) * 0.01f;
  else ambient_temp -= static_cast<float>(balance::Day_cold) * 0.01f;
}

} // namespace mdaz
