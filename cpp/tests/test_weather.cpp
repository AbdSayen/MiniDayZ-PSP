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
  fail += expect_near("wrap Timer_ALL", w.timer_all, 0.f, 0.01f);
  if (!w.consume_day_rollover()) {
    std::printf("FAIL day rollover flag\n");
    ++fail;
  } else {
    std::printf("OK   Timer_ALL wrap 1440\n");
  }

  // Rain_speed toggle (360s default).
  mdaz::WeatherState r;
  r.raining = false;
  r.rains_level = 0;
  r.rain_timer_accum_ = 0.f;
  r.update(static_cast<float>(mdaz::balance::Rain_speed));
  if (!r.raining) {
    std::printf("FAIL Rain toggle on\n");
    ++fail;
  } else {
    std::printf("OK   Rain_speed toggle on (level %d)\n", r.rains_level);
  }
  r.update(static_cast<float>(mdaz::balance::Rain_speed));
  if (r.raining) {
    std::printf("FAIL Rain toggle off\n");
    ++fail;
  } else {
    std::printf("OK   Rain_speed toggle off\n");
  }

  if (fail == 0) std::printf("OK   weather timer/rain sverka\n");
  return fail > 0 ? 1 : 0;
}
