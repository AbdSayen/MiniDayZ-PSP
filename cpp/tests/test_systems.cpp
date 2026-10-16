#include "mdaz/bandit.hpp"
#include "mdaz/bullet.hpp"
#include "mdaz/campfire.hpp"
#include "mdaz/quest.hpp"
#include "mdaz/vehicle.hpp"
#include "mdaz/weapons.hpp"

#include <cstdio>

int test_systems() {
  std::printf("--- test_systems (bandit/vehicle/quest/weapons) ---\n");
  int fail = 0;

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
  }

  mdaz::QuestState q;
  q.start(mdaz::Quest_KillZeds10);
  if (!q.tick(1, 10, false)) { std::printf("FAIL quest complete\n"); ++fail; }
  else std::printf("OK   quest kill-10\n");
  q.start(mdaz::Quest_VisitHospital);
  if (!q.tick(1, 0, false, true)) { std::printf("FAIL quest hospital\n"); ++fail; }
  else std::printf("OK   quest hospital\n");

  const mdaz::WeaponDef* fnx = mdaz::find_weapon(151);
  if (!fnx || fnx->stats.damage != 23.f) {
    std::printf("FAIL FNX dmg want 23 got %g\n", fnx ? fnx->stats.damage : -1.f);
    ++fail;
  } else std::printf("OK   FNX base dmg 23\n");

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

  return fail ? 1 : 0;
}
