#pragma once

#include "mdaz/balance_globals.hpp"

namespace mdaz {

namespace {
// C2 Spawn_parameters group name (Timer Check_loot → Global_respawn lives under this group).
const char kC2_LOOT_Respawn_group[] = "LOOT Respawn";

inline float airdrop_period() {
  // Airdrop_Timer_ALL = 360 is the in-game cycle; Airdrop_recharge = 400 init fallback
  return static_cast<float>(balance::Airdrop_Timer_ALL > 0
                                ? balance::Airdrop_Timer_ALL
                                : balance::Airdrop_recharge);
}
} // namespace

// Respawn / recharge timers from Game_events globals (init values verified).
struct WorldTimers {
  float zed_respawn;   // ZED_RESPAWN_TIME = 60
  float bot_respawn;   // BOT_RESPAWN_TIME = 10
  float loot_respawn;  // LOOT_RESPAWN_TIME = 1480
  float airdrop;       // Airdrop_Timer_ALL = 360 (preferred)
  float animal_respawn; // scaffold — between bot and loot cadence

  WorldTimers() { reset(); }

  void reset() {
    (void)kC2_LOOT_Respawn_group;
    zed_respawn = static_cast<float>(balance::ZED_RESPAWN_TIME);
    bot_respawn = static_cast<float>(balance::BOT_RESPAWN_TIME);
    loot_respawn = static_cast<float>(balance::LOOT_RESPAWN_TIME);
    airdrop = airdrop_period();
    animal_respawn = 90.f;
  }

  void update(float dt) {
    if (zed_respawn > 0.f) zed_respawn -= dt;
    if (bot_respawn > 0.f) bot_respawn -= dt;
    if (loot_respawn > 0.f) loot_respawn -= dt;
    if (airdrop > 0.f) airdrop -= dt;
    if (animal_respawn > 0.f) animal_respawn -= dt;
  }

  bool consume_zed_respawn() {
    if (zed_respawn > 0.f) return false;
    zed_respawn = static_cast<float>(balance::ZED_RESPAWN_TIME);
    return true;
  }
  bool consume_loot_respawn() {
    if (loot_respawn > 0.f) return false;
    loot_respawn = static_cast<float>(balance::LOOT_RESPAWN_TIME);
    return true;
  }
  bool consume_airdrop() {
    if (airdrop > 0.f) return false;
    airdrop = airdrop_period();
    return true;
  }
  bool consume_bot_respawn() {
    if (bot_respawn > 0.f) return false;
    bot_respawn = static_cast<float>(balance::BOT_RESPAWN_TIME);
    return true;
  }
  bool consume_animal_respawn() {
    if (animal_respawn > 0.f) return false;
    animal_respawn = 90.f;
    return true;
  }
};

} // namespace mdaz
