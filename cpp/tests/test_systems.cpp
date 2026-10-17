#include "mdaz/bandit.hpp"
#include "mdaz/bullet.hpp"
#include "mdaz/campfire.hpp"
#include "mdaz/combat.hpp"
#include "mdaz/loot.hpp"
#include "mdaz/quest.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/types.hpp"
#include "mdaz/vehicle.hpp"
#include "mdaz/weapons.hpp"

#include <cstdio>

int test_systems() {
  std::printf("--- test_systems (bandit/vehicle/quest/weapons) ---\n");
  int fail = 0;

  mdaz::set_npc_damage_for_difficulty(mdaz::Difficulty_Regular);
  mdaz::BanditSystem bs;
  mdaz::Bandit* b = bs.spawn(mdaz::Vec2(0, 0), false);
  if (!b || !b->alive) { std::printf("FAIL bandit spawn\n"); ++fail; }
  else {
    float dmg = bs.update(0.016f, mdaz::Vec2(10, 0));
    if (dmg < 12.f) { std::printf("FAIL bandit melee got %g\n", dmg); ++fail; }
    else std::printf("OK   bandit melee %.0f\n", dmg);
  }

  mdaz::VehicleSystem vs;
  mdaz::Vehicle* v = vs.spawn(mdaz::Vehicle_Car, mdaz::Vec2(0, 0));
  mdaz::Vec2 pos(5, 0);
  if (!v) { std::printf("FAIL vehicle spawn\n"); ++fail; }
  else {
    bool drove = vs.update(0.1f, pos, 1.f, 0.f, true); // mount
    drove = vs.update(0.1f, pos, 1.f, 0.f, false);
    if (!drove || !vs.player_driving()) { std::printf("FAIL drive\n"); ++fail; }
    else std::printf("OK   vehicle mount/drive\n");
    if (!v->stash_add(18, 2) || v->stash_used() != 1) {
      std::printf("FAIL vehicle stash add\n");
      ++fail;
    } else {
      mdaz::ItemId id = 0;
      int n = 0;
      if (!v->stash_take(id, n) || id != 18 || n != 2) {
        std::printf("FAIL vehicle stash take\n");
        ++fail;
      } else std::printf("OK   vehicle stash\n");
    }
  }

  mdaz::QuestState q;
  q.start(mdaz::Quest_KillZeds10);
  if (!q.tick(1, 10, false)) { std::printf("FAIL quest complete\n"); ++fail; }
  else std::printf("OK   quest kill-10\n");
  q.start(mdaz::Quest_VisitHospital);
  if (!q.tick(1, 0, false, true)) { std::printf("FAIL quest hospital\n"); ++fail; }
  else std::printf("OK   quest hospital\n");
  {
    mdaz::QuestState q2;
    q2.start(mdaz::Quest_VisitFirestation);
    if (!q2.tick(1, 0, false, false, 0, 0, false, true, false)) {
      std::printf("FAIL quest firestation\n");
      ++fail;
    } else std::printf("OK   quest firestation\n");
    q2.start(mdaz::Quest_VisitChurch);
    if (!q2.tick(1, 0, false, false, 0, 0, false, false, true)) {
      std::printf("FAIL quest church\n");
      ++fail;
    } else std::printf("OK   quest church\n");
    if (q2.next_after(mdaz::Quest_VisitPolice) != mdaz::Quest_VisitFirestation ||
        q2.next_after(mdaz::Quest_VisitFirestation) != mdaz::Quest_VisitChurch ||
        q2.next_after(mdaz::Quest_VisitChurch) != mdaz::Quest_SurviveDay1) {
      std::printf("FAIL quest chain\n");
      ++fail;
    } else std::printf("OK   quest chain fire→church→day2\n");
  }

  const mdaz::WeaponDef* fnx = mdaz::find_weapon(151);
  if (!fnx || fnx->stats.damage != 23.f) {
    std::printf("FAIL FNX dmg want 23 got %g\n", fnx ? fnx->stats.damage : -1.f);
    ++fail;
  } else std::printf("OK   FNX base dmg 23\n");

  if (!mdaz::find_weapon(181) || !mdaz::find_weapon(185) || !mdaz::find_weapon(187)) {
    std::printf("FAIL missing PM/VSS/Madsen weapon defs\n");
    ++fail;
  } else std::printf("OK   PM/VSS/Madsen defs present\n");

  const mdaz::WeaponDef* sword = mdaz::find_weapon(209);
  if (!sword || sword->stats.damage != 15.f) {
    std::printf("FAIL sword dmg want 15\n");
    ++fail;
  } else std::printf("OK   sword base dmg 15\n");

  mdaz::BulletSystem bullets;
  mdaz::Bullet* bu = bullets.spawn(mdaz::Vec2(0, 0), 0.f, 100.f, 10.f, 1.f, true);
  bullets.update(0.5f);
  if (!bu || !bu->alive || bu->pos.x < 40.f) {
    std::printf("FAIL bullet travel\n");
    ++fail;
  } else std::printf("OK   bullet travel\n");

  mdaz::CampfireSystem fires;
  mdaz::Campfire* f = fires.place(mdaz::Vec2(0, 0), 10.f);
  if (!f || !fires.near_fire(mdaz::Vec2(10, 0), 55.f)) {
    std::printf("FAIL campfire warmth\n");
    ++fail;
  } else {
    fires.update(11.f);
    if (fires.near_fire(mdaz::Vec2(0, 0))) {
      std::printf("FAIL campfire should expire\n");
      ++fail;
    } else std::printf("OK   campfire place/expire\n");
  }

  // Spawn_drop weighted table: IDs 15 and 100 dominate extract counts
  {
    mdaz::LootSystem loot;
    mdaz::Rng rng;
    rng.seed(42u);
    int hit_top = 0;
    int total_items = 0;
    for (int i = 0; i < 80; ++i) {
      mdaz::LootPile* p = loot.spawn_random(mdaz::Vec2(0, 0), rng);
      if (!p) continue;
      for (int j = 0; j < p->item_count; ++j) {
        ++total_items;
        if (p->items[j] == 15 || p->items[j] == 100) ++hit_top;
      }
    }
    if (total_items < 40 || hit_top * 4 < total_items) {
      std::printf("FAIL spawn_drop weight top=%d / %d\n", hit_top, total_items);
      ++fail;
    } else {
      std::printf("OK   spawn_drop weighted (%d/%d top IDs)\n", hit_top, total_items);
    }
  }

  // Military loot table (army zed corpse path)
  {
    mdaz::LootSystem loot;
    mdaz::Rng rng(7);
    mdaz::LootPile* mil = loot.spawn_military(mdaz::Vec2(2, 2), rng);
    if (!mil || mil->item_count < 1) {
      std::printf("FAIL military loot\n");
      ++fail;
    } else std::printf("OK   military loot (%d items)\n", mil->item_count);
  }

  // Typed firestation / church tables return piles with items
  {
    mdaz::LootSystem loot;
    mdaz::Rng rng;
    rng.seed(7u);
    mdaz::LootPile* fire = loot.spawn_firestation(mdaz::Vec2(1, 1), rng);
    mdaz::LootPile* church = loot.spawn_church(mdaz::Vec2(2, 2), rng);
    if (!fire || fire->item_count < 1 || !church || church->item_count < 1) {
      std::printf("FAIL firestation/church loot piles\n");
      ++fail;
    } else {
      std::printf("OK   firestation/church loot (%d / %d items)\n",
                  fire->item_count, church->item_count);
    }
  }

  // Bunker dens table: mil/med mix
  {
    mdaz::LootSystem loot;
    mdaz::Rng rng;
    rng.seed(11u);
    mdaz::LootPile* bun = loot.spawn_bunker(mdaz::Vec2(3, 3), rng);
    if (!bun || bun->item_count < 2) {
      std::printf("FAIL bunker loot pile\n");
      ++fail;
    } else {
      std::printf("OK   bunker loot (%d items)\n", bun->item_count);
    }
  }

  return fail ? 1 : 0;
}
