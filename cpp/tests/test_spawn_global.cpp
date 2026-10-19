#include "mdaz/bandit.hpp"
#include "mdaz/bunker.hpp"
#include "mdaz/bullet.hpp"
#include "mdaz/loot.hpp"
#include "mdaz/mapgen.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/spawn_global.hpp"
#include "mdaz/survival.hpp"
#include "mdaz/zombie.hpp"

#include <cstdio>

int test_spawn_global() {
  std::printf("--- test_spawn_global / bunker_erase / bots ---\n");
  int fail = 0;

  mdaz::MapGenerator map;
  map.generate_new_game(42);
  mdaz::LootSystem loot;
  mdaz::Rng rng(99);
  mdaz::GlobalRespawnParams gr = {&map, &loot, &rng, mdaz::Vec2(500.f, 500.f)};
  const int piles_before = loot.count();
  mdaz::global_respawn(gr);
  if (loot.count() <= piles_before) {
    std::printf("FAIL global_respawn added piles\n");
    ++fail;
  } else {
    std::printf("OK   global_respawn piles=%d\n", loot.count());
  }

  mdaz::SurvivalState surv;
  surv.blood = 0.f;
  mdaz::Vec2 pos(400.f, 400.f);
  mdaz::casual_respawn_player(surv, map, loot, rng, pos);
  if (surv.blood != 100.f || surv.food != 50.f || surv.water != 50.f) {
    std::printf("FAIL casual vitals b=%.0f f=%.0f w=%.0f\n", surv.blood, surv.food, surv.water);
    ++fail;
  } else {
    std::printf("OK   Casual_Respawn_player vitals\n");
  }

  mdaz::BunkerMap bmap;
  const char* sample = "{\"c2tilemap\":true,\"width\":8,\"height\":8,\"data\":\"64x0\"}";
  if (!bmap.load_from_c2_json(sample)) {
    std::printf("FAIL bunker sample load\n");
    return ++fail;
  }
  mdaz::ZombieSystem zeds;
  mdaz::BulletSystem bullets;
  zeds.spawn(mdaz::Zed_Normal, mdaz::Vec2(10.f, 10.f));
  mdaz::BunkerEraseResult er =
      mdaz::bunker_erase(true, bmap, zeds, loot, bullets, true);
  if (er.reopen_timer_sec != 1800.f || zeds.alive_count() != 0) {
    std::printf("FAIL bunker_erase reopen=%g zeds=%d\n", er.reopen_timer_sec, zeds.alive_count());
    ++fail;
  } else {
    std::printf("OK   Bunker_erase 1800s seal\n");
  }

  mdaz::BanditSystem bots;
  mdaz::Bandit* gun = bots.spawn(mdaz::Vec2(0, 0), true);
  mdaz::Bandit* punch = bots.spawn(mdaz::Vec2(1, 0), false);
  mdaz::Bandit* ally = bots.spawn(mdaz::Vec2(2, 0), false);
  ally->is_ally = true;
  mdaz::remove_bot_firearm(gun);
  mdaz::remove_bot_puncher(punch);
  if (gun->alive || punch->alive) {
    std::printf("FAIL remove_bot_firearm/puncher\n");
    ++fail;
  }
  mdaz::remove_team_bots(bots);
  if (ally->alive) {
    std::printf("FAIL remove_team_bots\n");
    ++fail;
  } else {
    std::printf("OK   Remove_bot_* helpers\n");
  }

  mdaz::Bandit* friend2 =
      mdaz::spawn_friend_bot(bots, mdaz::Vec2(0, 0), mdaz::Vec2(50.f, 0.f), rng);
  if (!friend2 || !friend2->is_ally) {
    std::printf("FAIL Spawn_friend_bot\n");
    ++fail;
  }

  return fail;
}
