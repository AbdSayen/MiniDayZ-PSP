#include "mdaz/weather.hpp"
#include "mdaz/balance_globals.hpp"

namespace mdaz {

namespace {

// Audit anchor — C2 event group name (typo preserved in export).
const char kC2_Time_cyles_and_weather[] = "Time cyles and weather";

constexpr float kTimerWrap = 1440.f;
constexpr float kTimerStepSec = 1.5f; // Time cyles and weather: system Every 1.5s

void touch_weather_audit_anchors() {
  (void)kC2_Time_cyles_and_weather;
  // Check_loot / Hit_effect: rad_dist vs rad_phone (computed in Game from map POIs).
  static const char kC2_rad_dist[] = "rad_dist";
  (void)kC2_rad_dist;
}

bool hours_in_day_mode(int h) { return h >= 6 && h <= 23; }

float rain_intensity_from_level(int level) {
  if (level <= 0) return 0.f;
  if (level >= 4) return 1.f;
  return 0.25f + 0.2f * static_cast<float>(level);
}

} // namespace

WeatherState::WeatherState() {
  touch_weather_audit_anchors();
  reset();
}

void WeatherState::reset() {
  timer_all = static_cast<float>(balance::Timer_ALL);
  timer_step_accum_ = 0.f;
  rain_timer_accum_ = 0.f;
  rains_level = static_cast<int>(balance::rains_level);
  raining = balance::Rain != 0;
  day_rollover_pending_ = false;
  sync_clock_from_timer_all();
  rain = raining ? rain_intensity_from_level(rains_level) : 0.f;
  ambient_temp = day_mode ? 62.f : 38.f;
}

void WeatherState::sync_clock_from_timer_all() {
  while (timer_all >= kTimerWrap) timer_all -= kTimerWrap;
  while (timer_all < 0.f) timer_all += kTimerWrap;
  day_phase = timer_all / kTimerWrap;
  const int h = timer_hours();
  timer_minutes = static_cast<int>(timer_all) - h * 60;
  day_mode = hours_in_day_mode(h);
  is_night = !day_mode;
}

void WeatherState::sync_from_day_phase() {
  timer_all = day_phase * kTimerWrap;
  sync_clock_from_timer_all();
}

int WeatherState::timer_hours() const {
  return static_cast<int>(timer_all / 60.f);
}

void WeatherState::advance_game_minutes(float minutes) {
  if (minutes <= 0.f) return;
  timer_all += minutes;
  if (timer_all >= kTimerWrap) {
    timer_all -= kTimerWrap;
    day_rollover_pending_ = true;
  }
  sync_clock_from_timer_all();
}

bool WeatherState::consume_day_rollover() {
  if (!day_rollover_pending_) return false;
  day_rollover_pending_ = false;
  return true;
}

namespace {

void on_rain_speed_tick(WeatherState& w) {
  // Rain group (Time_cyles_and_weather.json): Timer Rain_speed → toggle Rain global.
  if (w.rains_level >= 4) w.rains_level = 0;
  if (w.raining) {
    w.rains_level = 0;
    w.raining = false;
  } else {
    w.raining = true;
    if (w.rains_level < 4) w.rains_level += 1;
  }
}

} // namespace

bool WeatherState::update(float dt) {
  const float rain_period = static_cast<float>(balance::Rain_speed);
  if (rain_period > 0.f) {
    rain_timer_accum_ += dt;
    while (rain_timer_accum_ >= rain_period) {
      rain_timer_accum_ -= rain_period;
      on_rain_speed_tick(*this);
    }
  }

  timer_step_accum_ += dt;
  while (timer_step_accum_ >= kTimerStepSec) {
    timer_step_accum_ -= kTimerStepSec;
    timer_all += 1.f;
    if (timer_all >= kTimerWrap) {
      timer_all = 0.f;
      day_rollover_pending_ = true;
    }
    sync_clock_from_timer_all();
  }

  if (raining)
    rain = rain_intensity_from_level(rains_level);
  else if (rain > 0.f) {
    rain -= dt * 0.35f;
    if (rain < 0.f) rain = 0.f;
  }

  ambient_temp = day_mode ? 62.f : 38.f;
  ambient_temp -= static_cast<float>(day_mode ? balance::Day_cold : balance::Night_cold);
  if (raining)
    ambient_temp -= static_cast<float>(balance::Rain_cold) *
                    rain_intensity_from_level(rains_level);

  return day_rollover_pending_;
}

} // namespace mdaz
