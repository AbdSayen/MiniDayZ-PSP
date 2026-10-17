#include "mdaz/weather.hpp"
#include "mdaz/balance_globals.hpp"

namespace mdaz {

namespace {
// C2: Timer_ALL wraps at 1440; Day_Night_speed=720 advances the in-game clock.
constexpr float kTimerWrap = 1440.f;
constexpr float kTimerStart = 360.f; // matches balance Timer_ALL initial

bool hours_are_night(int h) {
  // Day&Night group: dusk after ~19, dawn before ~6 (Timer_Hours checks).
  return h < 6 || h >= 19;
}
} // namespace

WeatherState::WeatherState() { reset(); }

void WeatherState::reset() {
  timer_all = kTimerStart;
  day_phase = timer_all / kTimerWrap;
  rain = 0.f;
  raining = false;
  rain_phase_ = 0.f;
  ambient_temp = 55.f;
  is_night = hours_are_night(timer_hours());
}

int WeatherState::timer_hours() const {
  return static_cast<int>(timer_all / 60.f) % 24;
}

void WeatherState::sync_from_day_phase() {
  timer_all = day_phase * kTimerWrap;
  is_night = hours_are_night(timer_hours());
}

void WeatherState::update(float dt) {
  const float day_speed = static_cast<float>(balance::Day_Night_speed);
  const float rain_speed = static_cast<float>(balance::Rain_speed);

  // One full Timer_ALL cycle in Day_Night_speed wall seconds (720s default).
  timer_all += dt * (kTimerWrap / day_speed);
  while (timer_all >= kTimerWrap) timer_all -= kTimerWrap;
  while (timer_all < 0.f) timer_all += kTimerWrap;
  day_phase = timer_all / kTimerWrap;

  is_night = hours_are_night(timer_hours());

  // Rain_speed timer toggles Rain global; pulse rain intensity while active.
  rain_phase_ += dt * (kTimerWrap / rain_speed);
  if (rain_phase_ >= kTimerWrap) {
    rain_phase_ -= kTimerWrap;
    raining = !raining;
  }
  if (raining) {
    rain += dt * 0.35f;
    if (rain > 1.f) rain = 1.f;
  } else {
    rain -= dt * 0.25f;
    if (rain < 0.f) rain = 0.f;
  }

  ambient_temp = is_night ? 38.f : 62.f;
  ambient_temp -= static_cast<float>(is_night ? balance::Night_cold : balance::Day_cold);
  if (rain > 0.25f)
    ambient_temp -= static_cast<float>(balance::Rain_cold) * rain;
}

} // namespace mdaz
