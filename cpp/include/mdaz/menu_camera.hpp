#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

class Rng;

// C2 Menu_Events → camera_roll_menu (see cpp/docs_gen/functions/camera_roll_menu_impl.json).
struct MenuCamera {
  static constexpr float kLayoutW = 2500.f;
  static constexpr float kLayoutH = 1500.f;
  static constexpr float kCenterX = kLayoutW * 0.5f;
  static constexpr float kCenterY = kLayoutH * 0.5f;
  // Verified in camera_roll_menu_impl: distance compare uses 100 layout units.
  static constexpr float kRecenterDistance = 100.f;
  // Stopper behavior speed literal 3 in C2; scaled for layout units/sec (approx).
  static constexpr float kMoveSpeed = 45.f;

  Vec2 cam;
  Vec2 pin;
  bool initialized;

  MenuCamera();

  void reset(Rng& rng);
  void update(float dt, Rng& rng);
  Vec2 position() const { return cam; }
};

} // namespace mdaz
