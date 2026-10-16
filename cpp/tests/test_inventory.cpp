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

  {
    mdaz::ItemDatabase med;
    const char* mjson =
      "{\"version\":\"1.4.1\",\"items\":["
      "{\"id\":19,\"name\":\"Blood bag\"},"
      "{\"id\":28,\"name\":\"Saline bag\"},"
      "{\"id\":45,\"name\":\"Morphine\"},"
      "{\"id\":29,\"name\":\"Vitamins\"}"
      "]}";
    if (!med.load_from_json_string(mjson)) {
      std::printf("FAIL medical db\n");
      ++fail;
    } else {
      mdaz::Inventory minv;
      mdaz::SurvivalState ms;
      ms.blood = 40.f;
      minv.add_item(19, 1, med);
      if (!minv.use_consumable(0, ms, med) || ms.blood < 80.f) {
        std::printf("FAIL blood bag heal (blood=%.1f)\n", ms.blood);
        ++fail;
      } else {
        std::printf("OK   blood bag heal\n");
      }
      ms.blood = 50.f;
      minv.add_item(28, 1, med);
      int sal = -1;
      for (int i = 0; i < minv.slot_count(); ++i) if (minv.slot(i).id == 28) sal = i;
      if (sal < 0 || !minv.use_consumable(sal, ms, med) || ms.blood < 70.f) {
        std::printf("FAIL saline heal (blood=%.1f)\n", ms.blood);
        ++fail;
      } else {
        std::printf("OK   saline heal\n");
      }
      ms.bleeding = 2.f;
      minv.add_item(45, 1, med);
      int mor = -1;
      for (int i = 0; i < minv.slot_count(); ++i) if (minv.slot(i).id == 45) mor = i;
      if (mor < 0 || !minv.use_consumable(mor, ms, med) || ms.bleeding != 0.f) {
        std::printf("FAIL morphine\n");
        ++fail;
      } else {
        std::printf("OK   morphine stops bleed\n");
      }
    }
  }

  return fail ? 1 : 0;
}
