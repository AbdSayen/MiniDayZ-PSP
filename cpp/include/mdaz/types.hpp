#pragma once

#include <cstdint>
#include <cstddef>

namespace mdaz {

typedef std::int32_t ItemId;
typedef std::uint32_t EntityId;

struct InvSlot {
  ItemId id;
  int count;
  InvSlot() : id(0), count(0) {}
  // C2 arrays: a slot is free iff its id is 0 (Contains(0)); count 0 is a valid item state
  // (empty bottle / canteen: item_activate return "zero").
  bool empty() const { return id == 0; }
};

enum AppState {
  AppState_Loading = 0,
  AppState_Menu,
  AppState_Tutorial,
  AppState_Playing,
  AppState_Dead,
  AppState_Paused
};

enum Difficulty {
  Difficulty_Novice = 0,
  Difficulty_Regular,
  Difficulty_Veteran
};

enum ZedType {
  Zed_Normal = 0,
  Zed_Buried,
  Zed_Army,
  Zed_Fast,
  Zed_ArmyShooter,
  Zed_Screamer,
  Zed_Tank,
  ZedType_Count
};

struct Vec2 {
  float x;
  float y;

  Vec2() : x(0.f), y(0.f) {}
  Vec2(float x_, float y_) : x(x_), y(y_) {}

  Vec2 operator+(const Vec2& o) const { return Vec2(x + o.x, y + o.y); }
  Vec2 operator-(const Vec2& o) const { return Vec2(x - o.x, y - o.y); }
  Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
};

struct Aabb {
  float x, y, w, h;
  bool contains(float px, float py) const {
    return px >= x && py >= y && px < x + w && py < y + h;
  }
  bool intersects(const Aabb& o) const {
    return x < o.x + o.w && x + w > o.x && y < o.y + o.h && y + h > o.y;
  }
};

// Original Construct export is 1024x768; PSP is 480x272.
static const int kDesignWidth = 1024;
static const int kDesignHeight = 768;
static const int kPspWidth = 480;
static const int kPspHeight = 272;

// From globals Map_size_X/Y init = 16
static const int kMapSectorsX = 16;
static const int kMapSectorsY = 16;

// Layout Map size from data.js
static const float kMapLayoutW = 17850.f;
static const float kMapLayoutH = 20000.f;

} // namespace mdaz
