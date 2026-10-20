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
  // Item 11d: GUI_Drag + gui_pickpile.
  check(c2_check_craft_slot(12, 12) == 1 && c2_check_craft_slot(31, 31) == 0 &&
            c2_check_craft_slot(40, 41) == 2 && c2_check_craft_slot(41, 12) == 0,
        "Check_craft Slot: same Combine id 1, recipe pair 2, else 0");
  Game* gp = new (std::nothrow) Game();
  if (gp) {
    Game& g = *gp;
    g.start_new_game(Difficulty_Regular);
    g.harness_drag(0);
    const int piles0 = g.pickpile_count();
    g.harness_drag(1);
    g.harness_drag(2);   // swap 0 <-> 1
    const Inventory& inv = g.inventory();
    check(inv.slot(0).id == 12 && inv.slot(0).count == 25 && inv.slot(1).id == 156 &&
              inv.slot(1).iv6 == 75.f, "Replace_clear_slot swaps cells (IV6 travels)");
    g.harness_drag(3);   // ammo 25 onto ammo 30 -> Combine_ammo (stack 40)
    check(inv.slot(2).id == 12 && inv.slot(2).count == 40 && inv.slot(0).count == 15,
          "drag same ammo onto ammo -> Combine (40 + 15)");
    int c0 = 0;
    const ItemId g0 = g.pickpile_id(0, &c0);
    g.harness_drag(4);
    g.harness_drag(5);   // rifle onto pickpile row 0
    bool rifle_on_ground = false;
    for (int k = 0; k < g.pickpile_count(); ++k) if (g.pickpile_id(k, 0) == 156) rifle_on_ground = true;
    check(piles0 >= 2 && inv.slot(1).id == g0 && inv.slot(1).count == c0 && rifle_on_ground,
          "Replace_slot_ground: cell dropped, ground item into the cell");
    delete gp;
  }
  return g_fail;
}
