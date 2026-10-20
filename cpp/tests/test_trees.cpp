// Item 6: tree collision 1:1 (obst_base 10 for sprites 1..6, big_obstacle_base 17/21/22/23 for
// tree_block frames 0..3 = create_map_element sub 7..10; tutorial tree_block keeps its frame).
#include "mdaz/game.hpp"
#include <cstdio>
#include <new>

using namespace mdaz;

namespace {
int g_fail = 0;
void check(bool ok, const char* what) { std::printf("%s %s\n", ok ? "OK  " : "FAIL", what); if (!ok) ++g_fail; }
// Centroid of each big_obstacle_base poly (fractions x frame size), relative to the spawn point.
const float kCx[4] = {0.06f * 145.f, -0.025f * 160.f, -0.018f * 140.f, 0.005f * 149.f};
const float kCy[4] = {0.12f * 172.f, 0.295f * 132.f, 0.18f * 167.f, 0.135f * 169.f};
}

int test_trees() {
  std::printf("--- test_trees ---\n");
  g_fail = 0;
  Game* gp = new (std::nothrow) Game();
  if (!gp) return 1;
  Game& g = *gp;
  g.start_new_game(Difficulty_Regular);
  int seen[11] = {0};
  for (int sub = 7; sub <= 10; ++sub) {
    for (int i = 0; i < g.tree_count_for_test(); ++i) {
      if (g.tree_sub_for_test(i) != sub) continue;
      ++seen[sub];
      if (seen[sub] > 1) continue;
      const Vec2 t = g.tree_trunk_for_test(i);
      char msg[96];
      std::snprintf(msg, sizeof(msg), "tree_block sub %d (frame %d): poly %d blocks its centroid", sub, sub - 7, sub == 7 ? 17 : 13 + sub);
      check(g.blocks_for_test(Vec2(t.x + kCx[sub - 7], t.y + kCy[sub - 7])), msg);
    }
  }
  // Frame 1 (sub 8): its poly is a low strip (y 0.20..0.41 of 132); 30 px above the spawn
  // point is outside it (frame 17 would not matter here either: it is not used for sub 8).
  for (int i = 0; i < g.tree_count_for_test(); ++i) {
    if (g.tree_sub_for_test(i) != 8) continue;
    const Vec2 t = g.tree_trunk_for_test(i);
    check(!g.blocks_for_test(Vec2(t.x, t.y - 30.f)), "sub 8: point above the frame-21 strip is free");
    break;
  }
  // Sprite trees: the trunk triangle (frame 10) blocks just above the trunk point.
  for (int i = 0; i < g.tree_count_for_test(); ++i) {
    if (g.tree_sub_for_test(i) < 1 || g.tree_sub_for_test(i) > 6) continue;
    const Vec2 t = g.tree_trunk_for_test(i);
    check(g.blocks_for_test(Vec2(t.x, t.y - 2.f)), "sprite tree: obst_base frame 10 blocks at the trunk");
    check(!g.blocks_for_test(Vec2(t.x, t.y - 15.f)), "sprite tree: 15 px above the trunk is free");
    break;
  }
  std::printf("     tree_block subs on Map: 7=%d 8=%d 9=%d 10=%d\n", seen[7], seen[8], seen[9], seen[10]);
  // Tutorial: tree_block instances keep their layout frame.
  g.start_tutorial();
  int tb[11] = {0};
  for (int i = 0; i < g.tree_count_for_test(); ++i) { const int s = g.tree_sub_for_test(i); if (s >= 7 && s <= 10) ++tb[s]; }
  std::printf("     tutorial tree_block: 7=%d 8=%d 9=%d 10=%d\n", tb[7], tb[8], tb[9], tb[10]);
  check(tb[10] > 0, "tutorial tree_block frame 3 -> sub 10 (was drawn/collided as frame 0)");
  delete gp;
  return g_fail;
}
