#include "mdaz/vehicle.hpp"

#include <cmath>

namespace mdaz {

Vehicle::Vehicle() { reset(); }

void Vehicle::reset() {
  type = Vehicle_None;
  pos = Vec2();
  facing_deg = 0.f;
  fuel = 100.f;
  hp = 100.f;
  active = false;
}

VehicleSystem::VehicleSystem()
  : count_(0), driving_index_(-1), driving_(false) {}

void VehicleSystem::clear() {
  count_ = 0;
  driving_index_ = -1;
  driving_ = false;
}

Vehicle* VehicleSystem::spawn(VehicleType t, Vec2 pos) {
  if (count_ >= kMax) return 0;
  Vehicle& v = vehicles_[count_++];
  v.reset();
  v.type = t;
  v.pos = pos;
  v.active = true;
  v.fuel = (t == Vehicle_Bike) ? 0.f : 80.f;
  return &v;
}

bool VehicleSystem::try_repair(Vec2 player_pos, float amount) {
  Vehicle* v = driven();
  if (!v) {
    for (int i = 0; i < count_; ++i) {
      if (!vehicles_[i].active) continue;
      const float dx = vehicles_[i].pos.x - player_pos.x;
      const float dy = vehicles_[i].pos.y - player_pos.y;
      if (dx * dx + dy * dy < 50.f * 50.f) { v = &vehicles_[i]; break; }
    }
  }
  if (!v || v->hp >= 100.f) return false;
  v->hp += amount;
  if (v->hp > 100.f) v->hp = 100.f;
  return true;
}

Vehicle* VehicleSystem::driven() {
  if (!driving_ || driving_index_ < 0 || driving_index_ >= count_) return 0;
  return &vehicles_[driving_index_];
}

const Vehicle* VehicleSystem::driven() const {
  if (!driving_ || driving_index_ < 0 || driving_index_ >= count_) return 0;
  return &vehicles_[driving_index_];
}

bool VehicleSystem::update(float dt, Vec2& player_pos, float move_x, float move_y,
                           bool interact_pressed) {
  // Mount/dismount
  if (interact_pressed && !driving_) {
    for (int i = 0; i < count_; ++i) {
      if (!vehicles_[i].active) continue;
      const float dx = vehicles_[i].pos.x - player_pos.x;
      const float dy = vehicles_[i].pos.y - player_pos.y;
      if (dx * dx + dy * dy < 40.f * 40.f) {
        driving_ = true;
        driving_index_ = i;
        break;
      }
    }
  } else if (interact_pressed && driving_) {
    driving_ = false;
    driving_index_ = -1;
  }

  if (!driving_ || driving_index_ < 0) return false;
  Vehicle& v = vehicles_[driving_index_];
  if (!v.active || v.hp <= 0.f) {
    driving_ = false;
    return false;
  }

  float len = move_x * move_x + move_y * move_y;
  if (len > 1.e-6f) {
    const float inv = 1.f / std::sqrt(len);
    move_x *= inv;
    move_y *= inv;
    v.facing_deg = std::atan2(move_y, move_x) * 57.2957795f;
  }

  float spd = 160.f;
  if (v.type == Vehicle_Bike) spd = 140.f;
  if (v.type == Vehicle_Boat) spd = 130.f;
  if (v.type == Vehicle_Car) {
    if (v.fuel > 0.f) {
      v.fuel -= 2.f * dt;
      if (v.fuel < 0.f) v.fuel = 0.f;
    } else {
      spd = 0.f;
    }
  } else if (v.type == Vehicle_Boat && v.fuel > 0.f) {
    v.fuel -= 0.8f * dt;
    if (v.fuel < 0.f) v.fuel = 0.f;
  } else if (v.type == Vehicle_Boat && v.fuel <= 0.f) {
    spd = 35.f; // paddle stub
  }

  v.pos.x += move_x * spd * dt;
  v.pos.y += move_y * spd * dt;
  player_pos = v.pos;
  return true;
}

} // namespace mdaz
