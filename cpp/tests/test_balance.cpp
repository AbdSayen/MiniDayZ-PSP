#include "mdaz/balance_globals.hpp"
#include "mdaz/combat.hpp"
#include "mdaz/types.hpp"

#include <cmath>
#include <cstdio>

static int expect_eq(const char* name, double got, double want) {
  if (std::fabs(got - want) > 1e-9) {
    std::printf("FAIL %s: got %g want %g\n", name, got, want);
    return 1;
  }
  std::printf("OK   %s = %g\n", name, got);
  return 0;
}

int test_balance() {
  std::printf("--- test_balance (sverka with extracted 1.4.1 globals) ---\n");
  int f = 0;
  mdaz::set_npc_damage_for_difficulty(mdaz::Difficulty_Regular);
  f += expect_eq("gameversion", mdaz::balance::gameversion, 140);
  f += expect_eq("Zed_dmg", mdaz::balance::Zed_dmg, 9);
  f += expect_eq("Zed_fast_dmg", mdaz::balance::Zed_fast_dmg, 6);
  f += expect_eq("Zed_army_dmg", mdaz::balance::Zed_army_dmg, 13);
  f += expect_eq("Wolf_dmg", mdaz::balance::Wolf_dmg, 10);
  f += expect_eq("Zed_per_base", mdaz::balance::Zed_per_base, 5);
  f += expect_eq("Bot_corpse_stay_time", mdaz::balance::Bot_corpse_stay_time, 90);
  f += expect_eq("BOT_RESPAWN_TIME", mdaz::balance::BOT_RESPAWN_TIME, 10);
  f += expect_eq("Player_Hear_radius", mdaz::balance::Player_Hear_radius, 1000);
  f += expect_eq("Airdrop_Timer_ALL", mdaz::balance::Airdrop_Timer_ALL, 360);
  f += expect_eq("Airdrop_recharge", mdaz::balance::Airdrop_recharge, 400);
  f += expect_eq("Novice_starving", mdaz::balance::Novice_starving, 0.75);
  f += expect_eq("Regular_starving", mdaz::balance::Regular_starving, 1.1);
  f += expect_eq("Veteran_starving", mdaz::balance::Veteran_starving, 1.4);
  f += expect_eq("Day_Night_speed", mdaz::balance::Day_Night_speed, 720);
  f += expect_eq("Rain_speed", mdaz::balance::Rain_speed, 360);
  f += expect_eq("ZED_RESPAWN_TIME", mdaz::balance::ZED_RESPAWN_TIME, 60);
  f += expect_eq("LOOT_RESPAWN_TIME", mdaz::balance::LOOT_RESPAWN_TIME, 1480);
  f += expect_eq("Map_size_X", mdaz::balance::Map_size_X, 16);
  f += expect_eq("dispersion_angle_max", mdaz::balance::dispersion_angle_max, 25);
  f += expect_eq("dispersion_pershot", mdaz::balance::dispersion_pershot, 5);
  f += expect_eq("zed_melee_damage(Normal)", mdaz::zed_melee_damage(mdaz::Zed_Normal), 9);
  f += expect_eq("zed_melee_damage(Fast)", mdaz::zed_melee_damage(mdaz::Zed_Fast), 6);
  return f ? 1 : 0;
}
