#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

struct Campfire {
  Vec2 pos;
  float life;   // seconds remaining
  bool active;
  bool molotov_burn; // C2 molotov_flame — Molotov_damage tick, not warmth fuel
};

static const int kMaxCampfires = 16;

class CampfireSystem {
public:
  CampfireSystem();
  void clear();
  Campfire* place(Vec2 pos, float life = 180.f);
  // Thrown molotov puddle (≈5s fade in C2 Molotov params).
  Campfire* place_molotov(Vec2 pos, float life = 5.f);
  // Add life to nearest active fire within radius; returns true if boosted.
  bool boost_near(Vec2 pos, float add_life, float radius = 60.f);
  // rain01 0..1 — heavy rain extinguishes unprotected fires faster
  void update(float dt, float rain01 = 0.f);
  // Warmth bonus 0..1 if any fire within radius
  float warmth_near(Vec2 pos, float radius = 55.f) const;
  bool near_fire(Vec2 pos, float radius = 55.f) const;
  bool in_molotov_burn(Vec2 pos, float radius = 48.f) const;
  int count() const { return count_; }
  const Campfire* at(int i) const { return &fires_[i]; }

private:
  Campfire fires_[kMaxCampfires];
  int count_;
};

} // namespace mdaz
