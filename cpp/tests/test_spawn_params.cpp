#include "mdaz/bandit.hpp"
#include "mdaz/inventory.hpp"
#include "mdaz/inventory_helpers.hpp"
#include "mdaz/item_db.hpp"
#include "mdaz/loot.hpp"
#include "mdaz/player_drop.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/spawn_params.hpp"
#include "mdaz/zombie.hpp"

#include <cstdio>

int test_spawn_params() {
  std::printf("--- test_spawn_params / allies / inventory helpers ---\n");
  int fail = 0;

  mdaz::BanditSystem bots;
  mdaz::Rng rng(1);
  mdaz::Vec2 player(100.f, 100.f);
  mdaz::Vec2 marker(150.f, 120.f);
  mdaz::Bandit* ally = mdaz::spawn_friend_bot(bots, player, marker, rng);
  if (!ally || !ally->is_ally || ally->has_gun) {
    std::printf("FAIL spawn_friend_bot\n");
    ++fail;
  } else {
    std::printf("OK   Spawn_friend_bot ally\n");
  }

  mdaz::TeamCampSystem camps;
  mdaz::bot_team_check_situation(camps, 3, player, 9);
  if (camps.active_count() != 1) {
    std::printf("FAIL Spawn_team_camp count=%d\n", camps.active_count());
    ++fail;
  } else {
    std::printf("OK   Spawn_team_camp\n");
  }
  camps.clear_team_camps();
  if (camps.active_count() != 0) {
    std::printf("FAIL Clear_team_camps\n");
    ++fail;
  }

  mdaz::ZombieSystem zeds;
  mdaz::ZedRespawnChannels ch;
  for (int i = 0; i < 16; ++i) (void)mdaz::zed_resp_tick_spawn1(ch, zeds, player, rng);
  if (zeds.alive_count() == 0) {
    std::printf("FAIL spawn1 threshold\n");
    ++fail;
  } else {
    std::printf("OK   spawn1 Trigger_spawn zeds=%d\n", zeds.alive_count());
  }

  mdaz::LootSystem loot;
  mdaz::spawn3_secret_drops(loot, mdaz::Vec2(500.f, 500.f), rng);
  if (loot.count() < 3) {
    std::printf("FAIL spawn3 drops count=%d\n", loot.count());
    ++fail;
  } else {
    std::printf("OK   spawn3 secret Spawn_drop x%d\n", loot.count());
  }

  mdaz::inventory_set_item_for_remove_id(18);
  mdaz::Inventory inv;
  { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; inv.restore_worn(tw_, 4); } // test: 16 C2 slots
  mdaz::ItemDatabase db;
  inv.add_item(18, 1, db);
  inv.add_item(19, 1, db);
  if (!mdaz::inventory_remove_marked(inv, 0, 1)) {
    std::printf("FAIL item_for_remove_id\n");
    ++fail;
  } else if (!inv.slot(0).empty()) {
    std::printf("FAIL item_for_remove_id slot0\n");
    ++fail;
  } else if (inv.slot(1).id != 19) {
    std::printf("FAIL item_for_remove_id slot1\n");
    ++fail;
  } else {
    std::printf("OK   item_for_remove_id\n");
  }

  float rust = 0.f;
  mdaz::recheck_inventory_cards(3.f, 5.f, 2.f, 4.f, rust);
  if (rust != 5.f) {
    std::printf("FAIL Recheck_inventory_cards rust=%g\n", rust);
    ++fail;
  } else {
    std::printf("OK   Recheck_inventory_cards\n");
  }

  mdaz::LootSystem drop_loot;
  mdaz::td_drop_weapons(drop_loot, player, 151, 1);
  if (drop_loot.count() != 1) {
    std::printf("FAIL TD_drop_weapons\n");
    ++fail;
  } else {
    std::printf("OK   TD_drop_weapons ground drop\n");
  }

  return fail;
}
