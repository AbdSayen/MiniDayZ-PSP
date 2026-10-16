#include "mdaz/survival.hpp"
#include "mdaz/balance_globals.hpp"

namespace mdaz {

SurvivalRates rates_for_difficulty(Difficulty d) {
  SurvivalRates r;
  switch (d) {
  case Difficulty_Regular:
    r.starving = static_cast<float>(balance::Regular_starving);
    r.thirsty = static_cast<float>(balance::Regular_thirsty);
    r.weathercold = static_cast<float>(balance::Regular_weathercold);
    r.regeneration = static_cast<float>(balance::Regular_regeneration);
    break;
  case Difficulty_Veteran:
    r.starving = static_cast<float>(balance::Veteran_starving);
    r.thirsty = static_cast<float>(balance::Veteran_thirsty);
    r.weathercold = static_cast<float>(balance::Veteran_weathercold);
    r.regeneration = static_cast<float>(balance::Veteran_regeneration);
    break;
  case Difficulty_Novice:
  default:
    r.starving = static_cast<float>(balance::Novice_starving);
    r.thirsty = static_cast<float>(balance::Novice_thirsty);
    r.weathercold = static_cast<float>(balance::Novice_weathercold);
    r.regeneration = static_cast<float>(balance::Novice_regeneration);
    break;
  }
  return r;
}

SurvivalState::SurvivalState() { reset_full(); }

void SurvivalState::reset_full() {
  food = 100.f;
  water = 100.f;
  blood = 100.f;
  temperature = 50.f;
  bleeding = 0.f;
  infected = false;
}

namespace {

float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// Base drain per second at rate multiplier 1.0.
// Calibrated so that with Regular_starving=1.1, food lasts on the order of
// tens of minutes of play — exact C2 timers are still being extracted from
// "Player stats" / "Time cyles" groups. Documented as APPROXIMATE until sverka.
const float kBaseFoodDrainPerSec = 0.015f;
const float kBaseWaterDrainPerSec = 0.020f;
const float kBleedDpsPerUnit = 0.35f;
const float kStarvationDps = 0.05f;
const float kDehydrationDps = 0.08f;
const float kColdDps = 0.04f;
const float kBaseRegenPerSec = 0.08f;

} // namespace

void SurvivalState::update(float dt, const SurvivalRates& rates, float weather_temp, bool is_running) {
  const float run_mult = is_running ? 1.35f : 1.0f;

  food -= kBaseFoodDrainPerSec * rates.starving * run_mult * dt;
  water -= kBaseWaterDrainPerSec * rates.thirsty * run_mult * dt;
  food = clampf(food, 0.f, 100.f);
  water = clampf(water, 0.f, 100.f);

  // Temperature drifts toward weather; cold rates amplify cooling.
  const float target = weather_temp;
  const float cool = (temperature > target) ? (0.15f * rates.weathercold) : 0.10f;
  temperature += (target - temperature) * cool * dt;
  temperature = clampf(temperature, 0.f, 100.f);

  if (bleeding > 0.f) {
    blood -= bleeding * kBleedDpsPerUnit * dt;
    bleeding = clampf(bleeding - 0.02f * dt, 0.f, 100.f);
  }

  if (food <= 0.f) blood -= kStarvationDps * rates.starving * dt;
  if (water <= 0.f) blood -= kDehydrationDps * rates.thirsty * dt;
  if (temperature < 20.f) blood -= kColdDps * rates.weathercold * dt;

  // Regen when fed, hydrated, warm, not bleeding — scaled by difficulty regen global.
  if (bleeding <= 0.f && food > 40.f && water > 40.f && temperature > 35.f && blood < 100.f) {
    blood += kBaseRegenPerSec * rates.regeneration * dt;
  }

  if (infected) {
    blood -= 0.03f * dt;
  }

  blood = clampf(blood, 0.f, 100.f);
}

} // namespace mdaz
