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

private:
  std::uint32_t state_;
};

} // namespace mdaz
