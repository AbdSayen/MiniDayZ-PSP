#include "mdaz/combat.hpp"
#include "mdaz/inventory.hpp"
#include "mdaz/item_db.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/weapons.hpp"
#include "mdaz/zombie.hpp"
#include "mdaz/animal.hpp"

#include <cstdio>

int test_combat() {
  std::printf("--- test_combat ---\n");
  int fail = 0;

  mdaz::set_npc_damage_for_difficulty(mdaz::Difficulty_Regular);
  if (mdaz::zed_melee_damage(mdaz::Zed_Normal) != 9.f) {
    std::printf("FAIL normal zed dmg\n");
    ++fail;
  } else std::printf("OK   normal zed dmg=9\n");

  if (mdaz::wolf_melee_damage() != 10.f) {
    std::printf("FAIL wolf dmg\n");
    ++fail;
  } else std::printf("OK   wolf dmg=10\n");

  mdaz::set_npc_damage_for_difficulty(mdaz::Difficulty_Novice);
  if (mdaz::zed_melee_damage(mdaz::Zed_Normal) != 6.f ||
      mdaz::npc_wolf_damage() != 6.f) {
    std::printf("FAIL novice npc dmg table\n");
    ++fail;
  } else std::printf("OK   novice npc dmg 6/6\n");
  mdaz::set_npc_damage_for_difficulty(mdaz::Difficulty_Veteran);
  if (mdaz::zed_melee_damage(mdaz::Zed_Normal) != 10.f ||
      mdaz::zed_melee_damage(mdaz::Zed_Army) != 15.f) {
    std::printf("FAIL veteran npc dmg table\n");
    ++fail;
  } else std::printf("OK   veteran npc dmg 10/15\n");
  mdaz::set_npc_damage_for_difficulty(mdaz::Difficulty_Regular);

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

  const mdaz::WeaponDef* fnx = mdaz::find_weapon(151);
  if (!fnx || fnx->mag_capacity != 15) {
    std::printf("FAIL FNX mag_capacity want 15\n");
    ++fail;
  } else {
    std::printf("OK   FNX mag capacity 15\n");
  }

  if (!mdaz::weapon_reload_uses_pistol_anim(fnx) ||
      mdaz::weapon_reload_duration_sec(fnx) != 1.f) {
    std::printf("FAIL FNX reload anim/duration\n");
    ++fail;
  } else {
    std::printf("OK   FNX pistol reload 1s\n");
  }

  const mdaz::WeaponDef* m4 = mdaz::find_weapon(158);
  if (!m4 || mdaz::weapon_reload_uses_pistol_anim(m4)) {
    std::printf("FAIL M4 should use rifle reload\n");
    ++fail;
  } else {
    std::printf("OK   M4 rifle reload branch\n");
  }

  const mdaz::WeaponDef* bow = mdaz::find_weapon(163);
  if (!bow || !mdaz::weapon_uses_bow_nock(bow) ||
      mdaz::weapon_bow_nock_sec(bow) != 2.f) {
    std::printf("FAIL bow nock reload\n");
    ++fail;
  } else {
    std::printf("OK   bow Bow_reload nock 2s\n");
  }

  {
    mdaz::ItemDatabase db;
    db.load_from_json_string(
      "{\"items\":[{\"id\":251,\"name\":\"Army helmet\"},{\"id\":400,\"name\":\"Bulletproof vest\"}]}");
    mdaz::Inventory inv;
    inv.clear();
    inv.add_item(251, 1, db);
    inv.add_item(400, 1, db);
    inv.wear_clothing(0, db);
    inv.wear_clothing(1, db);
    const float mul = inv.armor_damage_mul();
    // 0.85 * 0.65 = 0.5525
    if (mul > 0.56f || mul < 0.54f) {
      std::printf("FAIL armor mul got %g\n", mul);
      ++fail;
    } else {
      std::printf("OK   armor mul helmet+vest %.3f\n", mul);
    }
  }

  return fail ? 1 : 0;
}
