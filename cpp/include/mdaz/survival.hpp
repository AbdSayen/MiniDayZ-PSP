#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

// Survival meters mirror original player instance vars on obj172:
//   IV13 food, IV14 water, IV15 blood, IV16 temperature, IV22 bleeding flag
// Rates come from Game_events globals; tick cadence from States (Every 5s / Bleed Every 1s).

struct SurvivalRates {
  float starving;       // Starving_current (per 5s food drain)
  float thirsty;        // Thirsty_current (per 5s water drain)
  float weathercold;    // Weathercold_current (legacy; often 0)
  float regeneration;   // Regeneration_current (per 5s blood regen)
  float day_cold;       // set with difficulty (Novice6/Regular7/Veteran8)
  float night_cold;     // Novice1/Regular2/Veteran3
  float rain_cold;      // Novice1/Regular2/Veteran3
};

SurvivalRates rates_for_difficulty(Difficulty d);

struct SurvivalState {
  float food;          // 0..100  (IV13)
  float water;         // 0..100  (IV14)
  float blood;         // 0..100  (IV15)
  float temperature;   // 0..100  (IV16)
  float bleeding;      // >0 means bleeding active (IV22)
  bool infected;
  char death_reason[32];

  SurvivalState();

  void reset_full();

  // dt in seconds. Perks match original globals perk_camel / perk_hamster / perk_regen / perk_metabolism.
  // clothing_insulation: reduces Day_cold (and part of Night/Rain) — stub until C2 gear extract.
  void update(float dt, const SurvivalRates& rates, float weather_temp,
              bool is_running, bool is_night, bool is_raining,
              bool perk_camel, bool perk_hamster, bool perk_regen,
              bool perk_metabolism, float clothing_insulation = 0.f);

  bool is_dead() const { return blood <= 0.f; }

private:
  float tick_accum_;
  float bleed_accum_;
};

} // namespace mdaz
