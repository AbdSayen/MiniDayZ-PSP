// Item 10: C2 perk state (Restart_game / perkmenu buy / notify).
#include "mdaz/perks.hpp"
#include "mdaz/ui_strings.hpp"

#include <cstdio>
#include <cstring>

using namespace mdaz;

int test_perks() {
  std::printf("--- test_perks ---\n");
  int fail = 0;
  auto check = [&](bool ok, const char* what) {
    std::printf("%s %s\n", ok ? "OK  " : "FAIL", what);
    if (!ok) ++fail;
  };
  PerkState p;
  check(p.xp_raw == 16 && p.cost == 200 && p.learnt == 0 && !p.all_owned(), "Restart_game values");
  check(!p.can_buy(Perk_Camel), "XP 16 < 200: cannot buy");
  p.xp_raw = 10000;
  int expect = 200;
  for (int i = 0; i < Perk_Count; ++i) {
    if (p.cost != expect) { check(false, "perk_cost sequence"); break; }
    p.buy(i);
    expect = 216 * (i + 2) + 100 * (i + 1) - 16 * (i + 1);
  }
  check(p.learnt == 16 && p.all_owned() && p.xp_raw == 10000, "16 buys, XP never deducted");
  check(p.cost == 216 + 300 * 16, "perk_cost after 16 = 216 + 300c");
  check(!p.notify_visible(), "notify hidden when all 16 perks are owned");
  PerkState q;
  q.unpack_bits(p.pack_bits());
  check(q.all_owned(), "pack/unpack 16 bits");
  int seen = 0;
  for (int i = 0; i < Perk_Count; ++i) seen |= 1 << kPerkMenuSlots[i].frame;
  check(seen == 0xFFFF, "perk menu grid holds each frame once");
  check(std::strcmp(perk_c2_var(Perk_Metabolism), "perk_metabolism") == 0 &&
        std::strcmp(perk_c2_var(Perk_Header), "perk_header") == 0, "frame -> C2 global");
  const char* d = ui_string(403);
  check(d && std::strncmp(d, "Camel\n", 6) == 0, "l_eng_ui 403 Camel description");
  int sx, sy; perk_icon_src(15, sx, sy);
  check(sx == 1 && sy == 277, "icon frame 15 at (1,277)");
  return fail;
}
