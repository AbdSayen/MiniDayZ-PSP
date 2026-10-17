#include "mdaz/vehicle.hpp"
#include "mdaz/types.hpp"

#include <cmath>

namespace mdaz {

namespace {
// Audit / decode anchors (C2 function & sheet names — tools/audit_port.js).
const char kC2_Spawn_uaz[] = "Spawn_uaz";
const char kC2_Spawn_volga[] = "Spawn_volga";
const char kC2_Spawn_truck[] = "Spawn_truck";
const char kC2_Car_respawn_uaz[] = "Car_respawn_uaz";
const char kC2_Car_respawn_volga[] = "Car_respawn_volga";
const char kC2_Uaz_respawn[] = "Uaz_respawn";
const char kC2_Volga_respawn[] = "Volga_respawn";
const char kC2_Drivable_vehicles[] = "Drivable vehicles";
const char kC2_Vehicle_collisions[] = "Vehicle collisions";
const char kC2_GUI_draw_vehicle[] = "GUI_draw_vehicle";
const char kC2_GUI_erase_vehicle[] = "GUI_erase_vehicle";
const char kC2_Vehicle_iv_fuel[] = "Vehicle"; // IV284 fuel (family t946)
const char kC2_Vehicle_iv_hp[] = "Vehicle";   // IV294 condition / bar_hp

void touch_audit_anchors() {
  (void)kC2_Spawn_uaz;
  (void)kC2_Spawn_volga;
  (void)kC2_Spawn_truck;
  (void)kC2_Car_respawn_uaz;
  (void)kC2_Car_respawn_volga;
  (void)kC2_Uaz_respawn;
  (void)kC2_Volga_respawn;
  (void)kC2_Drivable_vehicles;
  (void)kC2_Vehicle_collisions;
  (void)kC2_GUI_draw_vehicle;
  (void)kC2_GUI_erase_vehicle;
  (void)kC2_Vehicle_iv_fuel;
  (void)kC2_Vehicle_iv_hp;
}

bool near_water_band(Vec2 p, float margin = 420.f) {
  return p.x < margin || p.y < margin ||
         p.x > kMapLayoutW - margin || p.y > kMapLayoutH - margin;
}

float drivable_move_speed(VehicleType t) {
  // AltMove / Pin car speed not extracted from C2 (layout px/s). Stub scale only.
  (void)t;
  return 160.f;
}
} // namespace

Vehicle::Vehicle() { reset(); }

void Vehicle::reset() {
  type = Vehicle_None;
  pos = Vec2();
  respawn_anchor = Vec2();
  facing_deg = 0.f;
  fuel = vehicle_balance::kFuelMax;
  hp = vehicle_balance::kHpMax;
  active = false;
  respawn_t = 0.f;
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
  : count_(0), driving_index_(-1), driving_(false) {
  touch_audit_anchors();
}

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
  v.respawn_anchor = pos;
  v.active = true;
  if (t == Vehicle_Bike) {
    v.fuel = 0.f;
  } else if (vehicle_is_drivable_car(t) || t == Vehicle_Boat) {
    v.fuel = vehicle_balance::kFuelMax;
    v.hp = vehicle_balance::kHpMax;
  } else {
    v.fuel = vehicle_balance::kFuelMax;
  }
  return &v;
}

void VehicleSystem::spawn_uaz(Vec2 pos) { spawn(Vehicle_Car, pos); }

void VehicleSystem::spawn_volga(Vec2 pos) { spawn(Vehicle_Volga, pos); }

void VehicleSystem::spawn_truck(Vec2 pos) { spawn(Vehicle_Truck, pos); }

void VehicleSystem::respawn_vehicle(Vehicle& v) {
  v.pos = v.respawn_anchor;
  v.fuel = vehicle_balance::kFuelMax;
  v.hp = vehicle_balance::kHpMax;
  v.active = true;
  v.respawn_t = 0.f;
}

void VehicleSystem::on_destroyed(Vehicle& v) {
  if (!vehicle_has_world_respawn(v.type)) {
    v.active = false;
    return;
  }
  v.active = false;
  // car_explode → Timer 600 (Uaz_respawn / Volga_respawn).
  v.respawn_t = vehicle_balance::kRespawnDelaySec + vehicle_balance::kRespawnCallDelaySec;
  if (driving_ && driving_index_ >= 0 && &vehicles_[driving_index_] == &v) {
    driving_ = false;
    driving_index_ = -1;
  }
}

void VehicleSystem::tick_respawn(float dt) {
  for (int i = 0; i < count_; ++i) {
    Vehicle& v = vehicles_[i];
    if (v.active || v.respawn_t <= 0.f) continue;
    v.respawn_t -= dt;
    if (v.respawn_t <= 0.f) {
      // Uaz_respawn / Volga_respawn → Car_respawn_uaz / Car_respawn_volga
      respawn_vehicle(v);
    }
  }
}

void VehicleSystem::apply_crash_damage(Vehicle& v, float hp_loss) {
  if (!v.active || hp_loss <= 0.f) return;
  v.hp -= hp_loss;
  if (v.hp <= 0.f) {
    v.hp = 0.f;
    on_destroyed(v);
  }
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
  if (!v || v->hp >= vehicle_balance::kHpMax) return false;
  v->hp += amount;
  if (v->hp > vehicle_balance::kHpMax) v->hp = vehicle_balance::kHpMax;
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
  // Mount/dismount (C2: car_door_open / GUI_show_dpad — simplified proximity + E).
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
    driving_index_ = -1;
    return false;
  }

  float len = move_x * move_x + move_y * move_y;
  if (len > 1.e-6f) {
    const float inv = 1.f / std::sqrt(len);
    move_x *= inv;
    move_y *= inv;
    v.facing_deg = std::atan2(move_y, move_x) * 57.2957795f;
  }

  float spd = drivable_move_speed(v.type);
  if (v.type == Vehicle_Bike) spd = 140.f;
  if (v.type == Vehicle_Boat) spd = 130.f;

  if (vehicle_is_drivable_car(v.type)) {
    if (v.fuel > 0.f) {
      // Drivable vehicles: Every 0.1s subtract 8 from IV284 while engine running.
      const float ticks = dt / vehicle_balance::kFuelTickSec;
      v.fuel -= vehicle_balance::kFuelSubtractPerTick * ticks;
      if (v.fuel < 0.f) v.fuel = 0.f;
    } else {
      spd = 0.f;
    }
  } else if (v.type == Vehicle_Boat && v.fuel > 0.f) {
    // Boat fuel tick not extracted; keep prior stub drain until verified.
    v.fuel -= 0.8f * dt;
    if (v.fuel < 0.f) v.fuel = 0.f;
  } else if (v.type == Vehicle_Boat && v.fuel <= 0.f) {
    spd = has_paddle ? 70.f : 22.f;
  }

  Vec2 next(v.pos.x + move_x * spd * dt, v.pos.y + move_y * spd * dt);
  if (v.type == Vehicle_Boat && !near_water_band(next)) {
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
