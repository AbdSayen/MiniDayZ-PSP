#include "mdaz/survival.hpp"

#include <cstdio>

int test_survival() {
  std::printf("--- test_survival ---\n");
  int fail = 0;

  mdaz::SurvivalRates nov = mdaz::rates_for_difficulty(mdaz::Difficulty_Novice);
  mdaz::SurvivalRates vet = mdaz::rates_for_difficulty(mdaz::Difficulty_Veteran);
  if (!(nov.starving < vet.starving)) {
    std::printf("FAIL novice starving should be < veteran\n");
    ++fail;
  } else {
    std::printf("OK   novice starving %g < veteran %g\n", nov.starving, vet.starving);
  }

  // Verified difficulty cold globals
  if (nov.day_cold != 6.f || nov.night_cold != 1.f || nov.rain_cold != 1.f) {
    std::printf("FAIL novice cold %g/%g/%g\n", nov.day_cold, nov.night_cold, nov.rain_cold);
    ++fail;
  } else {
    std::printf("OK   novice cold Day6 Night1 Rain1\n");
  }
  if (vet.day_cold != 8.f || vet.night_cold != 3.f || vet.rain_cold != 3.f) {
    std::printf("FAIL veteran cold\n");
    ++fail;
  } else {
    std::printf("OK   veteran cold Day8 Night3 Rain3\n");
  }

  // Every 5s: food -= Starving_current (0.75 novice)
  mdaz::SurvivalState s;
  s.reset_full();
  s.update(5.0f, nov, 50.f, false, false, false, false, false, false, false);
  if (s.food < 99.2f || s.food > 99.3f) {
    // 100 - 0.75 = 99.25
    std::printf("FAIL food after 5s got %g want ~99.25\n", s.food);
    ++fail;
  } else {
    std::printf("OK   Every-5s food drain %g (100-0.75)\n", s.food);
  }

  // Starvation blood damage: 1 per 5s when food==0
  s.food = 0.f;
  const float b0 = s.blood;
  s.update(5.0f, nov, 50.f, false, false, false, false, false, false, false);
  if (s.blood != b0 - 1.f) {
    std::printf("FAIL starve blood %g -> %g\n", b0, s.blood);
    ++fail;
  } else {
    std::printf("OK   starve blood -1 per 5s\n");
  }

  // Bleed Every 1s: -0.5 blood
  s.reset_full();
  s.bleeding = 1.f;
  s.update(1.0f, nov, 50.f, false, false, false, false, false, false, false);
  if (s.blood != 99.5f) {
    std::printf("FAIL bleed dmg got %g want 99.5\n", s.blood);
    ++fail;
  } else {
    std::printf("OK   bleed -0.5 /s\n");
  }

  // perk_metabolism halves bleed
  s.reset_full();
  s.bleeding = 1.f;
  s.update(1.0f, nov, 50.f, false, false, false, false, false, false, true);
  if (s.blood != 99.75f) {
    std::printf("FAIL metabolism bleed got %g\n", s.blood);
    ++fail;
  } else {
    std::printf("OK   perk_metabolism bleed -0.25 /s\n");
  }

  // perk_hamster reduces food drain by 0.3
  s.reset_full();
  s.update(5.0f, nov, 50.f, false, false, false, false, true, false, false);
  const float expect = 100.f - (0.75f - 0.3f);
  if (s.food < expect - 0.01f || s.food > expect + 0.01f) {
    std::printf("FAIL hamster food %g want %g\n", s.food, expect);
    ++fail;
  } else {
    std::printf("OK   perk_hamster food drain reduced\n");
  }

  return fail ? 1 : 0;
}
