#include "mdaz/ammo.hpp"
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
  if (inv.capacity() != 12) {
    std::printf("FAIL base capacity %d\n", inv.capacity());
    ++fail;
  } else {
    std::printf("OK   base bag capacity 12\n");
  }
  if (!inv.add_item(1, 1, db) || inv.slot(0).id != 1) {
    std::printf("FAIL add_item\n");
    ++fail;
  } else {
    std::printf("OK   add_item\n");
  }

  mdaz::SurvivalState surv;
  surv.food = 10.f;
  // Canned food needs a knife
  if (inv.use_consumable(0, surv, db)) {
    std::printf("FAIL canned without knife should fail\n");
    ++fail;
  } else {
    std::printf("OK   canned needs knife\n");
  }
  {
    mdaz::ItemDatabase knifedb;
    knifedb.load_from_json_string(
      "{\"items\":[{\"id\":1,\"name\":\"Canned beans\"},{\"id\":42,\"name\":\"Hunter knife\"}]}");
    mdaz::Inventory kinv;
    kinv.add_item(1, 1, knifedb);
    kinv.add_item(42, 1, knifedb);
    mdaz::SurvivalState ks;
    ks.food = 10.f;
    if (!kinv.use_consumable(0, ks, knifedb) || !(ks.food > 10.f)) {
      std::printf("FAIL canned with knife\n");
      ++fail;
    } else {
      std::printf("OK   use food restores hunger\n");
    }
  }
  // Clear canned for bandage tests — re-add nothing on inv slot0 may still have beans
  inv.remove_at(0, 1);

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

  {
    mdaz::ItemDatabase bagdb;
    bagdb.load_from_json_string(
      "{\"items\":[{\"id\":454,\"name\":\"Improvised bag\"},{\"id\":450,\"name\":\"Taloon backpack\"}]}");
    mdaz::Inventory binv;
    binv.add_item(454, 1, bagdb);
    binv.wear_clothing(0, bagdb);
    if (binv.capacity() != 15) {
      std::printf("FAIL improvised bag cap %d\n", binv.capacity());
      ++fail;
    } else {
      std::printf("OK   improvised bag capacity 15\n");
    }
  }

  {
    mdaz::ItemDatabase casedb;
    casedb.load_from_json_string(
      "{\"items\":[{\"id\":88,\"name\":\"Protector Case\"}]}");
    mdaz::Inventory cinv;
    const int base = cinv.capacity();
    cinv.add_item(88, 1, casedb);
    if (cinv.capacity() != base + 3) {
      std::printf("FAIL protector case cap %d want %d\n", cinv.capacity(), base + 3);
      ++fail;
    } else {
      std::printf("OK   protector case +3 capacity\n");
    }
  }

  {
    mdaz::ItemDatabase filldb;
    filldb.load_from_json_string(
      "{\"items\":[{\"id\":1,\"name\":\"Beans\",\"stack_max\":1}]}");
    mdaz::Inventory finv;
    for (int i = 0; i < finv.capacity(); ++i) {
      if (!finv.add_item(1, 1, filldb)) break;
    }
    if (finv.can_add_item(1, 1, filldb)) {
      std::printf("FAIL can_add when bag full\n");
      ++fail;
    } else if (finv.add_item(1, 1, filldb)) {
      std::printf("FAIL add_item overflow when full\n");
      ++fail;
    } else {
      std::printf("OK   Check_space_inventory full bag\n");
    }
  }

  {
    mdaz::ItemDatabase adb;
    adb.load_from_json_string("{\"items\":[{\"id\":12,\"name\":\".45 ACP\"}]}");
    mdaz::Inventory ainv;
    ainv.add_item(12, 7, adb);
    ainv.add_item(12, 5, adb);
    if (mdaz::count_ammo_in_bag(ainv, 12) != 12) {
      std::printf("FAIL ammo count\n");
      ++fail;
    } else if (mdaz::consume_ammo_from_bag(ainv, 12, 4) != 4 ||
               mdaz::count_ammo_in_bag(ainv, 12) != 8) {
      std::printf("FAIL ammo consume\n");
      ++fail;
    } else {
      std::printf("OK   ammo count/consume\n");
    }
  }

  return fail ? 1 : 0;
}
