#include "mdaz/craft.hpp"
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

  if (inv.capacity() != 1) {
    std::printf("FAIL base capacity %d\n", inv.capacity());
    ++fail;
  } else {
    std::printf("OK   base capacity 1 (C2 t381)\n");
  }
  {
    const mdaz::ItemId w[2] = {350, 300};
    inv.restore_worn(w, 2);
    if (inv.capacity() != 3) { std::printf("FAIL C2 start cap %d\n", inv.capacity()); ++fail; }
    else std::printf("OK   C2 start (tshirt+jeans) = 3 slots\n");
    const mdaz::ItemId tw_[4] = {303, 355, 403, 452};
    inv.restore_worn(tw_, 4);
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
    { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; kinv.restore_worn(tw_, 4); } // test: 16 C2 slots
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
      { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; minv.restore_worn(tw_, 4); } // test: 16 C2 slots
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
    if (binv.capacity() != 4 || binv.slot(0).id != 0) {
      std::printf("FAIL improvised bag cap %d\n", binv.capacity());
      ++fail;
    } else {
      std::printf("OK   improvised bag worn: 1+3 slots, left bag\n");
    }
    binv.add_item(450, 1, bagdb);
    binv.wear_clothing(0, bagdb);
    if (binv.capacity() != 4 || binv.slot(0).id != 454 || binv.worn_count() != 1) {
      std::printf("FAIL backpack swap\n"); ++fail;
    } else std::printf("OK   taloon swaps with improvised bag (3 slots)\n");
    const mdaz::ItemId w2[4] = {300, 355, 402, 452};
    binv.restore_worn(w2, 4);
    if (binv.capacity() != 1 + 2 + 3 + 2 + 7) { std::printf("FAIL full cap %d\n", binv.capacity()); ++fail; }
    else std::printf("OK   jeans+gorka+assault+mountain = 15 slots\n");
  }

  {
    // C2 Check_space_*: new slot, no merge; Pick_player_item gear swap + overflow drop.
    mdaz::Inventory g;
    const mdaz::ItemId w0[2] = {350, 300};
    g.restore_worn(w0, 2);                       // 3 slots
    g.add_item_new_slot(12, 10);
    g.add_item_new_slot(12, 10);
    if (g.slot(0).count != 10 || g.slot(1).id != 12) { std::printf("FAIL new-slot pickup merged\n"); ++fail; }
    else std::printf("OK   pickup takes a new slot (no stack merge)\n");
    mdaz::ItemId old = 0;
    g.equip_gear(452, &old);                     // mountain backpack -> 10 slots
    for (int k = 0; k < 6; ++k) g.add_item_new_slot(1, 1);
    if (old != 0 || g.capacity() != 10) { std::printf("FAIL equip backpack cap %d\n", g.capacity()); ++fail; }
    g.equip_gear(453, &old);                     // school backpack -> 5 slots
    mdaz::InvSlot over[24];
    const int n = g.take_overflow(over, 24);
    if (old != 452 || g.capacity() != 5 || n != 3) { std::printf("FAIL gear swap old=%d cap=%d over=%d\n", (int)old, g.capacity(), n); ++fail; }
    else std::printf("OK   backpack swap returns old piece, 3 items overflow\n");
  }

  {
    // Item 9: C2 Combine_ammo table + semantics (sid4428966087350586).
    mdaz::ItemDatabase sdb;
    sdb.load_from_json_string("{\"items\":[{\"id\":12,\"name\":\"A\"},{\"id\":38,\"name\":\"S\"},"
                              "{\"id\":1,\"name\":\"Canned beans\"},{\"id\":31,\"name\":\"B\"}]}");
    const mdaz::ItemDef* d12 = sdb.find(12); const mdaz::ItemDef* d1 = sdb.find(1);
    bool ok = d12 && d12->stack_max == 40 && d1 && d1->stack_max == 1 &&
              mdaz::c2_combine_stack(58) == 50 && mdaz::c2_combine_stack(38) == 5 &&
              mdaz::c2_combine_stack(31) == 3 && !mdaz::c2_combine_menu(31) && mdaz::c2_combine_menu(101);
    mdaz::Inventory ci;
    const mdaz::ItemId cw[2] = {350, 300};
    ci.restore_worn(cw, 2);
    ci.add_item(12, 30, sdb); ci.add_item(12, 25, sdb); ci.add_item(1, 1, sdb);
    ok = ok && ci.slot(0).count == 30 && ci.slot(1).count == 25;      // pickup: no merge
    int r = ci.combine_slots(1, 0, sdb);                               // 30+25=55 -> 40, src 15
    ok = ok && r == 15 && ci.slot(0).count == 40 && ci.slot(1).id == 12 && ci.slot(1).count == 15;
    r = ci.combine_slots(1, 0, sdb);                                   // target full -> return 15
    ok = ok && r == 15 && ci.slot(1).count == 15 && ci.slot(0).count == 40;
    r = ci.combine_slots(0, 1, sdb);                                   // 15+40=55 -> 40, src 15
    ok = ok && r == 15 && ci.slot(1).count == 40 && ci.slot(0).count == 15;
    ci.remove_at(1, 30);                                               // slot1 = 10
    r = ci.combine_slots(0, 1, sdb);                                   // 10+15=25 -> src cleared
    ok = ok && r == -15 && ci.slot(1).count == 25 && ci.slot(0).empty();
    ok = ok && ci.combine_slots(1, 2, sdb) == -1;                      // different ids
    if (!ok) { std::printf("FAIL item9 Combine_ammo\n"); ++fail; }
    else std::printf("OK   item9 Combine_ammo stacks/overflow/no-merge pickup\n");
  }

  {
    // build_fence: VAR required_count = 5 on ONE stick slot (not a sum over slots).
    mdaz::Inventory fi;
    const mdaz::ItemId fw[2] = {350, 300};
    fi.restore_worn(fw, 2);
    fi.add_item_new_slot(38, 3); fi.add_item_new_slot(38, 2); fi.add_item_new_slot(50, 1);
    bool ok = !mdaz::craft_build_fence1_can(fi) && mdaz::fence_stick_slot(fi) == -1;
    fi.remove_at(1, 2); fi.slot_count();
    fi.remove_at(0, 3); fi.add_item_new_slot(38, 5);
    ok = ok && mdaz::fence_stick_slot(fi) == 0 && mdaz::craft_build_fence2_can(fi);
    if (!ok) { std::printf("FAIL fence one-stack-of-5\n"); ++fail; }
    else std::printf("OK   fence needs one stick stack >= 5 (3+2 refused)\n");
  }

  {
    mdaz::ItemDatabase casedb;
    casedb.load_from_json_string(
      "{\"items\":[{\"id\":88,\"name\":\"Protector Case\"}]}");
    mdaz::Inventory cinv;
    { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; cinv.restore_worn(tw_, 4); } // test: 16 C2 slots
    const int base = cinv.capacity();
    cinv.add_item(88, 1, casedb);
    if (cinv.capacity() != base) {
      std::printf("FAIL protector case cap %d want %d\n", cinv.capacity(), base);
      ++fail;
    } else {
      std::printf("OK   protector case adds no slots (invented bonus removed)\n");
    }
  }

  {
    mdaz::ItemDatabase filldb;
    filldb.load_from_json_string(
      "{\"items\":[{\"id\":1,\"name\":\"Beans\",\"stack_max\":1}]}");
    mdaz::Inventory finv;
    { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; finv.restore_worn(tw_, 4); } // test: 16 C2 slots
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
    { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; ainv.restore_worn(tw_, 4); } // test: 16 C2 slots
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
    { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; cinv.restore_worn(tw_, 4); } // test: 16 C2 slots
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
    { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; cascade.restore_worn(tw_, 4); } // test: 16 C2 slots
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
    { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; pinv.restore_worn(tw_, 4); } // test: 16 C2 slots
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
    { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; inv.restore_worn(tw_, 4); } // test: 16 C2 slots
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
    { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; rinv.restore_worn(tw_, 4); } // test: 16 C2 slots
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
