#include "mdaz/menu_camera.hpp"

#include "mdaz/rng.hpp"

#include <cmath>

namespace mdaz {

MenuCamera::MenuCamera() : cam(), pin(), initialized(false) {}

void MenuCamera::reset(Rng& rng) {
  cam.x = kCenterX;
  cam.y = kCenterY;
  pin.x = kCenterX + rng.range(-400.f, 400.f);
  pin.y = kCenterY + rng.range(-250.f, 250.f);
  initialized = true;
}

void MenuCamera::update(float dt, Rng& rng) {
  if (!initialized) reset(rng);
  const float dx = pin.x - cam.x;
  const float dy = pin.y - cam.y;
  const float dist = std::sqrt(dx * dx + dy * dy);
  if (dist > kRecenterDistance) {
    // C2: when cam–pin distance > 100, camera_roll_menu picks a new Buildings pin.
    pin.x = kCenterX + rng.range(-650.f, 650.f);
    pin.y = kCenterY + rng.range(-420.f, 420.f);
  }
  if (dist > 1.f) {
    const float step = kMoveSpeed * dt;
    const float t = step >= dist ? 1.f : step / dist;
    cam.x += dx * t;
    cam.y += dy * t;
  }
}

} // namespace mdaz
