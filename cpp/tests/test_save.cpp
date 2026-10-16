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

  mdaz::ItemDatabase db;
  db.load_from_json_string("{\"items\":[{\"id\":1,\"name\":\"Canned beans\"}]}");
  mdaz::Inventory inv;
  inv.add_item(1, 2, db);

  mdaz::WeatherState w;
  w.day_phase = 0.42f;

  mdaz::SaveBlob blob;
  if (!mdaz::SaveSystem::write_to_buffer(p, inv, w, blob)) {
    std::printf("FAIL write\n");
    return 1;
  }

  mdaz::Player p2;
  mdaz::Inventory inv2;
  mdaz::WeatherState w2;
  if (!mdaz::SaveSystem::read_from_buffer(blob, p2, inv2, w2)) {
    std::printf("FAIL read\n");
    return 1;
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

  return fail ? 1 : 0;
}
