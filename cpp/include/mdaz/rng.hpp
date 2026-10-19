#pragma once

#include <cstdint>

namespace mdaz {

// Deterministic LCG for reproducible tests / seeded runs (SEED_* globals in original).
class Rng {
public:
  explicit Rng(std::uint32_t seed = 1u) : state_(seed ? seed : 1u) {}

  void seed(std::uint32_t s) { state_ = s ? s : 1u; }
  std::uint32_t next_u32();
  float next_float(); // [0,1)
  int next_int(int min_inclusive, int max_inclusive);
  float range(float min_v, float max_v);

  // Two draws in a fixed order (x first, then y). C++ leaves the evaluation order of
  // function/constructor arguments unspecified: MSVC and psp-gcc evaluated
  // f(rng.next_int(), rng.next_int()) in opposite orders, which swapped the x/y sector of every
  // POI between the desktop and the PSP build (same seed, different map). Always use these.
  void int_pair(int lo_a, int hi_a, int lo_b, int hi_b, int& a, int& b) {
    a = next_int(lo_a, hi_a);
    b = next_int(lo_b, hi_b);
  }
  void range_pair(float lo_a, float hi_a, float lo_b, float hi_b, float& a, float& b) {
    a = range(lo_a, hi_a);
    b = range(lo_b, hi_b);
  }

private:
  std::uint32_t state_;
};

} // namespace mdaz
