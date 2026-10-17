#include "mdaz/loot.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/weapon_unlocks.hpp"

#include <cstdio>
#include <cmath>
#include <cstring>

int test_weapon_unlocks() {
  std::printf("--- test_weapon_unlocks / drop ---\n");
  int fail = 0;

  mdaz::WeaponUnlockState st;
  for (int i = 0; i < 4; ++i)
    mdaz::weapon_unlock_on_zed_kill(st, 158, mdaz::Difficulty_Regular); // M4 → aug cluster
  if (st.aug_progress != 4 || st.aug_unlocked) {
    std::printf("FAIL aug progress=%d unlocked=%d\n", st.aug_progress, (int)st.aug_unlocked);
    ++fail;
  } else {
    mdaz::weapon_unlock_on_zed_kill(st, 160, mdaz::Difficulty_Regular);
    if (!st.aug_unlocked || st.aug_progress != 5) {
      std::printf("FAIL aug unlock at 5\n");
      ++fail;
    } else std::printf("OK   aug unlock 5 zed kills (Regular)\n");
  }

  st.reset();
  mdaz::weapon_unlock_on_zed_kill(st, 151, mdaz::Difficulty_Novice);
  if (st.mac10_progress != 0) {
    std::printf("FAIL mac10 on Novice\n");
    ++fail;
  } else std::printf("OK   mac10 skipped on Novice\n");

  st.reset();
  for (int i = 0; i < 5; ++i)
    mdaz::weapon_unlock_on_zed_kill(st, 156, mdaz::Difficulty_Novice);
  if (!st.sv98_unlocked) {
    std::printf("FAIL sv98 unlock\n");
    ++fail;
  } else std::printf("OK   sv98 unlock mosin kills\n");

  st.reset();
  for (int i = 0; i < 6; ++i)
    mdaz::weapon_unlock_on_bleed_second(st, mdaz::Difficulty_Veteran);
  if (!st.vss_unlocked) {
    std::printf("FAIL vss bleed unlock\n");
    ++fail;
  } else std::printf("OK   vss unlock 6 bleed seconds Veteran\n");

  mdaz::WeaponUnlockState locked;
  if (mdaz::weapon_unlock_allows_loot_item(locked, 185)) {
    std::printf("FAIL vss loot gate when locked\n");
    ++fail;
  } else if (!mdaz::weapon_unlock_allows_loot_item(st, 185)) {
    std::printf("FAIL vss loot when unlocked\n");
    ++fail;
  } else std::printf("OK   loot gate flags\n");

  st.reset();
  mdaz::ItemId u = 0;
  for (int i = 0; i < 4; ++i)
    u = mdaz::weapon_unlock_on_zed_kill(st, 158, mdaz::Difficulty_Regular);
  if (u != 0) {
    std::printf("FAIL unlock return before threshold\n");
    ++fail;
  }
  u = mdaz::weapon_unlock_on_zed_kill(st, 158, mdaz::Difficulty_Regular);
  if (u != 177) {
    std::printf("FAIL unlock return aug want 177 got %d\n", (int)u);
    ++fail;
  } else if (std::strcmp(mdaz::weapon_unlock_toast_title_c2(), "New weapon unlocked!") != 0) {
    std::printf("FAIL toast title\n");
    ++fail;
  } else if (std::strcmp(mdaz::weapon_unlock_toast_body_c2(177), "AUG unlocked.") != 0) {
    std::printf("FAIL toast body aug\n");
    ++fail;
  } else {
    std::printf("OK   unlock return + Show_unlock_wpn strings\n");
  }

  mdaz::LootSystem loot;
  mdaz::Rng rng;
  mdaz::LootPile* mil = loot.spawn_military(mdaz::Vec2(0, 0), rng, &locked);
  if (mil) {
    for (int i = 0; i < mil->item_count; ++i) {
      if (mil->items[i] == 177 || mil->items[i] == 179 || mil->items[i] == 186 ||
          mil->items[i] == 178 || mil->items[i] == 185) {
        std::printf("FAIL locked military spawn tier weapon %d\n", (int)mil->items[i]);
        ++fail;
      }
    }
    if (!fail) std::printf("OK   military spawn unlock filter\n");
  }

  mdaz::LootPile* p = loot.spawn_drop_from_player(mdaz::Vec2(100.f, 200.f), 38, 2);
  if (!p || p->item_count != 1 || p->items[0] != 38 || p->counts[0] != 2) {
    std::printf("FAIL spawn_drop_from_player\n");
    ++fail;
  } else if (std::fabs(p->pos.y - 215.f) > 0.01f) {
    std::printf("FAIL drop Y offset got %g\n", p->pos.y);
    ++fail;
  } else std::printf("OK   Spawn_drop_from_player Y+15\n");

  return fail ? 1 : 0;
}
