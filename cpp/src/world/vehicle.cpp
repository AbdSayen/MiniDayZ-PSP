#include "mdaz/vehicle.hpp"
#include "mdaz/types.hpp"

#include <cmath>

namespace mdaz {

namespace {
bool near_water_band(Vec2 p, float margin = 420.f) {
  return p.x < margin || p.y < margin ||
         p.x > kMapLayoutW - margin || p.y > kMapLayoutH - margin;
}
} // namespace

Vehicle::Vehicle() { reset(); }

void Vehicle::reset() {
  type = Vehicle_None;
  pos = Vec2();
  facing_deg = 0.f;
  fuel = 100.f;
  hp = 100.f;
  active = false;
  for (int i = 0; i < kStashSlots; ++i) {
    stash_ids[i] = 0;
    stash_counts[i] = 0;
  }
}

bool Vehicle::stash_add(ItemId id, int count) {
  if (id <= 0 || count <= 0) return false;
  for (int i = 0; i < kStashSlots; ++i) {
    if (stash_ids[i] == id && stash_counts[i] > 0) {
      stash_counts[i] += count;
      return true;
    }
  }
  for (int i = 0; i < kStashSlots; ++i) {
    if (stash_ids[i] == 0 || stash_counts[i] <= 0) {
      stash_ids[i] = id;
      stash_counts[i] = count;
      return true;
    }
  }
  return false;
}

bool Vehicle::stash_take(ItemId& out_id, int& out_count) {
  for (int i = 0; i < kStashSlots; ++i) {
    if (stash_ids[i] > 0 && stash_counts[i] > 0) {
      out_id = stash_ids[i];
      out_count = stash_counts[i];
      stash_ids[i] = 0;
      stash_counts[i] = 0;
      return true;
    }
  }
  return false;
}

int Vehicle::stash_used() const {
  int n = 0;
  for (int i = 0; i < kStashSlots; ++i)
    if (stash_ids[i] > 0 && stash_counts[i] > 0) ++n;
  return n;
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
                           bool interact_pressed, bool has_paddle) {
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
    // Paddle: wood sticks (or ashwood) in inventory = decent speed; else crawl
    spd = has_paddle ? 70.f : 22.f;
  }

  Vec2 next(v.pos.x + move_x * spd * dt, v.pos.y + move_y * spd * dt);
  if (v.type == Vehicle_Boat && !near_water_band(next)) {
    // Beached — crawl only along shore, don't leave water band
    next = v.pos;
    if (near_water_band(Vec2(v.pos.x + move_x * 40.f * dt, v.pos.y)))
      next.x = v.pos.x + move_x * 40.f * dt;
    if (near_water_band(Vec2(v.pos.x, v.pos.y + move_y * 40.f * dt)))
      next.y = v.pos.y + move_y * 40.f * dt;
  }
  v.pos = next;
  player_pos = v.pos;
  return true;
}

} // namespace mdaz
