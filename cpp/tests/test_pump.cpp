// Item 4: C2 waterpump (t435) + avalible_interaction priority (loot_checker2).
#include "mdaz/game.hpp"
#include <cstdio>
#include <new>

using namespace mdaz;

namespace {
int g_fail = 0;
void check(bool ok, const char* what) { std::printf("%s %s\n", ok ? "OK  " : "FAIL", what); if (!ok) ++g_fail; }
}

int test_pump() {
  std::printf("--- test_pump ---\n");
  g_fail = 0;
  Game* gp = new (std::nothrow) Game();
  if (!gp) { std::printf("FAIL alloc Game\n"); return 1; }
  Game& g = *gp;
  g.start_tutorial();
  const int pk = g.first_pump_for_test();
  check(pk >= 0, "tutorial layout has the waterpump");
  if (pk < 0) { delete gp; return g_fail; }
  check(g.pump_water(pk) == 250, "tutorial pump IV0 = 250 (layout instance)");
  Player& p = g.player_mut();
  p.pos = Vec2(4634.f + 300.f, 742.f);
  g.tick_pump_for_test(0.6f);
  check(g.avalible_interaction() != 4, "pump 300 px away: no frame 4");
  p.pos = Vec2(4634.f + 60.f, 742.f);
  g.tick_pump_for_test(0.6f);
  check(g.avalible_interaction() == 4, "pump <= 100 px with LOS: avalible_interaction = 4");
  // Drink: IV14 50 -> pump -= min(25, 100-50) = 25; after Wait 2: IV14 + 25.
  p.survival.water = 50.f;
  g.pump_drink_for_test(pk);
  check(g.pump_water(pk) == 225, "drink: pump 250 -> 225");
  check(p.survival.water == 50.f, "water unchanged during Wait 2");
  g.tick_pump_for_test(1.0f);
  check(p.survival.water == 50.f, "still waiting at 1 s");
  g.tick_pump_for_test(1.1f);
  check(p.survival.water == 75.f, "after 2 s: water +25");
  // IV14 90: pump -= round(min(25, 10)) = 10, water +25 (C2 adds 25 regardless).
  p.survival.water = 90.f;
  g.pump_drink_for_test(pk);
  check(g.pump_water(pk) == 215, "IV14 90: pump -= 10");
  g.tick_pump_for_test(2.1f);
  check(p.survival.water == 115.f, "IV14 90: +25 (Update_Stats clamps later)");
  // IV14 >= 100: "I don't want to drink."
  p.survival.water = 100.f;
  g.pump_drink_for_test(pk);
  check(g.pump_water(pk) == 215, "IV14 100: nothing taken");
  // pump <= 25: water_left = pump, pump = 0.
  g.set_pump_water_for_test(pk, 20);
  p.survival.water = 10.f;
  g.pump_drink_for_test(pk);
  g.tick_pump_for_test(2.1f);
  check(g.pump_water(pk) == 0 && p.survival.water == 30.f, "pump 20: water +20, pump 0");
  // empty pump
  g.pump_drink_for_test(pk);
  g.tick_pump_for_test(2.1f);
  check(p.survival.water == 30.f, "empty pump: no water");
  // Global_respawn pump part: pumps >= 500 px from the player get choose(0,25,0,25,50,75,100,150,200);
  // closer pumps keep their IV0. Use the Map layout (new game).
  g.start_new_game(Difficulty_Regular);
  const int n = g.pump_count_for_test();
  check(n > 1, "Map has waterpumps");
  if (n > 1) {
    const int a = g.pump_index_for_test(0);
    const MapProp& pa = g.prop_for_test(a);
    p.pos = Vec2(pa.x + 10.f, pa.y);
    for (int k = 0; k < n; ++k) g.set_pump_water_for_test(g.pump_index_for_test(k), 777);
    g.global_respawn_pumps_for_test();
    bool near_kept = true, far_ok = true; int far = 0;
    for (int k = 0; k < n; ++k) {
      const int i = g.pump_index_for_test(k);
      const MapProp& m = g.prop_for_test(i);
      const float dx = m.x - p.pos.x, dy = m.y - p.pos.y;
      const int w = g.pump_water(i);
      if (dx * dx + dy * dy < 500.f * 500.f) { if (w != 777) near_kept = false; }
      else { ++far; if (!(w == 0 || w == 25 || w == 50 || w == 75 || w == 100 || w == 150 || w == 200)) far_ok = false; }
    }
    check(near_kept, "Global_respawn: pumps < 500 px untouched");
    check(far > 0 && far_ok, "Global_respawn: pumps >= 500 px in choose(0..200) set");
  }
  delete gp;
  return g_fail;
}
