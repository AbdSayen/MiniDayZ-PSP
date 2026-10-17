#include "mdaz/craft.hpp"
#include "mdaz/item_db.hpp"
#include "mdaz/timers.hpp"

#include <cstdio>
#include <cstring>

int test_craft() {
  std::printf("--- test_craft / timers ---\n");
  int fail = 0;

  mdaz::WorldTimers t;
  if (t.zed_respawn != 60.f || t.loot_respawn != 1480.f || t.airdrop != 360.f) {
    std::printf("FAIL timer inits zed=%g loot=%g air=%g\n", t.zed_respawn, t.loot_respawn, t.airdrop);
    ++fail;
  } else {
    std::printf("OK   timer globals 60 / 1480 / 360\n");
  }

  mdaz::ItemDatabase db;
  db.load_from_json_string(
    "{\"items\":["
    "{\"id\":26,\"name\":\"Rags\"},{\"id\":18,\"name\":\"Bandage\"},"
    "{\"id\":38,\"name\":\"Wood sticks\"},{\"id\":51,\"name\":\"Crafted arrows\"},"
    "{\"id\":42,\"name\":\"Hunter knife\"},{\"id\":40,\"name\":\"Burlap sack\"},"
    "{\"id\":41,\"name\":\"Rope\"},{\"id\":454,\"name\":\"Improvised bag\"},"
    "{\"id\":37,\"name\":\"Ashwood stick\"},{\"id\":163,\"name\":\"Improvised bow\"}"
    "]}");

  mdaz::Inventory inv;
  inv.add_item(26, 1, db);
  const mdaz::CraftRecipe* r = mdaz::craft_recipe_at(0);
  if (!r || !mdaz::craft_can(inv, *r)) {
    std::printf("FAIL craft_can rags\n");
    ++fail;
  } else if (!mdaz::craft_do(inv, *r, db)) {
    std::printf("FAIL craft_do\n");
    ++fail;
  } else if (inv.slot(0).id != 18 && !(inv.slot(0).id == 18 || inv.slot(1).id == 18)) {
    // bandage should appear somewhere
    bool found = false;
    for (int i = 0; i < inv.slot_count(); ++i) if (inv.slot(i).id == 18) found = true;
    if (!found) {
      std::printf("FAIL no bandage result\n");
      ++fail;
    } else std::printf("OK   craft rags->bandage\n");
  } else {
    std::printf("OK   craft rags->bandage\n");
  }

  {
    mdaz::Inventory inv2;
    inv2.add_item(38, 5, db);
    inv2.add_item(42, 1, db);
    const mdaz::CraftRecipe* arrows = 0;
    for (int i = 0; i < mdaz::craft_recipe_count(); ++i) {
      const mdaz::CraftRecipe* c = mdaz::craft_recipe_at(i);
      if (c && c->result_id == 51 && c->result_count == 5) { arrows = c; break; }
    }
    if (!arrows || !mdaz::craft_do(inv2, *arrows, db)) {
      std::printf("FAIL craft sticks+knife->5 arrows\n");
      ++fail;
    } else {
      int n = 0;
      bool knife_kept = false;
      for (int i = 0; i < inv2.slot_count(); ++i) {
        if (inv2.slot(i).id == 51) n += inv2.slot(i).count;
        if (inv2.slot(i).id == 42) knife_kept = true;
      }
      if (n != 5 || !knife_kept) {
        std::printf("FAIL arrows=%d knife_kept=%d\n", n, (int)knife_kept);
        ++fail;
      } else std::printf("OK   craft sticks+knife->5 arrows\n");
    }
  }

  {
    mdaz::Inventory inv3;
    inv3.add_item(40, 1, db);
    inv3.add_item(41, 1, db);
    const mdaz::CraftRecipe* bag = 0;
    for (int i = 0; i < mdaz::craft_recipe_count(); ++i) {
      const mdaz::CraftRecipe* c = mdaz::craft_recipe_at(i);
      if (c && c->result_id == 454 && std::strcmp(c->name, "Craft_1") == 0) { bag = c; break; }
    }
    if (!bag || !mdaz::craft_do(inv3, *bag, db)) {
      std::printf("FAIL craft burlap+rope->bag\n");
      ++fail;
    } else std::printf("OK   craft improvised bag\n");
  }

  {
    mdaz::Inventory inv4;
    inv4.add_item(37, 1, db);
    inv4.add_item(41, 1, db);
    const mdaz::CraftRecipe* bow = 0;
    for (int i = 0; i < mdaz::craft_recipe_count(); ++i) {
      const mdaz::CraftRecipe* c = mdaz::craft_recipe_at(i);
      if (c && c->result_id == 163 && std::strcmp(c->name, "Craft_5") == 0) { bow = c; break; }
    }
    if (!bow || !mdaz::craft_do(inv4, *bow, db)) {
      std::printf("FAIL craft Craft_5 bow\n");
      ++fail;
    } else std::printf("OK   craft Craft_5 bow\n");
  }

  {
    mdaz::Inventory inv5;
    inv5.add_item(26, 4, db);
    const mdaz::CraftRecipe* rope = 0;
    for (int i = 0; i < mdaz::craft_recipe_count(); ++i) {
      const mdaz::CraftRecipe* c = mdaz::craft_recipe_at(i);
      if (c && c->result_id == 41 && c->ing_a == 26) { rope = c; break; }
    }
    if (!rope || !mdaz::craft_do(inv5, *rope, db)) {
      std::printf("FAIL craft 4 rags->rope\n");
      ++fail;
    } else std::printf("OK   craft rags->rope\n");
  }

  {
    mdaz::ItemDatabase db2;
    db2.load_from_json_string(
      "{\"items\":[{\"id\":38,\"name\":\"Wood sticks\"},{\"id\":37,\"name\":\"Ashwood stick\"},"
      "{\"id\":42,\"name\":\"Hunter knife\"}]}");
    mdaz::Inventory inv6;
    inv6.add_item(38, 3, db2);
    inv6.add_item(42, 1, db2);
    const mdaz::CraftRecipe* ash = 0;
    for (int i = 0; i < mdaz::craft_recipe_count(); ++i) {
      const mdaz::CraftRecipe* c = mdaz::craft_recipe_at(i);
      if (c && c->result_id == 37 && c->ing_a == 38) { ash = c; break; }
    }
    if (!ash || !mdaz::craft_do(inv6, *ash, db2)) {
      std::printf("FAIL craft 3 sticks+knife->ashwood\n");
      ++fail;
    } else std::printf("OK   craft sticks->ashwood\n");
  }

  {
    mdaz::ItemDatabase db3;
    db3.load_from_json_string(
      "{\"items\":[{\"id\":49,\"name\":\"Papers\"},{\"id\":38,\"name\":\"Wood sticks\"},"
      "{\"id\":24,\"name\":\"Matches\"}]}");
    mdaz::Inventory inv7;
    inv7.add_item(49, 1, db3);
    inv7.add_item(38, 2, db3);
    const mdaz::CraftRecipe* mat = 0;
    for (int i = 0; i < mdaz::craft_recipe_count(); ++i) {
      const mdaz::CraftRecipe* c = mdaz::craft_recipe_at(i);
      if (c && c->result_id == 24 && c->ing_a == 49) { mat = c; break; }
    }
    if (!mat || !mdaz::craft_do(inv7, *mat, db3)) {
      std::printf("FAIL craft papers+sticks->matches\n");
      ++fail;
    } else std::printf("OK   craft papers->matches\n");
  }

  // C2 Craft_fishing_rod: ashwood + rope → basic fishing rod
  {
    mdaz::ItemDatabase db4;
    db4.load_from_json_string(
      "{\"items\":[{\"id\":37,\"name\":\"Ashwood stick\"},{\"id\":41,\"name\":\"Rope\"},"
      "{\"id\":87,\"name\":\"Basic Fishing rod\"}]}");
    mdaz::Inventory inv8;
    inv8.add_item(37, 1, db4);
    inv8.add_item(41, 1, db4);
    const mdaz::CraftRecipe* rod = 0;
    for (int i = 0; i < mdaz::craft_recipe_count(); ++i) {
      const mdaz::CraftRecipe* c = mdaz::craft_recipe_at(i);
      if (c && c->result_id == 87 && c->ing_a == 37 && c->ing_b == 41) { rod = c; break; }
    }
    if (!rod || !mdaz::craft_do(inv8, *rod, db4)) {
      std::printf("FAIL craft ashwood+rope->fishing rod\n");
      ++fail;
    } else std::printf("OK   craft ashwood+rope->rod\n");
  }

  // 2 rags → bandana
  {
    mdaz::ItemDatabase dbB;
    dbB.load_from_json_string(
      "{\"items\":[{\"id\":26,\"name\":\"Rags\"},{\"id\":266,\"name\":\"Bandana\"}]}");
    mdaz::Inventory invB;
    invB.add_item(26, 2, dbB);
    const mdaz::CraftRecipe* ban = 0;
    for (int i = 0; i < mdaz::craft_recipe_count(); ++i) {
      const mdaz::CraftRecipe* c = mdaz::craft_recipe_at(i);
      if (c && c->result_id == 266 && c->ing_a == 26) { ban = c; break; }
    }
    if (!ban || !mdaz::craft_do(invB, *ban, dbB)) {
      std::printf("FAIL craft rags->bandana\n");
      ++fail;
    } else std::printf("OK   craft rags->bandana\n");
  }

  // Craft_4: improvised bag + 3 sticks → pack
  {
    mdaz::ItemDatabase dbPack;
    dbPack.load_from_json_string(
      "{\"items\":[{\"id\":454,\"name\":\"Improvised bag\"},{\"id\":38,\"name\":\"Wood sticks\"},"
      "{\"id\":455,\"name\":\"Improvised pack\"}]}");
    mdaz::Inventory invPack;
    invPack.add_item(454, 1, dbPack);
    invPack.add_item(38, 3, dbPack);
    const mdaz::CraftRecipe* pack = 0;
    for (int i = 0; i < mdaz::craft_recipe_count(); ++i) {
      const mdaz::CraftRecipe* c = mdaz::craft_recipe_at(i);
      if (c && std::strcmp(c->name, "Craft_4") == 0) { pack = c; break; }
    }
    if (!pack || !mdaz::craft_do(invPack, *pack, dbPack)) {
      std::printf("FAIL craft Craft_4 pack\n");
      ++fail;
    } else std::printf("OK   craft Craft_4 pack\n");
  }

  // Salmon → big fillet cook
  {
    mdaz::ItemDatabase db5;
    db5.load_from_json_string(
      "{\"items\":[{\"id\":92,\"name\":\"Salmon\"},{\"id\":96,\"name\":\"Big fish fillet\"}]}");
    mdaz::Inventory inv9;
    inv9.add_item(92, 1, db5);
    const mdaz::CraftRecipe* cook = 0;
    for (int i = 0; i < mdaz::craft_recipe_count(); ++i) {
      const mdaz::CraftRecipe* c = mdaz::craft_recipe_at(i);
      if (c && c->result_id == 96 && c->ing_a == 92 && c->requires_fire) { cook = c; break; }
    }
    if (!cook || !mdaz::craft_do(inv9, *cook, db5)) {
      std::printf("FAIL cook salmon->fillet\n");
      ++fail;
    } else std::printf("OK   cook salmon->big fillet\n");
  }

  // 2 rags + matches → heatpack
  {
    mdaz::ItemDatabase dbH;
    dbH.load_from_json_string(
      "{\"items\":[{\"id\":26,\"name\":\"Rags\"},{\"id\":24,\"name\":\"Matches\"},"
      "{\"id\":34,\"name\":\"Heatpack\"}]}");
    mdaz::Inventory invH;
    invH.add_item(26, 2, dbH);
    invH.add_item(24, 1, dbH);
    const mdaz::CraftRecipe* hp = 0;
    for (int i = 0; i < mdaz::craft_recipe_count(); ++i) {
      const mdaz::CraftRecipe* c = mdaz::craft_recipe_at(i);
      if (c && c->result_id == 34 && c->ing_a == 26) { hp = c; break; }
    }
    if (!hp || !mdaz::craft_do(invH, *hp, dbH)) {
      std::printf("FAIL craft rags+matches->heatpack\n");
      ++fail;
    } else std::printf("OK   craft heatpack\n");
  }

  // 4 sticks + rope → baseball bat
  {
    mdaz::ItemDatabase dbBat;
    dbBat.load_from_json_string(
      "{\"items\":[{\"id\":38,\"name\":\"Wood sticks\"},{\"id\":41,\"name\":\"Rope\"},"
      "{\"id\":203,\"name\":\"Baseball bat\"}]}");
    mdaz::Inventory invBat;
    invBat.add_item(38, 4, dbBat);
    invBat.add_item(41, 1, dbBat);
    const mdaz::CraftRecipe* bat = 0;
    for (int i = 0; i < mdaz::craft_recipe_count(); ++i) {
      const mdaz::CraftRecipe* c = mdaz::craft_recipe_at(i);
      if (c && c->result_id == 203) { bat = c; break; }
    }
    if (!bat || !mdaz::craft_do(invBat, *bat, dbBat)) {
      std::printf("FAIL craft sticks+rope->bat\n");
      ++fail;
    } else std::printf("OK   craft baseball bat\n");
  }

  // Craft_21: sticks+papers+knife → firekit
  {
    mdaz::ItemDatabase db21;
    db21.load_from_json_string(
      "{\"items\":[{\"id\":38,\"name\":\"Wood sticks\"},{\"id\":49,\"name\":\"Papers\"},"
      "{\"id\":42,\"name\":\"Hunter knife\"},{\"id\":23,\"name\":\"Campfire kit\"}]}");
    mdaz::Inventory inv21;
    inv21.add_item(38, 2, db21);
    inv21.add_item(49, 1, db21);
    inv21.add_item(42, 1, db21);
    const mdaz::CraftRecipe* r21 = 0;
    for (int i = 0; i < mdaz::craft_recipe_count(); ++i) {
      const mdaz::CraftRecipe* c = mdaz::craft_recipe_at(i);
      if (c && c->name && std::strstr(c->name, "Craft_21")) { r21 = c; break; }
    }
    if (!r21 || !mdaz::craft_do(inv21, *r21, db21)) {
      std::printf("FAIL craft_21 firekit\n");
      ++fail;
    } else {
      bool knife_kept = false, kit = false;
      for (int i = 0; i < inv21.slot_count(); ++i) {
        if (inv21.slot(i).id == 42) knife_kept = true;
        if (inv21.slot(i).id == 23) kit = true;
      }
      if (!knife_kept || !kit) {
        std::printf("FAIL craft_21 knife/kit\n");
        ++fail;
      } else std::printf("OK   craft_21 firekit\n");
    }
  }

  return fail ? 1 : 0;
}
