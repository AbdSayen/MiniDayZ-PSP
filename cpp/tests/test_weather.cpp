#include "mdaz/rng.hpp"
#include "mdaz/balance_globals.hpp"
#include "mdaz/weather.hpp"

#include <cmath>
#include <cstdio>

namespace {

int expect_eq(const char* label, int a, int b) {
  if (a == b) return 0;
  std::printf("FAIL %s expected %d got %d\n", label, b, a);
  return 1;
}

int expect_near(const char* label, float a, float b, float eps) {
  if (std::fabs(a - b) <= eps) return 0;
  std::printf("FAIL %s expected %.4f got %.4f\n", label, b, a);
  return 1;
}

} // namespace

int test_weather() {
  std::printf("--- test_weather (Timer_ALL / Rain_speed) ---\n");
  int fail = 0;

  mdaz::WeatherState w;
  fail += expect_near("Timer_ALL init", w.timer_all, static_cast<float>(mdaz::balance::Timer_ALL),
                      0.01f);
  fail += expect_eq("Timer_Hours init", w.timer_hours(), 6); // 360/60
  fail += expect_eq("Day_mode noon band", w.day_mode ? 1 : 0, 1);

  // C2: +1 minute every 1.5s wall time.
  const bool rolled = w.update(1.5f);
  (void)rolled;
  fail += expect_near("after 1.5s tick", w.timer_all,
                      static_cast<float>(mdaz::balance::Timer_ALL) + 1.f, 0.01f);
  fail += expect_eq("Timer_Minutes sync", w.timer_minutes, 1);

  // Advance to hour 5 → night (Day_mode 0).
  w.timer_all = 5.f * 60.f + 10.f;
  w.sync_clock_from_timer_all();
  fail += expect_eq("hour 5 night", w.day_mode ? 1 : 0, 0);
  fail += expect_eq("hour 5 is Timer_Hours", w.timer_hours(), 5);

  w.timer_all = 6.f * 60.f;
  w.sync_clock_from_timer_all();
  fail += expect_eq("hour 6 day", w.day_mode ? 1 : 0, 1);

  // Wrap at 1440 → Day_survived handled via consume_day_rollover.
  w.timer_all = 1439.f;
  w.timer_step_accum_ = 0.f;
  w.day_rollover_pending_ = false;
  w.update(1.5f);
  fail += expect_near("Timer_ALL 1440 not yet wrapped", w.timer_all, 1440.f, 0.01f);
  w.update(1.5f);
  fail += expect_near("wrap Timer_ALL", w.timer_all, 0.f, 0.01f);
  if (!w.consume_day_rollover()) {
    std::printf("FAIL day rollover flag\n");
    ++fail;
  } else {
    std::printf("OK   Timer_ALL wrap 1440\n");
  }

  // C2 Rain timer: off if raining; else choose(1..4)==1 and not in bunker; level 0 never rains.
  {
    mdaz::WeatherState r; mdaz::Rng rr(7);
    const float P = static_cast<float>(mdaz::balance::Rain_speed);
    r.raining = false; r.rains_level = 0; r.rain_timer_accum_ = 0.f;
    r.update(P, &rr, false);
    fail += expect_eq("rains_level 0 never rains", r.raining ? 1 : 0, 0);
    r.rains_level = 1;
    int on = 0;
    for (int k = 0; k < 400; ++k) { r.raining = false; r.update(P, &rr, false); on += r.raining ? 1 : 0; }
    if (on < 60 || on > 140) { std::printf("FAIL rain start rate %d/400\n", on); ++fail; }
    else std::printf("OK   rain start ~1/4 (%d/400)\n", on);
    fail += expect_eq("rains_level unchanged", r.rains_level, 1);
    r.raining = false;
    for (int k = 0; k < 50; ++k) r.update(P, &rr, true);
    fail += expect_eq("no rain start in bunker", r.raining ? 1 : 0, 0);
    r.raining = true; r.update(P, &rr, false);
    fail += expect_eq("rain stops on next tick", r.raining ? 1 : 0, 0);
  }

  if (fail == 0) std::printf("OK   weather timer/rain sverka\n");
  return fail > 0 ? 1 : 0;
}
