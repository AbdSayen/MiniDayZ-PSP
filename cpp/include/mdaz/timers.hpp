#pragma once

#include "mdaz/balance_globals.hpp"

namespace mdaz {

// Respawn / recharge timers from Game_events globals (init values verified).
struct WorldTimers {
  float zed_respawn;   // ZED_RESPAWN_TIME = 60
  float bot_respawn;   // BOT_RESPAWN_TIME = 10
  float loot_respawn;  // LOOT_RESPAWN_TIME = 1480
  float airdrop;       // Airdrop_recharge = 400

  WorldTimers() { reset(); }

  void reset() {
    zed_respawn = static_cast<float>(balance::ZED_RESPAWN_TIME);
    bot_respawn = static_cast<float>(balance::BOT_RESPAWN_TIME);
    loot_respawn = static_cast<float>(balance::LOOT_RESPAWN_TIME);
    airdrop = static_cast<float>(balance::Airdrop_recharge);
  }

  void update(float dt) {
    if (zed_respawn > 0.f) zed_respawn -= dt;
    if (bot_respawn > 0.f) bot_respawn -= dt;
    if (loot_respawn > 0.f) loot_respawn -= dt;
    if (airdrop > 0.f) airdrop -= dt;
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
    airdrop = static_cast<float>(balance::Airdrop_recharge);
    return true;
  }
};

} // namespace mdaz
