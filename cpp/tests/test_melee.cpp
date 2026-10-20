#include "mdaz/bandit.hpp"
// Item 5b: Melee_2_auto 1:1 — every melee attack (fist and items) spawns melee_swing_general
// (t910) that deals damage on collision; no radius damage.
#include "mdaz/game.hpp"
#include <cmath>
#include <cstdio>
#include <new>

using namespace mdaz;

namespace {
int g_fail = 0;
void check(bool ok, const char* what) { std::printf("%s %s\n", ok ? "OK  " : "FAIL", what); if (!ok) ++g_fail; }
}

int test_melee() {
  std::printf("--- test_melee ---\n");
  g_fail = 0;
  // Tables (data.js Melee_2_auto / pick-up events / layout t909 IV4).
  check(c2_melee_iv4(202) == 1 && c2_melee_iv4(204) == 5 && c2_melee_iv4(44) == 7 && c2_melee_iv4(42) == 8 &&
        c2_melee_iv4(43) == 9 && c2_melee_iv4(209) == 15 && c2_melee_iv4(210) == 16 && c2_melee_iv4(0) == 0,
        "player_collision_base IV4 per item (axe 1, fireaxe 5, knives 7/8/9, sword 15, pan 16)");
  check(c2_melee_swing_dmg(1) == 50 && c2_melee_swing_dmg(5) == 60 && c2_melee_swing_dmg(15) == 75 &&
        c2_melee_swing_dmg(8) == 20 && c2_melee_swing_dmg(0) == 10, "swing IV0: 50 / 60 / 75 / 20 / fist 10");
  check(std::fabs(c2_melee_reload(208) - 1.8f) < 1e-5f && std::fabs(c2_melee_reload(42) - 0.8f) < 1e-5f,
        "Reloading_time_melee: sledgehammer 1.8, knife_any 0.8");
  check(c2_melee_crit_chance(0, false) == 10 && c2_melee_crit_chance(16, true) == 50 &&
        c2_melee_crit_chance(1, false) == 25, "crit chance fist 10, pan+perk 50, axe 25");

  Game* gp = new (std::nothrow) Game();
  if (!gp) return 1;
  Game& g = *gp;
  g.start_new_game(Difficulty_Regular);
  Player& p = g.player_mut();
  // Find a free spot (no obstacle in a 60 px box) near the start.
  Vec2 base = p.pos;
  for (int t = 0; t < 400; ++t) {
    bool free = true;
    for (int dx = -40; dx <= 40 && free; dx += 4)
      for (int dy = -40; dy <= 40 && free; dy += 4)
        if (g.blocks_for_test(Vec2(base.x + dx, base.y + dy))) free = false;
    if (free) break;
    base.x += 37.f; base.y += 23.f;
  }
  g.clear_zeds_for_test();
  p.pos = base;
  p.facing_deg = 0.f;
  // Fist: zed 20 px in front gets hit, zed 20 px behind does not.
  Zombie* front = g.spawn_zed_for_test(Vec2(base.x + 20.f, base.y));
  Zombie* back = g.spawn_zed_for_test(Vec2(base.x - 30.f, base.y));
  check(front && back, "spawned two zeds");
  if (!front || !back) { delete gp; return g_fail; }
  check(g.unequip_for_test(), "empty hand (fist)");
  g.set_attack_cd_for_test(0.f);
  const float hf = front->hp, hb = back->hp;
  g.melee_attack_for_test();
  const float d0 = g.swing_dmg_for_test(0);
  check((d0 >= 5.f && d0 <= 15.f) || (d0 >= 10.f && d0 <= 30.f), "fist swing IV0 = 10 + choose(-5..5) (x2 on crit)");
  check(std::fabs(g.attack_cd_for_test() - 0.5f) < 1e-5f, "fist Reloading_time_melee 0.5 s");
  for (int k = 0; k < 15; ++k) g.tick_swings_for_test(1.f / 60.f);
  const float lost = hf - front->hp;
  std::printf("     fist: swing IV0 %.0f, front zed lost %.0f, back zed lost %.0f\n", d0, lost, hb - back->hp);
  check(lost > 0.f && lost <= std::floor(d0 * 1.25f + 0.5f) && lost >= std::floor(d0 * 0.75f + 0.5f) - 0.01f,
        "front zed: round(IV0 + random(-IV0/4, IV0/4))");
  check(back->hp == hb, "zed behind the player is not hit (no radius damage)");
  check(g.melee_hits_for_test() == 1, "swing destroyed after one NPC hit");
  // Sword (IV4 15 -> 75): zed 60 px away is out of reach (swing travels 20 px in 0.2 s).
  g.equip_for_test(209);
  g.set_attack_cd_for_test(0.f);
  front->pos = Vec2(base.x + 60.f, base.y);
  const float h2 = front->hp;
  g.melee_attack_for_test();
  const float d1 = g.swing_dmg_for_test(0) >= 0.f ? g.swing_dmg_for_test(0) : g.swing_dmg_for_test(1);
  check(d1 == 75.f || d1 == 150.f, "sword swing IV0 75 (150 on crit)");
  check(std::fabs(g.attack_cd_for_test() - 1.5f) < 1e-5f, "sword Reloading_time_melee 1.5 s");
  for (int k = 0; k < 15; ++k) g.tick_swings_for_test(1.f / 60.f);
  check(front->hp == h2, "zed at 60 px: swing fades before reaching it");
  delete gp;
  // NPC hit polys (data.js): zed skin 30x30 bbox = 15 px half-size; swing strip is 10 px long.
  check(seg_hits_box(Vec2(-24.f, 0.f), Vec2(-14.f, 0.f), -15.f, -15.f, 15.f, 15.f), "swing tip enters 30x30 zed bbox");
  check(!seg_hits_box(Vec2(-26.f, 0.f), Vec2(-16.f, 0.f), -15.f, -15.f, 15.f, 15.f), "swing 1 px short of 30x30 bbox misses");
  check(seg_hits_box(Vec2(0.f, 16.f), Vec2(0.f, 26.f), -16.f, -15.f, 16.f, 17.f) == true, "box test inclusive of start inside");
  {
    BanditSystem bs; Rng r(5);
    spawn_bandit_camp(bs, Vec2(0.f, 0.f), r);
    int n243 = 0, n540 = 0;
    for (const Bandit* b = bs.begin(); b != bs.end(); ++b) { n243 += b->c2_obj == 243; n540 += b->c2_obj == 540; }
    check(n243 == 2 && n540 == 2, "camp bots = 2 shooter#243 + 2 puncher#540 (Survivors layer)");
    Bandit* rb = bs.spawn(Vec2(0.f, 0.f), true);
    check(rb && rb->c2_obj == 827, "roaming bandit = bot_enemy_bandit#827 (player layer)");
  }
  return g_fail;
}
