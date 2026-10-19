#include "mdaz/craft.hpp"
#include "mdaz/item_db.hpp"
#include "mdaz/timers.hpp"

#include <cstdio>
#include <cstring>

int test_craft() {
  std::printf("--- test_craft / timers ---\n");
  if (mdaz::craft_recipe_count() < 1) {
    std::printf("FAIL craft_recipe_count\n");
    return 1;
  }
  int fail = 0;

  mdaz::WorldTimers t;
  if (t.zed_respawn != 60.f || t.loot_respawn != 1480.f || t.airdrop != 360.f) {
    std::printf("FAIL timer inits zed=%g loot=%g air=%g\n", t.zed_respawn, t.loot_respawn, t.airdrop);
    ++fail;
  } else {
    std::printf("OK   timer globals 60 / 1480 / 360\n");
  }

  // Only C2 Craft_* inventory Spawn_drop names present in data.js:
  // Craft_1, Craft_4, Craft_5, Craft_fishing_rod (+ fence Craft_31/32 world place).
  mdaz::ItemDatabase db;
  db.load_from_json_string(
    "{\"items\":["
    "{\"id\":40,\"name\":\"Burlap sack\"},{\"id\":41,\"name\":\"Rope\"},"
    "{\"id\":454,\"name\":\"Improvised bag\"},{\"id\":37,\"name\":\"Ashwood stick\"},"
    "{\"id\":163,\"name\":\"Improvised bow\"},{\"id\":38,\"name\":\"Wood sticks\"},"
    "{\"id\":455,\"name\":\"Improvised pack\"},{\"id\":87,\"name\":\"Basic Fishing rod\"}"
    "]}");

  {
    mdaz::Inventory inv3;
    { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; inv3.restore_worn(tw_, 4); } // test: 16 C2 slots
    inv3.add_item(40, 1, db);
    inv3.add_item(41, 1, db);
    const mdaz::CraftRecipe* bag = 0;
    for (int i = 0; i < mdaz::craft_recipe_count(); ++i) {
      const mdaz::CraftRecipe* c = mdaz::craft_recipe_at(i);
      if (c && std::strcmp(c->name, "Craft_1") == 0) { bag = c; break; }
    }
    if (!bag || !mdaz::craft_do(inv3, *bag, db)) {
      std::printf("FAIL craft Craft_1 bag\n");
      ++fail;
    } else {
      std::printf("OK   craft Craft_1 improvised bag\n");
    }
  }

  {
    mdaz::Inventory inv4;
    { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; inv4.restore_worn(tw_, 4); } // test: 16 C2 slots
    inv4.add_item(37, 1, db);
    inv4.add_item(41, 1, db);
    const mdaz::CraftRecipe* bow = 0;
    for (int i = 0; i < mdaz::craft_recipe_count(); ++i) {
      const mdaz::CraftRecipe* c = mdaz::craft_recipe_at(i);
      if (c && std::strcmp(c->name, "Craft_5") == 0) { bow = c; break; }
    }
    if (!bow || !mdaz::craft_do(inv4, *bow, db)) {
      std::printf("FAIL craft Craft_5 bow\n");
      ++fail;
    } else {
      std::printf("OK   craft Craft_5 bow\n");
    }
  }

  {
    mdaz::Inventory inv8;
    { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; inv8.restore_worn(tw_, 4); } // test: 16 C2 slots
    inv8.add_item(37, 1, db);
    inv8.add_item(41, 1, db);
    const mdaz::CraftRecipe* rod = 0;
    for (int i = 0; i < mdaz::craft_recipe_count(); ++i) {
      const mdaz::CraftRecipe* c = mdaz::craft_recipe_at(i);
      if (c && std::strcmp(c->name, "Craft_fishing_rod") == 0) { rod = c; break; }
    }
    if (!rod || !mdaz::craft_do(inv8, *rod, db)) {
      std::printf("FAIL craft Craft_fishing_rod\n");
      ++fail;
    } else {
      std::printf("OK   craft Craft_fishing_rod\n");
    }
  }

  {
    mdaz::Inventory invPack;
    { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; invPack.restore_worn(tw_, 4); } // test: 16 C2 slots
    invPack.add_item(454, 1, db);
    invPack.add_item(38, 3, db);
    const mdaz::CraftRecipe* pack = 0;
    for (int i = 0; i < mdaz::craft_recipe_count(); ++i) {
      const mdaz::CraftRecipe* c = mdaz::craft_recipe_at(i);
      if (c && std::strcmp(c->name, "Craft_4") == 0) { pack = c; break; }
    }
    if (!pack || !mdaz::craft_do(invPack, *pack, db)) {
      std::printf("FAIL craft Craft_4 pack\n");
      ++fail;
    } else {
      std::printf("OK   craft Craft_4 pack\n");
    }
  }

  {
    mdaz::Inventory invCk;
    { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; invCk.restore_worn(tw_, 4); } // test: 16 C2 slots
    if (mdaz::craft_any_possible(invCk, false)) {
      std::printf("FAIL craft_any_possible empty inv\n");
      ++fail;
    }
    invCk.add_item(40, 1, db);
    invCk.add_item(41, 1, db);
    if (!mdaz::craft_any_possible(invCk, false)) {
      std::printf("FAIL craft_any_possible with Craft_1 ingredients\n");
      ++fail;
    } else {
      std::printf("OK   craft_any_possible (Check_craft)\n");
    }
  }

  // Invented inventory Craft_* names must stay absent
  for (int i = 0; i < mdaz::craft_recipe_count(); ++i) {
    const mdaz::CraftRecipe* c = mdaz::craft_recipe_at(i);
    if (!c || !c->name) continue;
    if (std::strncmp(c->name, "Cook_", 5) == 0 ||
        std::strcmp(c->name, "Craft_rags_bandage") == 0 ||
        std::strcmp(c->name, "Craft_sticks_arrow") == 0 ||
        std::strcmp(c->name, "Craft_molotov") == 0) {
      std::printf("FAIL invented recipe still present: %s\n", c->name);
      ++fail;
    }
  }
  if (fail == 0) std::printf("OK   no invented Cook_*/Craft_rags_* inventory recipes\n");

  {
    mdaz::ItemDatabase fdb;
    fdb.load_from_json_string(
      "{\"items\":[{\"id\":38,\"name\":\"Wood sticks\"},{\"id\":50,\"name\":\"Woodpile\"}]}");
    mdaz::Inventory f;
    { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; f.restore_worn(tw_, 4); } // test: 16 C2 slots
    f.add_item(38, 5, fdb);
    if (!mdaz::craft_build_fence1_can(f)) {
      std::printf("FAIL craft_build_fence1_can\n");
      ++fail;
    } else {
      std::printf("OK   Craft_32 fence1 can (5 sticks)\n");
    }
    f.add_item(50, 1, fdb);
    if (!mdaz::craft_build_fence2_can(f)) {
      std::printf("FAIL craft_build_fence2_can\n");
      ++fail;
    } else {
      std::printf("OK   Craft_31 fence2 can (sticks+pile)\n");
    }
  }

  return fail ? 1 : 0;
}
