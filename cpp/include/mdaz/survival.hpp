#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

// Survival meters mirror original HUD: food, water, blood/health, heat.
// Rates come from Game_events globals (Novice_/Regular_/Veteran_*).
struct SurvivalRates {
  float starving;      // food drain multiplier
  float thirsty;       // water drain multiplier
  float weathercold;   // cold exposure multiplier
  float regeneration;  // blood regen multiplier when fed/warm
};

SurvivalRates rates_for_difficulty(Difficulty d);

struct SurvivalState {
  float food;          // 0..100
  float water;         // 0..100
  float blood;         // 0..100 (health)
  float temperature;   // player body heat proxy
  float bleeding;      // bleed intensity (>=0)
  bool infected;

  SurvivalState();

  void reset_full();
  void update(float dt, const SurvivalRates& rates, float weather_temp, bool is_running);
  bool is_dead() const { return blood <= 0.f; }
};

} // namespace mdaz
