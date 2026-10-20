// Item 11c: t909 IV6 durability (data.js Spawn_drop / Firearm shooting / melee wear / save v22).
#include "mdaz/game.hpp"
#include "mdaz/loot.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/save_game.hpp"
#include <cstdio>
#include <new>

using namespace mdaz;

namespace {
int g_fail = 0;
void check(bool ok, const char* what) { std::printf("%s %s\n", ok ? "OK  " : "FAIL", what); if (!ok) ++g_fail; }
}

int test_durability() {
  std::printf("--- test_durability ---\n");
  g_fail = 0;
  // Spawn_drop: 151 choose(25,25,50,50,75,100); 176 choose(50,50,75,100).
  Rng rng(7);
  bool ok151 = true, ok176 = true;
  bool seen25 = false;
  for (int i = 0; i < 400; ++i) {
    const float a = c2_spawn_drop_iv6(151, rng), b = c2_spawn_drop_iv6(176, rng);
    if (!(a == 25 || a == 50 || a == 75 || a == 100)) ok151 = false;
    if (a == 25) seen25 = true;
    if (!(b == 50 || b == 75 || b == 100)) ok176 = false;
  }
  check(ok151 && seen25, "Spawn_drop 151 IV6 in choose(25,25,50,50,75,100)");
  check(ok176, "Spawn_drop 176 IV6 in choose(50,50,75,100)");
  check(c2_spawn_drop_iv6(1, rng) == -1.f && !c2_item_has_iv6(1), "non-t909 item has no IV6");
  // Firearm shooting wear per shot.
  check(c2_fire_wear(156) == 0.8f && c2_fire_wear(154) == 1.f && c2_fire_wear(160) == 0.3f &&
            c2_fire_wear(179) == 0.4f && c2_fire_wear(181) == 0.5f,
        "fire wear table (156 .8, 154 1, 160 .3, pistol 179 .4, 181 .5)");
  // Melee NPC hit wear by IV15.
  check(c2_melee_npc_wear(203) == 1.f && c2_melee_npc_wear(200) == 0.2f && c2_melee_npc_wear(204) == 0.2f &&
            c2_melee_npc_wear(201) == 0.5f && c2_melee_npc_wear(209) == 0.5f,
        "melee NPC wear (203 1, 200/204 .2, 201/209 .5)");
  // Loot pile carries IV6 for spawn_item_iv6.
  LootSystem* ls = new (std::nothrow) LootSystem();
  if (ls) {
    LootPile* p = ls->spawn_item_iv6(Vec2{10.f, 10.f}, 151, 1, 37.5f);
    check(p && p->iv6[0] == 37.5f, "spawn_item_iv6 keeps IV6 37.5");
    LootPile* q = ls->spawn_item_iv6(Vec2{20.f, 10.f}, 1, 1, 50.f);
    check(q && q->iv6[0] == -1.f, "spawn_item_iv6 on non-t909 -> -1");
    delete ls;
  }
  // Save v22: bag cell IV6 round-trips.
  Player pl; Inventory inv; WeatherState w;
  inv.slot(0).id = 151; inv.slot(0).count = 1; inv.slot(0).iv6 = 12.5f;
  SaveBlob b;
  Player p2; Inventory i2; WeatherState w2;
  const bool ok = SaveSystem::write_to_buffer(pl, inv, w, 3, b, 0, 0, 66.f, 0, 0, 0, 0.f, 100.f, 0) &&
                  SaveSystem::read_from_buffer(b, p2, i2, w2, 0);
  check(ok && b.version == 22 && i2.slot(0).id == 151 && i2.slot(0).iv6 == 12.5f, "save v22 bag cell IV6 roundtrip");
  return g_fail;
}
