#include "mdaz/inventory.hpp"
#include "mdaz/item_db.hpp"

#include <cstdio>

int test_inventory() {
  std::printf("--- test_inventory ---\n");
  int fail = 0;

  mdaz::ItemDatabase db;
  const char* json =
    "{\"version\":\"1.4.1\",\"items\":["
    "{\"id\":1,\"name\":\"Canned beans\"},"
    "{\"id\":8,\"name\":\"Pipsi\"},"
    "{\"id\":18,\"name\":\"Bandage\"}"
    "]}";
  if (!db.load_from_json_string(json) || db.count() != 3) {
    std::printf("FAIL item db load count=%u\n", (unsigned)db.count());
    return 1;
  }
  std::printf("OK   loaded %u items\n", (unsigned)db.count());

  if (!db.find(1) || db.find(1)->name != "Canned beans") {
    std::printf("FAIL find canned beans\n");
    ++fail;
  } else {
    std::printf("OK   item 1 name\n");
  }

  mdaz::Inventory inv;
  if (!inv.add_item(1, 1, db) || inv.slot(0).id != 1) {
    std::printf("FAIL add_item\n");
    ++fail;
  } else {
    std::printf("OK   add_item\n");
  }

  mdaz::SurvivalState surv;
  surv.food = 10.f;
  if (!inv.use_consumable(0, surv, db)) {
    std::printf("FAIL use food\n");
    ++fail;
  } else if (!(surv.food > 10.f)) {
    std::printf("FAIL food not restored\n");
    ++fail;
  } else {
    std::printf("OK   use food restores hunger\n");
  }

  inv.add_item(18, 1, db);
  surv.bleeding = 4.f;
  // find bandage slot
  int band = -1;
  for (int i = 0; i < inv.slot_count(); ++i) {
    if (inv.slot(i).id == 18) band = i;
  }
  if (band < 0 || !inv.use_consumable(band, surv, db) || surv.bleeding != 0.f) {
    std::printf("FAIL bandage\n");
    ++fail;
  } else {
    std::printf("OK   bandage stops bleeding\n");
  }

  return fail ? 1 : 0;
}
