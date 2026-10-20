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
  // Item 7: Obstacles_transparenting — tree above the player only when overlapping and player Y <= Imagepoint 2.
  for (int i = 0; i < g.tree_count_for_test(); ++i) {
    if (g.tree_sub_for_test(i) != 1) continue;
    const Vec2 t = g.tree_trunk_for_test(i);
    Player& p = g.player_mut();
    p.pos = Vec2(t.x, t.y - 40.f);
    check(g.tree_above_for_test(i), "pine: player 40 px north of the trunk (inside crown, Y <= ip2) -> Buildings (above)");
    p.pos = Vec2(t.x, t.y + 2.f);
    check(!g.tree_above_for_test(i), "pine: player at the trunk (Y > ip2) -> Buildings_bg (below)");
    p.pos = Vec2(t.x + 200.f, t.y - 40.f);
    check(!g.tree_above_for_test(i), "pine: not overlapping -> Buildings_bg");
    break;
  }
  for (int i = 0; i < g.tree_count_for_test(); ++i) {
    if (g.tree_sub_for_test(i) != 9) continue;
    const Vec2 t = g.tree_trunk_for_test(i);
    Player& p = g.player_mut();
    p.pos = Vec2(t.x, t.y - 20.f);   // ip2 of frame 2 is 0.144*167 = 24 px below the origin
    check(g.tree_above_for_test(i), "tree_block f2: player above Imagepoint 2 -> above");
    p.pos = Vec2(t.x, t.y + 40.f);
    check(!g.tree_above_for_test(i), "tree_block f2: player below Imagepoint 2 -> below");
    break;
  }
  std::printf("     tree_block subs on Map: 7=%d 8=%d 9=%d 10=%d\n", seen[7], seen[8], seen[9], seen[10]);
  // Tutorial: tree_block instances keep their layout frame.
  g.start_tutorial();
  int tb[11] = {0};
  for (int i = 0; i < g.tree_count_for_test(); ++i) { const int s = g.tree_sub_for_test(i); if (s >= 7 && s <= 10) ++tb[s]; }
  std::printf("     tutorial tree_block: 7=%d 8=%d 9=%d 10=%d\n", tb[7], tb[8], tb[9], tb[10]);
  check(tb[10] > 0, "tutorial tree_block frame 3 -> sub 10 (was drawn/collided as frame 0)");
  // 7b: t920 car (origin rule, stateful) and t914/t930 Imagepoint 2 rule.
  {
    Player& p = g.player_mut(); bool roof = false; const Vec2 o(1000.f, 1000.f);
    p.pos = Vec2(1000.f, 990.f);
    check(g.location_above_for_test(3, "car_police-sheet0", o, roof), "car: player Y < origin -> Buildings");
    p.pos = Vec2(1000.f, 1010.f);
    check(!g.location_above_for_test(3, "car_police-sheet0", o, roof), "car: player Y >= origin -> Buildings_bg");
    p.pos = Vec2(1300.f, 1300.f);
    check(!g.location_above_for_test(3, "car_police-sheet0", o, roof), "car: no reset when not overlapping");
    p.pos = Vec2(1000.f, 1000.f);   // van ip2 = (0.71186-0.50847)*59 = 12 px below origin
    check(g.location_above_for_test(4, "car_van_red-sheet0", o, roof), "van (t930): Y <= ip2 -> Buildings");
    p.pos = Vec2(1000.f, 1020.f);
    check(!g.location_above_for_test(4, "car_van_red-sheet0", o, roof), "van (t930): Y > ip2 -> Buildings_bg");
    p.pos = Vec2(1300.f, 1300.f);
    check(!g.location_above_for_test(4, "car_van_red-sheet0", o, roof), "van: not overlapping -> Buildings_bg");
    p.pos = Vec2(1000.f, 970.f);   // barrack ip1 = 18 px above origin
    check(g.location_above_for_test(5, "b_military_barrack-sheet0", o, roof) && roof, "house t928: Y < ip1 -> opacity 25 + Buildings");
    p.pos = Vec2(1000.f, 1000.f);
    check(g.location_above_for_test(5, "b_military_barrack-sheet0", o, roof) && !roof, "house t928: ip1 <= Y <= ip2 -> opacity 100, Buildings");
    p.pos = Vec2(1000.f, 970.f);
    g.house_layers_tick_for_test();
    p.pos = Vec2(-90000.f, -90000.f);   // overlaps no t928 -> TriggerOnce reset
    g.house_layers_tick_for_test();
    check(!g.iv41_for_test(), "IV41 cleared when not overlapping any house (t943 Outline width 0)");
    check(!g.location_above_for_test(5, "b_military_barrack-sheet0", o, roof) && !roof, "house t928: not overlapping -> Buildings_bg, 100");
  }
  // 7e: map props on their C2 layers (Obstacles_transparenting ip2 rule) and car-breaks-props.
  {
    g.start_new_game(Difficulty_Regular);
    const int k = g.find_prop_type_for_test(305);
    check(k >= 0, "map has an obst_trash_container (t305) prop");
    if (k >= 0) {
      Player& p = g.player_mut();
      const Vec2 o = g.prop_pos_for_test(k);
      const float ip2 = g.prop_ip2_for_test(k);
      p.pos = Vec2(o.x, ip2 - 2.f);
      check(g.prop_layer_for_test(k) == 2, "trash: overlapping, Y < ip2 -> Buildings");
      p.pos = Vec2(o.x, ip2 + 2.f);
      check(g.prop_layer_for_test(k) == 1, "trash: overlapping, Y > ip2 -> Buildings_bg");
      p.pos = Vec2(o.x, ip2 - 2.f);
      g.prop_layer_for_test(k);
      p.pos = Vec2(o.x + 5000.f, o.y + 5000.f);
      check(g.prop_layer_for_test(k) == 1, "trash: TriggerOnce reset -> Buildings_bg");
      const int before = g.props_broken_for_test();
      check(g.car_break_for_test(o, 40.f) == before && g.prop_alive_for_test(k), "car at speed 40 (<= 50): prop kept");
      check(g.car_break_for_test(o, 60.f) > before && !g.prop_alive_for_test(k), "car at speed 60 (> 50): trash container destroyed");
    }
  }
  delete gp;
  return g_fail;
}
