// Item 5: chopping (interact frame 7 / walk_marker IV0 = 8 / melee_swing_general IV7).
#include "mdaz/game.hpp"
#include "mdaz/platform.hpp"
#include <cmath>
#include <cstdio>
#include <new>

using namespace mdaz;

namespace {
int g_fail = 0;
void check(bool ok, const char* what) { std::printf("%s %s\n", ok ? "OK  " : "FAIL", what); if (!ok) ++g_fail; }
}

int test_chop() {
  std::printf("--- test_chop ---\n");
  g_fail = 0;
  Game* gp = new (std::nothrow) Game();
  if (!gp) { std::printf("FAIL alloc Game\n"); return 1; }
  Game& g = *gp;
  g.start_new_game(Difficulty_Regular);
  // Find an isolated tree of each kind we test (sub 2 = tree_leaves t198: drops 38,38,50).
  int ti = -1;
  for (int i = 0; i < g.tree_count_for_test() && ti < 0; ++i) {
    if (g.tree_sub_for_test(i) != 2) continue;
    bool alone = true;
    const Vec2 a = g.tree_trunk_for_test(i);
    for (int j = 0; j < g.tree_count_for_test(); ++j) {
      if (j == i) continue;
      const Vec2 b = g.tree_trunk_for_test(j);
      if ((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) < 140.f * 140.f) { alone = false; break; }
    }
    if (alone) ti = i;
  }
  check(ti >= 0, "map has an isolated tree_leaves (sub 2)");
  if (ti < 0) { delete gp; return g_fail; }
  const Vec2 trunk = g.tree_trunk_for_test(ti);
  check(g.tree_iv0_for_test(ti) == 2 && g.tree_frame_for_test(ti) == 0, "tree IV0 = 2, frame 0");
  Player& p = g.player_mut();
  // No axe: no frame 7.
  p.pos = Vec2(trunk.x, trunk.y + 40.f);
  g.equip_for_test(42);   // Hunter knife
  g.tick_pump_for_test(0.6f);
  check(g.avalible_interaction() != 7, "knife: no avalible_interaction 7");
  g.equip_for_test(206);  // Pickaxe: not IV4 1/5
  g.tick_pump_for_test(0.6f);
  check(g.avalible_interaction() != 7, "pickaxe: no avalible_interaction 7");
  g.equip_for_test(202);  // Split Axe (IV4 = 1)
  g.tick_pump_for_test(0.6f);
  check(g.avalible_interaction() == 7, "split axe at 40 px: avalible_interaction 7");
  p.pos = Vec2(trunk.x, trunk.y + 85.f);
  g.tick_pump_for_test(0.6f);
  check(g.avalible_interaction() != 7, "85 px: beyond doorchecker LOS range 70");
  // Interact: walk_marker 10 px from the trunk toward the player, then the swing.
  p.pos = Vec2(trunk.x, trunk.y + 50.f);
  g.set_attack_cd_for_test(0.f);
  check(g.start_chop_for_test(), "interact frame 7 starts the walk (nearest trunk <= 100)");
  InputState in;
  int steps = 0;
  while (steps < 200) { InputState k; if (!g.tick_chop_for_test(k)) break; p.pos.x += k.move_x * 100.f / 60.f; p.pos.y += k.move_y * 100.f / 60.f; ++steps; }
  check(std::fabs(p.pos.y - (trunk.y + 10.f)) <= 3.5f, "walked to 10 px below the trunk");
  check(std::fabs(g.attack_cd_for_test() - 1.5f) < 1e-4f, "Reloading_time_melee = Split Axe IV4 1.5 s");
  for (int k = 0; k < 15; ++k) g.tick_swings_for_test(1.f / 60.f);
  check(g.tree_iv0_for_test(ti) == 1 && g.tree_frame_for_test(ti) == 0, "hit 1: IV0 2 -> 1, still standing");
  const int s0 = g.ground_count_for_test(38), w0 = g.ground_count_for_test(50);
  // Second hit via a direct swing (Melee_2_auto path) from the same spot.
  g.swing_for_test(-90.f, true);
  for (int k = 0; k < 15; ++k) g.tick_swings_for_test(1.f / 60.f);
  check(g.tree_iv0_for_test(ti) == 0 && g.tree_frame_for_test(ti) == 1, "hit 2: IV0 0, tree frame 1 (stump)");
  std::printf("     ground 38: %d -> %d, 50: %d -> %d\n", s0, g.ground_count_for_test(38), w0, g.ground_count_for_test(50));
  check(g.ground_count_for_test(38) - s0 >= 2 && g.ground_count_for_test(38) - s0 <= 10 && g.ground_count_for_test(50) - w0 == 1, "tree_leaves drops 2x Spawn_drop sticks(38, choose 1..5 each) + 1x wood pile(50)");
  check(g.ach_lumberjack_for_test() == 1, "ach_lumberjack IV2 += 1");
  // Stump: obst frame 8 still blocks, frame 7 no longer offered, further swings do nothing.
  check(g.tree_blocks_for_test(ti, trunk), "stump obst_base frame 8 still Solid at the trunk");
  g.tick_pump_for_test(0.6f);
  check(g.avalible_interaction() != 7, "stump: no avalible_interaction 7");
  g.swing_for_test(-90.f, true);
  for (int k = 0; k < 15; ++k) g.tick_swings_for_test(1.f / 60.f);
  check(g.tree_iv0_for_test(ti) == 0 && g.ground_count_for_test(50) - w0 == 1, "swing on a stump: nothing");
  // Non-chop swing (IV7 false) does not chop.
  int t2 = -1;
  for (int i = 0; i < g.tree_count_for_test(); ++i) if (i != ti && g.tree_sub_for_test(i) >= 1 && g.tree_sub_for_test(i) <= 6) { t2 = i; break; }
  if (t2 >= 0) {
    p.pos = Vec2(g.tree_trunk_for_test(t2).x, g.tree_trunk_for_test(t2).y + 10.f);
    g.swing_for_test(-90.f, false);
    for (int k = 0; k < 15; ++k) g.tick_swings_for_test(1.f / 60.f);
    check(g.tree_iv0_for_test(t2) == 2, "swing without IV7: tree IV0 unchanged");
  }
  // Swing that misses (facing away) does nothing.
  p.pos = Vec2(trunk.x, trunk.y + 10.f);
  (void)in;
  delete gp;
  return g_fail;
}
