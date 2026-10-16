#include "mdaz/survival.hpp"
#include "mdaz/balance_globals.hpp"

namespace mdaz {

SurvivalRates rates_for_difficulty(Difficulty d) {
  SurvivalRates r;
  // Difficulty select in original overwrites cold globals (declaration init is 0):
  // Verified SetVar from data.js:
  //   Novice:  Night_cold=1, Rain_cold=1, Day_cold=6
  //   Regular: Night_cold=2, Rain_cold=2, Day_cold=7
  //   Veteran: Night_cold=3, Rain_cold=3, Day_cold=8
  switch (d) {
  case Difficulty_Regular:
    r.starving = static_cast<float>(balance::Regular_starving);
    r.thirsty = static_cast<float>(balance::Regular_thirsty);
    r.weathercold = static_cast<float>(balance::Regular_weathercold);
    r.regeneration = static_cast<float>(balance::Regular_regeneration);
    r.day_cold = 7.f;
    r.night_cold = 2.f;
    r.rain_cold = 2.f;
    break;
  case Difficulty_Veteran:
    r.starving = static_cast<float>(balance::Veteran_starving);
    r.thirsty = static_cast<float>(balance::Veteran_thirsty);
    r.weathercold = static_cast<float>(balance::Veteran_weathercold);
    r.regeneration = static_cast<float>(balance::Veteran_regeneration);
    r.day_cold = 8.f;
    r.night_cold = 3.f;
    r.rain_cold = 3.f;
    break;
  case Difficulty_Novice:
  default:
    r.starving = static_cast<float>(balance::Novice_starving);
    r.thirsty = static_cast<float>(balance::Novice_thirsty);
    r.weathercold = static_cast<float>(balance::Novice_weathercold);
    r.regeneration = static_cast<float>(balance::Novice_regeneration);
    r.day_cold = 6.f;
    r.night_cold = 1.f;
    r.rain_cold = 1.f;
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
  death_reason[0] = 0;
  tick_accum_ = 0.f;
  bleed_accum_ = 0.f;
}

namespace {

float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void set_reason(SurvivalState& s, const char* why) {
  int i = 0;
  for (; why[i] && i < 31; ++i) s.death_reason[i] = why[i];
  s.death_reason[i] = 0;
}

} // namespace

void SurvivalState::update(float dt, const SurvivalRates& rates, float weather_temp,
                           bool is_running, bool is_night, bool is_raining,
                           bool perk_camel, bool perk_hamster, bool perk_regen,
                           bool perk_metabolism) {
  (void)is_running;
  (void)weather_temp;

  // Bleed timer: States Every 1s while bleeding (IV22)
  // blood -= perk_metabolism ? 0.25 : 0.5
  if (bleeding > 0.f) {
    bleed_accum_ += dt;
    while (bleed_accum_ >= 1.0f) {
      bleed_accum_ -= 1.0f;
      blood -= perk_metabolism ? 0.25f : 0.5f;
    }
  }

  // Main meter tick: States Every 5s (sys.cnd.Every [5]) — VERIFIED via ACE decode
  tick_accum_ += dt;
  while (tick_accum_ >= 5.0f) {
    tick_accum_ -= 5.0f;

    float food_drain = rates.starving;
    float water_drain = rates.thirsty;
    if (perk_hamster) food_drain -= 0.3f;
    if (perk_camel) water_drain -= 0.3f;
    if (food_drain < 0.f) food_drain = 0.f;
    if (water_drain < 0.f) water_drain = 0.f;

    food -= food_drain;
    water -= water_drain;

    // Temperature delta mirrors Update_Stats → then States adds Player_temperature to IV16.
    // Update_Stats (verified):
    //   start Player_temperature = 0
    //   if Day_mode==0 (night): -= Night_cold
    //   if Rain==1 (exposed):   -= Rain_cold
    //   fire / bunker / perks add bonuses (not all wired yet)
    //   then subtract clothing-modified Day_cold (gear reduce TBD — use Day_cold bare)
    float delta = 0.f;
    if (is_night) delta -= rates.night_cold;
    if (is_raining) delta -= rates.rain_cold;
    delta -= rates.day_cold;
    // Without clothing, original still runs Day_cold path; fire/bunker not applied here yet.
    temperature += delta;

    food = clampf(food, 0.f, 100.f);
    water = clampf(water, 0.f, 100.f);
    temperature = clampf(temperature, 0.f, 100.f);

    if (food <= 0.f) {
      food = 0.f;
      blood -= 1.f;
      set_reason(*this, "Hunger");
    }
    if (water <= 0.f) {
      water = 0.f;
      blood -= 1.f;
      set_reason(*this, "Thirst");
    }
    if (temperature <= 0.f) {
      temperature = 0.f;
      blood -= 0.25f;
      set_reason(*this, "Cold");
    }

    // Regen when food, water, temp all >= 50 and not bleeding
    if (bleeding <= 0.f && food >= 50.f && water >= 50.f && temperature >= 50.f) {
      blood += rates.regeneration;
      if (perk_regen && blood < 50.f) blood += rates.regeneration;
    }
  }

  if (infected) blood -= 0.03f * dt;
  blood = clampf(blood, 0.f, 100.f);
}

} // namespace mdaz
