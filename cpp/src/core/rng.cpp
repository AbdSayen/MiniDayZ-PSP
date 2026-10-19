#include "mdaz/rng.hpp"

namespace mdaz {

std::uint32_t Rng::next_u32() {
  // Numerical Recipes LCG
  state_ = state_ * 1664525u + 1013904223u;
  return state_;
}

float Rng::next_float() {
  return (next_u32() >> 8) * (1.0f / 16777216.0f);
}

int Rng::next_int(int min_inclusive, int max_inclusive) {
  if (max_inclusive <= min_inclusive) return min_inclusive;
  const int span = max_inclusive - min_inclusive + 1;
  // High bits via 64-bit multiply (Lemire range reduction without rejection):
  // LCG low bits have tiny periods (bit0 alternates), so `% span` was biased.
  const std::uint64_t r = static_cast<std::uint64_t>(next_u32()) * static_cast<std::uint32_t>(span);
  return min_inclusive + static_cast<int>(r >> 32);
}

float Rng::range(float min_v, float max_v) {
  return min_v + (max_v - min_v) * next_float();
}

} // namespace mdaz
