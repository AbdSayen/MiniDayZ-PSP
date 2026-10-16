#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

// Vehicle scaffold — C2 has car/boat groups; full physics not ported.
enum VehicleType {
  Vehicle_None = 0,
  Vehicle_Car,
  Vehicle_Boat,
  Vehicle_Bike
};

struct Vehicle {
  VehicleType type;
  Vec2 pos;
  float facing_deg;
  float fuel;
  float hp;
  bool active;

  Vehicle();
  void reset();
};

class VehicleSystem {
public:
  VehicleSystem();
  void clear();
  Vehicle* spawn(VehicleType t, Vec2 pos);
  // Repair driven or nearest vehicle; returns true if repaired.
  bool try_repair(Vec2 player_pos, float amount);
  // Returns true if player is currently driving (movement applied to vehicle).
  bool update(float dt, Vec2& player_pos, float move_x, float move_y, bool interact_pressed);
  bool player_driving() const { return driving_; }
  Vehicle* driven();
  const Vehicle* driven() const;
  int count() const { return count_; }
  Vehicle* at(int i) { return &vehicles_[i]; }
  const Vehicle* at(int i) const { return &vehicles_[i]; }

private:
  static const int kMax = 16;
  Vehicle vehicles_[kMax];
  int count_;
  int driving_index_;
  bool driving_;
};

} // namespace mdaz
