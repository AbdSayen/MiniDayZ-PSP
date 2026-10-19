#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

// C2 Vehicle family IVs (object type t946 / family "Vehicle"): 284=fuel, 294=condition (hp bar).
namespace vehicle_balance {
constexpr float kFuelMax = 100.f;       // Spawn_uaz / Car_respawn set IV to 100
constexpr float kHpMax = 100.f;
constexpr float kRefuelAdd = 40.f;      // Using inventory: action 172,159 +40, cap 100
// IV284 fuel drain interval/amount: not present as explicit events in data.js export
// (Drivable vehicles group references IV284 for bar_fuel only). Stub until decode exists.
constexpr float kFuelTickSec = 0.1f;
constexpr float kFuelSubtractPerTick = 8.f;
constexpr float kRespawnDelaySec = 600.f;   // Timer → Uaz_respawn / Volga_respawn
constexpr float kRespawnCallDelaySec = 1.f; // Uaz_respawn → Car_respawn_uaz
constexpr float kCrashBuildingHp = 15.f;    // Vehicle collisions [[10,1],[7,[0,15]]]
constexpr float kCrashHardHp = 30.f;        // car_crash_* [[10,1],[7,[0,30]]]
constexpr float kLowFuelWarn = 20.f;        // GUI nofuel branch (bar threshold; approximate)
// Drivable vehicles: player_base t184 AltMove + Pin to car (t946). Custom AltMove/Vehicle
// behavior JS is not in the HTML5 export — max speed not decodable from data.js.
constexpr float kDriveSpeedApprox = 160.f;  // layout-units/s stub (was bare 160 in vehicle.cpp)
// C2 Vehicle Layering / Vehicles UI group Z keys (Buildings car vs player).
constexpr int kC2VehicleLayerZ = 197;
constexpr int kC2PlayerLayerZ = 914;
constexpr int kC2GuiVehicleLayerZ = 928;

inline float vehicle_y_sort_key(Vec2 pos) { return pos.y; }

} // namespace vehicle_balance

enum VehicleType {
  Vehicle_None = 0,
  Vehicle_Car = 1, // drivable UAZ (car_uaz_base); legacy enum name
  Vehicle_Boat = 2,
  Vehicle_Bike = 3,
  Vehicle_Volga = 4,
  Vehicle_Truck = 5,
};

inline bool vehicle_is_drivable_car(VehicleType t) {
  return t == Vehicle_Car || t == Vehicle_Volga || t == Vehicle_Truck;
}

inline bool vehicle_has_world_respawn(VehicleType t) {
  return t == Vehicle_Car || t == Vehicle_Volga;
}

struct Vehicle {
  VehicleType type;
  Vec2 pos;
  Vec2 respawn_anchor;
  float facing_deg;
  float fuel;
  float hp;
  bool active;
  float respawn_t; // seconds until Car_respawn_* (0 = not waiting)
  // Trunk / basket stash (car/boat/bike)
  static const int kStashSlots = 4;
  ItemId stash_ids[kStashSlots];
  int stash_counts[kStashSlots];

  Vehicle();
  void reset();
  bool stash_add(ItemId id, int count);
  bool stash_take(ItemId& out_id, int& out_count); // first non-empty
  int stash_used() const;
};

class VehicleSystem {
public:
  VehicleSystem();
  void clear();
  Vehicle* spawn(VehicleType t, Vec2 pos);
  // C2 map reload spawns (Spawn_uaz / Spawn_volga / Spawn_truck dispatch).
  void spawn_uaz(Vec2 pos);
  void spawn_volga(Vec2 pos);
  void spawn_truck(Vec2 pos);
  void tick_respawn(float dt);
  void on_destroyed(Vehicle& v);
  void apply_crash_damage(Vehicle& v, float hp_loss);
  // Repair driven or nearest vehicle; returns true if repaired.
  bool try_repair(Vec2 player_pos, float amount);
  // Returns true if player is currently driving (movement applied to vehicle).
  bool update(float dt, Vec2& player_pos, float move_x, float move_y, bool interact_pressed,
              bool has_paddle = true);
  bool player_driving() const { return driving_; }
  Vehicle* driven();
  const Vehicle* driven() const;
  int count() const { return count_; }
  Vehicle* at(int i) { return &vehicles_[i]; }
  const Vehicle* at(int i) const { return &vehicles_[i]; }

private:
  void respawn_vehicle(Vehicle& v);
  static const int kMax = 16;
  Vehicle vehicles_[kMax];
  int count_;
  int driving_index_;
  bool driving_;
};

} // namespace mdaz
