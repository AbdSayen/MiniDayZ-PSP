#include "mdaz/combat.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/zombie.hpp"
#include "mdaz/animal.hpp"

#include <cstdio>

int test_combat() {
  std::printf("--- test_combat ---\n");
  int fail = 0;

  if (mdaz::zed_melee_damage(mdaz::Zed_Normal) != 9.f) {
    std::printf("FAIL normal zed dmg\n");
    ++fail;
  } else std::printf("OK   normal zed dmg=9\n");

  if (mdaz::wolf_melee_damage() != 10.f) {
    std::printf("FAIL wolf dmg\n");
    ++fail;
  } else std::printf("OK   wolf dmg=10\n");

  mdaz::DispersionState disp;
  disp.reset_from_globals();
  disp.on_shot(false);
  if (disp.angle != 5.f) {
    std::printf("FAIL dispersion per shot want 5 got %g\n", disp.angle);
    ++fail;
  } else std::printf("OK   dispersion per shot\n");

  disp.on_shot(true);
  if (disp.angle != 20.f) { // 5 + 5 + 10 run
    std::printf("FAIL dispersion run want 20 got %g\n", disp.angle);
    ++fail;
  } else std::printf("OK   dispersion run penalty\n");

  mdaz::ZombieSystem zs;
  mdaz::Zombie* z = zs.spawn(mdaz::Zed_Normal, mdaz::Vec2(0, 0));
  if (!z || !z->alive) {
    std::printf("FAIL spawn zed\n");
    ++fail;
  } else {
    float dmg = 0.f;
    zs.update(0.016f, mdaz::Vec2(10, 0), dmg); // in melee range
    if (dmg < 9.f) {
      std::printf("FAIL zed should hit for 9, got %g\n", dmg);
      ++fail;
    } else std::printf("OK   zed melee hit %.0f\n", dmg);
  }

  mdaz::Rng rng(1);
  float sum = 0.f;
  float mn = 1e9f, mx = -1e9f;
  for (int i = 0; i < 200; ++i) {
    float d = mdaz::roll_bullet_damage(40.f, rng);
    sum += d;
    if (d < mn) mn = d;
    if (d > mx) mx = d;
  }
  // ±25% of 40 => [30, 50]
  if (mn < 30.f || mx > 50.f) {
    std::printf("FAIL bullet roll range %g..%g\n", mn, mx);
    ++fail;
  } else {
    std::printf("OK   bullet dmg variance 40 -> [%g..%g] avg %.1f\n", mn, mx, sum / 200.f);
  }

  if (mdaz::default_melee_stats().range != 45.f) {
    std::printf("FAIL fist range\n");
    ++fail;
  } else {
    std::printf("OK   fist melee range 45\n");
  }

  if (mdaz::default_melee_stats().damage != 10.f) {
    std::printf("FAIL fist dmg want 10 got %g\n", mdaz::default_melee_stats().damage);
    ++fail;
  } else {
    std::printf("OK   fist melee dmg 10\n");
  }

  return fail ? 1 : 0;
}
