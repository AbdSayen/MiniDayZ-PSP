#include "mdaz/save_game.hpp"
#include "mdaz/item_db.hpp"

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
  db.load_from_json_string("{\"items\":[{\"id\":1,\"name\":\"Canned beans\"}]}");
  mdaz::Inventory inv;
  inv.add_item(1, 2, db);

  mdaz::WeatherState w;
  w.day_phase = 0.42f;

  bool visited[mdaz::kMapSectorsX * mdaz::kMapSectorsY];
  for (int i = 0; i < mdaz::kMapSectorsX * mdaz::kMapSectorsY; ++i) visited[i] = false;
  visited[0] = true;
  visited[17] = true;

  mdaz::SaveBlob blob;
  if (!mdaz::SaveSystem::write_to_buffer(p, inv, w, 3, blob, visited,
                                         mdaz::kMapSectorsX * mdaz::kMapSectorsY,
                                         66.f, 2, 4, 11, 55.f, 72.f, 3)) {
    std::printf("FAIL write\n");
    return 1;
  }
  if (blob.version != 6) {
    std::printf("FAIL save version %d\n", blob.version);
    ++fail;
  }

  mdaz::Player p2;
  mdaz::Inventory inv2;
  mdaz::WeatherState w2;
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
                                          &gfilt, &fish)) {
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

  return fail ? 1 : 0;
}
