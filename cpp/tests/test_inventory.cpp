#include "mdaz/ammo.hpp"
#include "mdaz/inventory.hpp"
#include "mdaz/inventory_ui.hpp"
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
      "{\"id\":29,\"name\":\"Vitamins\"},"
      "{\"id\":46,\"name\":\"Tetracycline\"}"
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
      minv.add_item(45, 1, med);
      int mor = -1;
      for (int i = 0; i < minv.slot_count(); ++i) if (minv.slot(i).id == 45) mor = i;
      if (mor < 0 || minv.use_consumable(mor, ms, med)) {
        std::printf("FAIL morphine (no C2 item_activate — use should fail)\n");
        ++fail;
      } else {
        std::printf("OK   morphine blocked without item_activate\n");
      }
      ms.infected = true;
      minv.add_item(46, 1, med);
      int ab = -1;
      for (int i = 0; i < minv.slot_count(); ++i) if (minv.slot(i).id == 46) ab = i;
      if (ab < 0 || !minv.use_consumable(ab, ms, med) || ms.infected) {
        std::printf("FAIL tetracycline\n");
        ++fail;
      } else {
        std::printf("OK   tetracycline clears infection\n");
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

  {
    mdaz::ItemDatabase cdb;
    cdb.load_from_json_string(
      "{\"items\":[{\"id\":12,\"name\":\".45 ACP ammo\",\"stack_max\":20},"
      "{\"id\":303,\"name\":\"Cargo pants\",\"category\":\"clothing\"}]}");
    mdaz::Inventory cinv;
    cinv.add_item(303, 1, cdb);
    cinv.wear_clothing(0, cdb);
    cinv.sync_clothing_gates(cdb);
    if (!cinv.clothing().region_available(mdaz::ClothingRegion_Pants)) {
      std::printf("FAIL clothing pants gate\n");
      ++fail;
    } else if (!cinv.clothing().add(mdaz::ClothingRegion_Pants, 12, 10, cdb)) {
      std::printf("FAIL clothing add ammo\n");
      ++fail;
    } else if (mdaz::consume_ammo_for_reload(cinv, 12, 3) != 3 ||
               cinv.clothing().count_ammo(12) != 7) {
      std::printf("FAIL clothing ammo consume\n");
      ++fail;
    } else {
      std::printf("OK   Check_ammo_pants / clothing reload\n");
    }
    mdaz::Inventory full;
    full.add_item(303, 1, cdb);
    full.wear_clothing(0, cdb);
    full.sync_clothing_gates(cdb);
    for (int i = 0; i < full.capacity(); ++i) full.add_item(1, 1, cdb);
    for (int i = 0; i < mdaz::kClothingContainerSlots; ++i)
      (void)full.clothing().add(mdaz::ClothingRegion_Pants, 12, 30, cdb);
    mdaz::ItemDatabase bagdb;
    bagdb.load_from_json_string("{\"items\":[{\"id\":1,\"name\":\"X\",\"stack_max\":1}]}");
    if (mdaz::pick_inventory_item(full, bagdb, 12, 1) != mdaz::PickInv_NoSpace) {
      std::printf("FAIL pick with no clothing space\n");
      ++fail;
    } else if (mdaz::pick_inventory_item(full, bagdb, 12, 1) != mdaz::PickInv_NoSpace) {
      std::printf("FAIL pick still no space\n");
      ++fail;
    } else {
      std::printf("OK   Check_space_pants full cascade\n");
    }
    mdaz::Inventory cascade;
    cascade.add_item(303, 1, cdb);
    cascade.wear_clothing(0, cdb);
    cascade.sync_clothing_gates(cdb);
    for (int i = 0; i < cascade.capacity(); ++i) cascade.add_item(1, 1, cdb);
    if (mdaz::pick_inventory_item(cascade, bagdb, 12, 3) != mdaz::PickInv_Ok ||
        cascade.clothing().count_ammo(12) != 3) {
      std::printf("FAIL pick into clothing cascade\n");
      ++fail;
    } else {
      std::printf("OK   Check_space_pants pickup cascade\n");
    }
  }

  {
    mdaz::ItemDatabase pdb;
    pdb.load_from_json_string("{\"items\":[{\"id\":4,\"name\":\"Test item\"}]}");
    mdaz::Inventory pinv;
    if (mdaz::pick_inventory_item(pinv, pdb, 4, 2) != mdaz::PickInv_Ok ||
        mdaz::count_item_in_inventory(pinv, 4) != 2) {
      std::printf("FAIL pick_inventory_item\n");
      ++fail;
    } else if (!mdaz::check_item_inventory(pinv, 4) || !mdaz::check_item_exists(pinv, 4)) {
      std::printf("FAIL check_item_inventory\n");
      ++fail;
    } else {
      std::printf("OK   pick_inventory_item / Check_item_inventory\n");
    }
    mdaz::InventoryUiState ui;
    ui.open();
    if (!ui.opened) {
      std::printf("FAIL inventory open\n");
      ++fail;
    }
    ui.close_inventory();
    if (ui.opened) {
      std::printf("FAIL close_inventory\n");
      ++fail;
    } else {
      std::printf("OK   close_inventory / Inventory_opened\n");
    }
  }

  {
    mdaz::ItemDatabase pdb;
    pdb.load_from_json_string(
      "{\"items\":[{\"id\":12,\"name\":\"Ammo\",\"stack_max\":30,\"category\":\"ammo\"},"
      "{\"id\":303,\"name\":\"Cargo pants\",\"category\":\"clothing\"}]}");
    mdaz::Inventory inv;
    inv.add_item(303, 1, pdb);
    inv.wear_clothing(0, pdb);
    inv.sync_clothing_gates(pdb);
    (void)inv.clothing().add(mdaz::ClothingRegion_Pants, 12, 5, pdb);
    if (mdaz::pick_player_item(inv, pdb, mdaz::ClothingRegion_Pants, 0) != mdaz::PickPlayer_Ok ||
        mdaz::count_item_in_inventory(inv, 12) != 5 ||
        !inv.clothing().slot(mdaz::ClothingRegion_Pants, 0).empty()) {
      std::printf("FAIL pick_player_item to bag\n");
      ++fail;
    } else {
      std::printf("OK   Pick_player_item clothing→bag\n");
    }
    if (mdaz::pick_player_item(inv, pdb, mdaz::ClothingRegion_Pants, 0) !=
        mdaz::PickPlayer_Empty) {
      std::printf("FAIL pick_player_item empty slot\n");
      ++fail;
    }
  }

  {
    // Cook-only / no item_activate IV restores must not invent hunger/thirst.
    mdaz::ItemDatabase rdb;
    rdb.load_from_json_string(
      "{\"items\":[{\"id\":60,\"name\":\"Raw steak\"},{\"id\":97,\"name\":\"Mushroom\"},"
      "{\"id\":999,\"name\":\"Mystery drink\"}]}");
    mdaz::Inventory rinv;
    mdaz::SurvivalState rs;
    rs.food = 40.f;
    rs.water = 40.f;
    rinv.add_item(60, 1, rdb);
    rinv.add_item(97, 1, rdb);
    rinv.add_item(999, 1, rdb);
    if (rinv.use_consumable(0, rs, rdb) || rinv.use_consumable(1, rs, rdb) ||
        rinv.use_consumable(2, rs, rdb) || rs.food != 40.f || rs.water != 40.f) {
      std::printf("FAIL no invented food/drink restores (food=%.1f water=%.1f)\n",
                  rs.food, rs.water);
      ++fail;
    } else {
      std::printf("OK   no invented food/drink restores without item_activate\n");
    }
  }

  return fail ? 1 : 0;
}
