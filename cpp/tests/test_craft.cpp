#include "mdaz/craft.hpp"
#include "mdaz/item_db.hpp"
#include "mdaz/timers.hpp"

#include <cstdio>

int test_craft() {
  std::printf("--- test_craft / timers ---\n");
  int fail = 0;

  mdaz::WorldTimers t;
  if (t.zed_respawn != 60.f || t.loot_respawn != 1480.f || t.airdrop != 400.f) {
    std::printf("FAIL timer inits zed=%g loot=%g air=%g\n", t.zed_respawn, t.loot_respawn, t.airdrop);
    ++fail;
  } else {
    std::printf("OK   timer globals 60 / 1480 / 400\n");
  }

  mdaz::ItemDatabase db;
  db.load_from_json_string(
    "{\"items\":["
    "{\"id\":26,\"name\":\"Rags\"},{\"id\":18,\"name\":\"Bandage\"},"
    "{\"id\":38,\"name\":\"Wood sticks\"},{\"id\":51,\"name\":\"Crafted arrows\"}"
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

  return fail ? 1 : 0;
}
