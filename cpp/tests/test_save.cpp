#include "mdaz/save_game.hpp"
#include "mdaz/item_db.hpp"
#include "mdaz/rng.hpp"

#include <cstdio>
#include <cstring>

int test_save() {
  std::printf("--- test_save ---\n");
  int fail = 0;

  mdaz::Player p;
  p.reset_new_game(mdaz::Difficulty_Regular);
  p.pos.x = 1234.f;
  p.pos.y = 5678.f;
  p.survival.food = 44.f;
  p.survival.blood = 77.f;
  p.zeds_killed = 3;
  p.animals_killed = 7;
  p.perk_hunter = true;
  p.nvg_on = true;

  mdaz::ItemDatabase db;
  db.load_from_json_string(
      "{\"items\":[{\"id\":1,\"name\":\"Canned beans\"},{\"id\":251,\"name\":\"Army helmet\"}]}");
  mdaz::Inventory inv;
  inv.add_item(251, 1, db);
  float saved_iv6 = 100.f;
  const bool worn_ok = inv.wear_clothing(0, db);  // frees the single base slot
  inv.add_item(1, 2, db);
  if (worn_ok || inv.worn_count() > 0) {
    saved_iv6 = inv.worn_iv6(0);
    mdaz::Rng wr(7);
    for (int i = 0; i < 60 && inv.worn_iv6(0) >= saved_iv6 - 0.01f; ++i)
      inv.apply_hit_armor_wear(wr, true, false, false);
    saved_iv6 = inv.worn_iv6(0);
  }

  mdaz::WeatherState w;
  w.day_phase = 0.42f;

  bool visited[mdaz::kMapSectorsX * mdaz::kMapSectorsY];
  for (int i = 0; i < mdaz::kMapSectorsX * mdaz::kMapSectorsY; ++i) visited[i] = false;
  visited[0] = true;
  visited[17] = true;

  mdaz::VehicleSystem vs;
  mdaz::Vehicle* car = vs.spawn(mdaz::Vehicle_Car, mdaz::Vec2(100.f, 200.f));
  car->fuel = 42.f;
  car->hp = 88.f;
  car->stash_add(1, 5);

  mdaz::CropSaveView crops;
  crops.count = 1;
  crops.plots[0].x = 50.f;
  crops.plots[0].y = 60.f;
  crops.plots[0].crop_id = 31;
  crops.plots[0].grow_t = 12.f;
  crops.plots[0].ready = 0;

  mdaz::TrapSaveView traps;
  traps.count = 1;
  traps.traps[0].x = 11.f;
  traps.traps[0].y = 22.f;
  traps.traps[0].life = 80.f;
  traps.traps[0].kind = 0;
  traps.traps[0].sprung = 0;

  mdaz::WorldExtras extras;
  extras.airdrop_t = 120.f;
  extras.airdrop_x = 700.f;
  extras.airdrop_y = 800.f;
  extras.tent_warmth_t = 240.f;
  extras.adrenaline_t = 9.f;
  extras.bunker_looted = 1;
  extras.melee_break_mul = 0.5f;

  mdaz::FireSaveView fires;
  fires.count = 1;
  fires.fires[0].x = 9.f;
  fires.fires[0].y = 8.f;
  fires.fires[0].life = 99.f;

  mdaz::LootSaveView loot;
  loot.count = 1;
  loot.piles[0].x = 400.f;
  loot.piles[0].y = 500.f;
  loot.piles[0].item_count = 2;
  loot.piles[0].items[0] = 18;
  loot.piles[0].counts[0] = 2;
  loot.piles[0].items[1] = 1;
  loot.piles[0].counts[1] = 1;
  extras.loot_respawn_t = 901.f;

  mdaz::SaveBlob blob;
  if (!mdaz::SaveSystem::write_to_buffer(p, inv, w, 3, blob, visited,
                                         mdaz::kMapSectorsX * mdaz::kMapSectorsY,
                                         66.f, 2, 4, 11, 55.f, 72.f, 3, &vs, &crops, &traps,
                                         &extras, &fires, &loot)) {
    std::printf("FAIL write\n");
    return 1;
  }
  if (blob.version != 19) {
    std::printf("FAIL save version %d\n", blob.version);
    ++fail;
  }
  mdaz::Player p2;
  mdaz::Inventory inv2;
  { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; inv2.restore_worn(tw_, 4); } // test: 16 C2 slots
  mdaz::WeatherState w2;
  mdaz::VehicleSystem vs2;
  mdaz::CropSaveView crops2;
  mdaz::TrapSaveView traps2;
  mdaz::FireSaveView fires2;
  mdaz::LootSaveView loot2;
  mdaz::WorldExtras extras2;
  int eq = -1;
  float batt = 0.f;
  int qact = -1, qprog = -1;
  int mag = -1;
  float wet = -1.f;
  float gfilt = -1.f;
  int fish = -1;
  bool visited2[mdaz::kMapSectorsX * mdaz::kMapSectorsY];
  for (int i = 0; i < mdaz::kMapSectorsX * mdaz::kMapSectorsY; ++i) visited2[i] = false;
  if (!mdaz::SaveSystem::read_from_buffer(blob, p2, inv2, w2, &eq, visited2,
                                          mdaz::kMapSectorsX * mdaz::kMapSectorsY,
                                          &batt, &qact, &qprog, &mag, &wet,
                                          &gfilt, &fish, &vs2, &crops2, &traps2,
                                          &extras2, &fires2, &loot2)) {
    std::printf("FAIL read\n");
    return 1;
  }
  if (eq != 3) {
    std::printf("FAIL equipped_slot\n");
    ++fail;
  }

  if (p2.pos.x != 1234.f || p2.survival.food != 44.f || p2.zeds_killed != 3) {
    std::printf("FAIL player fields\n");
    ++fail;
  } else std::printf("OK   player fields roundtrip\n");

  int total = 0;
  for (int i = 0; i < inv2.slot_count(); ++i) {
    if (inv2.slot(i).id == 1) total += inv2.slot(i).count;
  }
  if (total != 2) {
    std::printf("FAIL inventory roundtrip total=%d\n", total);
    ++fail;
  } else std::printf("OK   inventory roundtrip\n");

  if (w2.day_phase != 0.42f) {
    std::printf("FAIL weather\n");
    ++fail;
  } else std::printf("OK   weather roundtrip\n");

  if (!visited2[0] || !visited2[17] || visited2[1]) {
    std::printf("FAIL visited fog bits\n");
    ++fail;
  } else std::printf("OK   visited fog roundtrip\n");

  if (batt != 66.f || qact != 2 || qprog != 4 || !p2.perk_hunter || !p2.nvg_on ||
      p2.animals_killed != 7) {
    std::printf("FAIL v4 fields batt=%.1f q=%d/%d hunt=%d nvg=%d animals=%d\n",
                batt, qact, qprog, (int)p2.perk_hunter, (int)p2.nvg_on, p2.animals_killed);
    ++fail;
  } else std::printf("OK   v4 battery/quest/perks\n");

  if (mag != 11 || wet != 55.f) {
    std::printf("FAIL v5 mag/wet mag=%d wet=%.1f\n", mag, wet);
    ++fail;
  } else std::printf("OK   v5 mag/wetness\n");

  if (gfilt != 72.f || fish != 3 || p2.fish_caught != 3) {
    std::printf("FAIL v6 filter/fish g=%.1f fish=%d pfish=%d\n", gfilt, fish, p2.fish_caught);
    ++fail;
  } else std::printf("OK   v6 filter/fish\n");

  if (vs2.count() != 1 || !vs2.at(0)->active || vs2.at(0)->type != mdaz::Vehicle_Car ||
      vs2.at(0)->fuel != 42.f || vs2.at(0)->hp != 88.f ||
      vs2.at(0)->stash_ids[0] != 1 || vs2.at(0)->stash_counts[0] != 5) {
    std::printf("FAIL v7 vehicle stash count=%d fuel=%.1f stash=%d/%d\n",
                vs2.count(), vs2.count() ? vs2.at(0)->fuel : -1.f,
                vs2.count() ? vs2.at(0)->stash_ids[0] : -1,
                vs2.count() ? vs2.at(0)->stash_counts[0] : -1);
    ++fail;
  } else std::printf("OK   v7 vehicle+stash\n");

  if (crops2.count != 1 || crops2.plots[0].crop_id != 31 || crops2.plots[0].grow_t != 12.f ||
      crops2.plots[0].x != 50.f || crops2.plots[0].ready != 0) {
    std::printf("FAIL v8 crops count=%d id=%d grow=%.1f\n",
                crops2.count, crops2.count ? crops2.plots[0].crop_id : -1,
                crops2.count ? crops2.plots[0].grow_t : -1.f);
    ++fail;
  } else std::printf("OK   v8 crop plots\n");

  if (traps2.count != 1 || traps2.traps[0].kind != 0 || traps2.traps[0].life != 80.f ||
      traps2.traps[0].x != 11.f) {
    std::printf("FAIL v9 traps count=%d kind=%d life=%.1f\n",
                traps2.count, traps2.count ? traps2.traps[0].kind : -1,
                traps2.count ? traps2.traps[0].life : -1.f);
    ++fail;
  } else std::printf("OK   v9 placed traps\n");

  if (extras2.airdrop_t != 120.f ||
      extras2.airdrop_x != 700.f || extras2.airdrop_y != 800.f) {
    std::printf("FAIL v10 airdrop air=%.1f pos=%.0f,%.0f\n",
                extras2.airdrop_t, extras2.airdrop_x, extras2.airdrop_y);
    ++fail;
  } else std::printf("OK   v10 airdrop\n");

  if (fires2.count != 1 || fires2.fires[0].life != 99.f || fires2.fires[0].x != 9.f ||
      extras2.tent_warmth_t != 240.f) {
    std::printf("FAIL v11 fires/tent count=%d life=%.1f tent=%.1f\n",
                fires2.count, fires2.count ? fires2.fires[0].life : -1.f,
                extras2.tent_warmth_t);
    ++fail;
  } else std::printf("OK   v11 campfires+tent\n");

  if (extras2.adrenaline_t != 9.f) {
    std::printf("FAIL v12 adrenaline %.1f\n", extras2.adrenaline_t);
    ++fail;
  } else std::printf("OK   v12 adrenaline\n");

  if (extras2.bunker_looted != 1 || extras2.melee_break_mul != 0.5f) {
    std::printf("FAIL v13 bunker/melee loot=%d mul=%.2f\n",
                extras2.bunker_looted, extras2.melee_break_mul);
    ++fail;
  } else std::printf("OK   v13 bunker+melee mul\n");

  if (loot2.count != 1 || loot2.piles[0].x != 400.f || loot2.piles[0].items[0] != 18 ||
      loot2.piles[0].counts[0] != 2 || extras2.loot_respawn_t != 901.f) {
    std::printf("FAIL v14 loot count=%d x=%.0f id=%d n=%d t=%.0f\n",
                loot2.count, loot2.count ? loot2.piles[0].x : -1.f,
                loot2.count ? loot2.piles[0].items[0] : -1,
                loot2.count ? loot2.piles[0].counts[0] : -1, extras2.loot_respawn_t);
    ++fail;
  } else std::printf("OK   v14 loot piles+respawn timer\n");

  if (inv2.worn_count() != inv.worn_count() || inv2.worn_at(0) != 251 ||
      inv2.worn_iv6(0) != saved_iv6 || saved_iv6 >= 99.f) {
    std::printf("FAIL v15 worn IV6 count=%d id=%d iv6=%.1f saved=%.1f\n",
                inv2.worn_count(), inv2.worn_count() ? inv2.worn_at(0) : 0,
                inv2.worn_count() ? inv2.worn_iv6(0) : -1.f, saved_iv6);
    ++fail;
  } else std::printf("OK   v15 worn IV6 roundtrip\n");

  // v18: piles 33+ live in loot_ext_piles (legacy v14 slot holds first 32).
  {
    mdaz::LootSaveView many;
    many.count = 35;
    for (int i = 0; i < 35; ++i) {
      many.piles[i].x = 100.f + static_cast<float>(i);
      many.piles[i].y = 200.f;
      many.piles[i].item_count = 1;
      many.piles[i].items[0] = 18;
      many.piles[i].counts[0] = 1;
    }
    mdaz::SaveBlob blob18;
    if (!mdaz::SaveSystem::write_to_buffer(p, inv, w, 3, blob18, visited,
                                           mdaz::kMapSectorsX * mdaz::kMapSectorsY,
                                           66.f, 2, 4, 11, 55.f, 72.f, 3, &vs, &crops,
                                           &traps, &extras, &fires, &many)) {
      std::printf("FAIL v18 loot write\n");
      ++fail;
    } else if (blob18.loot_count != 32 || blob18.loot_ext_count != 3) {
      std::printf("FAIL v18 loot split legacy=%d ext=%d\n",
                  blob18.loot_count, blob18.loot_ext_count);
      ++fail;
    } else {
      mdaz::LootSaveView many2;
      mdaz::Player p3;
      mdaz::Inventory inv3;
      { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; inv3.restore_worn(tw_, 4); } // test: 16 C2 slots
      mdaz::WeatherState w3;
      if (!mdaz::SaveSystem::read_from_buffer(blob18, p3, inv3, w3, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                              0, 0, 0, 0, 0, 0, &many2)) {
        std::printf("FAIL v18 loot read\n");
        ++fail;
      } else if (many2.count != 35 || many2.piles[32].x != 132.f) {
        std::printf("FAIL v18 loot roundtrip count=%d x32=%.0f\n",
                    many2.count, many2.count > 32 ? many2.piles[32].x : -1.f);
        ++fail;
      } else {
        std::printf("OK   v18 extended loot piles (96 max)\n");
      }
    }
    // v19: opened car trunks bitset + ach_gta
    mdaz::WorldExtras ex19;
    ex19.trunks_opened[0] = 0x05; ex19.trunks_opened[750] = 0x80; ex19.ach_gta = 7;
    mdaz::SaveBlob blob19;
    mdaz::WorldExtras ex19r;
    mdaz::Player p4; mdaz::Inventory inv4; mdaz::WeatherState w4;
    { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; inv4.restore_worn(tw_, 4); }
    if (!mdaz::SaveSystem::write_to_buffer(p, inv, w, 3, blob19, 0, 0, 66.f, 0, 0, 0, 0.f, 100.f, 0,
                                           0, 0, 0, &ex19) ||
        !mdaz::SaveSystem::read_from_buffer(blob19, p4, inv4, w4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                            &ex19r) ||
        blob19.version != 19 || ex19r.trunks_opened[0] != 0x05 || ex19r.trunks_opened[750] != 0x80 ||
        ex19r.ach_gta != 7) {
      std::printf("FAIL v19 trunks/ach_gta roundtrip\n");
      ++fail;
    } else {
      std::printf("OK   v19 opened trunks + ach_gta saved\n");
    }
  }

  return fail ? 1 : 0;
}
