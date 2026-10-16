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

  mdaz::SurvivalState s;
  s.reset_full();
  const float blood0 = s.blood;
  // Simulate long starvation
  for (int i = 0; i < 5000; ++i) {
    s.food = 0.f;
    s.water = 0.f;
    s.update(0.1f, nov, 50.f, false);
  }
  if (!(s.blood < blood0)) {
    std::printf("FAIL blood should drop when starving\n");
    ++fail;
  } else {
    std::printf("OK   starvation damages blood (%g -> %g)\n", blood0, s.blood);
  }

  s.reset_full();
  s.bleeding = 5.f;
  const float b1 = s.blood;
  s.update(1.0f, nov, 50.f, false);
  if (!(s.blood < b1)) {
    std::printf("FAIL bleeding should reduce blood\n");
    ++fail;
  } else {
    std::printf("OK   bleeding reduces blood\n");
  }

  return fail ? 1 : 0;
}
