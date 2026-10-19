#include "mdaz/combat.hpp"
#include "mdaz/inventory.hpp"
#include "mdaz/item_db.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/weapons.hpp"
#include "mdaz/zombie.hpp"
#include "mdaz/animal.hpp"

#include <cstdio>
#include <cstring>

int test_combat() {
  std::printf("--- test_combat ---\n");
  int fail = 0;

  mdaz::set_npc_damage_for_difficulty(mdaz::Difficulty_Regular);
  if (mdaz::zed_melee_damage(mdaz::Zed_Normal) != 9.f) {
    std::printf("FAIL normal zed dmg\n");
    ++fail;
  } else std::printf("OK   normal zed dmg=9\n");

  if (mdaz::wolf_melee_damage() != 9.f) {
    std::printf("FAIL wolf player dmg want 9 (Zed_dmg) got %g\n", mdaz::wolf_melee_damage());
    ++fail;
  } else std::printf("OK   wolf player dmg=9 (Zed_dmg)\n");

  if (mdaz::wolf_attack_melee_range() != 50.f ||
      mdaz::wolf_attack_cooldown_sec() != 1.5f) {
    std::printf("FAIL wolf attack range/cooldown\n");
    ++fail;
  } else {
    std::printf("OK   wolf attack range 50 cooldown 1.5s (battack timer, regular)\n");
  }

  mdaz::set_npc_damage_for_difficulty(mdaz::Difficulty_Novice);
  if (mdaz::zed_melee_damage(mdaz::Zed_Normal) != 6.f ||
      mdaz::wolf_melee_damage() != 6.f ||
      mdaz::npc_wolf_damage() != 6.f) {
    std::printf("FAIL novice npc dmg table\n");
    ++fail;
  } else std::printf("OK   novice npc dmg 6/6/6\n");
  mdaz::set_npc_damage_for_difficulty(mdaz::Difficulty_Veteran);
  if (mdaz::zed_melee_damage(mdaz::Zed_Normal) != 10.f ||
      mdaz::zed_melee_damage(mdaz::Zed_Army) != 15.f) {
    std::printf("FAIL veteran npc dmg table\n");
    ++fail;
  } else std::printf("OK   veteran npc dmg 10/15\n");
  mdaz::set_npc_damage_for_difficulty(mdaz::Difficulty_Regular);

  {
    float hp = 40.f;
    const float applied = mdaz::npc_apply_damage(hp, mdaz::NpcDmg_Zed, 15.f);
    if (applied != 15.f || hp != 25.f) {
      std::printf("FAIL npc_apply_damage want 15/25 got %g/%g\n", applied, hp);
      ++fail;
    } else {
      std::printf("OK   npc_apply_damage subtract IV\n");
    }
    if (mdaz::npc_damage_kind_for_zed(mdaz::Zed_Tank) != mdaz::NpcDmg_ZedTank ||
        mdaz::npc_damage_kind_for_animal(mdaz::Animal_Wolf) != mdaz::NpcDmg_Wolf) {
      std::printf("FAIL npc damage kind mapping\n");
      ++fail;
    } else {
      std::printf("OK   npc_get_damage kind map\n");
    }
    if (mdaz::npc_health_bar_width(15.f, 30.f, mdaz::NpcDmg_Zed) != 15.f) {
      std::printf("FAIL npc health bar width\n");
      ++fail;
    } else {
      std::printf("OK   npc health bar min(30,hp)\n");
    }
  }

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
      mdaz::weapon_reload_duration_sec(fnx) != 2.f) { // data.js Reload_* FNX: Mag_reload(2, 0)
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

  {
    mdaz::Rng sfx_rng(7);
    const char* fal = mdaz::weapon_fire_sfx_tag(175, sfx_rng);
    if (!fal || (std::strcmp(fal, "fn_fal_single_1") && std::strcmp(fal, "fn_fal_single_2"))) {
      std::printf("FAIL FN-CAL sfx want fn_fal_single_* got %s\n", fal ? fal : "(null)");
      ++fail;
    } else {
      std::printf("OK   FN-CAL fire sfx %s\n", fal);
    }
    const char* m4s = mdaz::weapon_fire_sfx_tag(158, sfx_rng);
    if (!m4s || (std::strcmp(m4s, "m4_single_1") && std::strcmp(m4s, "m4_single_2"))) {
      std::printf("FAIL M4 fire sfx\n");
      ++fail;
    } else {
      std::printf("OK   M4 fire sfx %s\n", m4s);
    }
    const char* zed = mdaz::pick_zed_player_hit_sfx(sfx_rng);
    if (!zed || std::strncmp(zed, "attack_", 7) != 0) {
      std::printf("FAIL zed hit sfx pool\n");
      ++fail;
    } else {
      std::printf("OK   zed hit sfx %s\n", zed);
    }
    if (std::strcmp(mdaz::c2_zed_screamer_attack_sfx(), "attack_8") != 0) {
      std::printf("FAIL screamer sfx\n");
      ++fail;
    } else {
      std::printf("OK   screamer attack_8\n");
    }
    mdaz::touch_combat_c2_audit_anchors();
    mdaz::touch_weapon_c2_audit_anchors();
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
    if (mdaz::player_damage_after_armor(40.f, mdaz::Inventory(), mdaz::PlayerDmg_Bullet) != 40.f) {
      std::printf("FAIL bullet path should skip vest mul\n");
      ++fail;
    } else {
      std::printf("OK   Player_get_hit bullet skips armor mul\n");
    }
    if (!mdaz::weapon_bullet_iv8_armor_pierce(156) ||
        mdaz::weapon_bullet_iv8_armor_pierce(158)) {
      std::printf("FAIL IV8 weapon map mosin/m4\n");
      ++fail;
    } else {
      std::printf("OK   Bullets_dmg_deal IV8 weapon ids\n");
    }
    mdaz::Rng prng(99);
    float zhp = 100.f;
    const float once = mdaz::zed_apply_bullet_hit_damage(
      zhp, mdaz::NpcDmg_Zed, 80.f, mdaz::Zed_Normal, true, prng);
    if (once != 80.f || zhp != 20.f) {
      std::printf("FAIL IV8 ignored on normal zed\n");
      ++fail;
    } else {
      std::printf("OK   IV8 no double on normal zed\n");
    }
    int doubles = 0;
    for (int t = 0; t < 300; ++t) {
      zhp = 200.f;
      mdaz::zed_apply_bullet_hit_damage(zhp, mdaz::NpcDmg_Zed, 80.f, mdaz::Zed_Army, true, prng);
      if (zhp == 40.f) ++doubles; // 200-80-80 vs 200-80=120
    }
    if (doubles < 35 || doubles > 110) {
      std::printf("FAIL IV8 army double rate %d/300 (want ~20%% for dmg 80)\n", doubles);
      ++fail;
    } else {
      std::printf("OK   IV8 army double-hit proc ~%d/300\n", doubles);
    }
  }

  {
    mdaz::ItemDatabase db;
    db.load_from_json_string(
      "{\"items\":[{\"id\":251,\"name\":\"Army helmet\"},{\"id\":400,\"name\":\"Bulletproof vest\"}]}");
    mdaz::Inventory inv;
    inv.clear();
    inv.add_item(251, 1, db);
    inv.wear_clothing(0, db);  // C2: equipping frees the bag slot
    inv.add_item(400, 1, db);
    inv.wear_clothing(0, db);
    const float mul = inv.armor_damage_mul();
    // 0.85 * 0.65 = 0.5525
    if (mul > 0.56f || mul < 0.54f) {
      std::printf("FAIL armor mul got %g\n", mul);
      ++fail;
    } else {
      std::printf("OK   armor mul helmet+vest %.3f\n", mul);
    }
    mdaz::Rng wear_rng(99);
    const float helm_before = inv.worn_iv6(0);
    for (int w = 0; w < 40 && inv.worn_iv6(0) >= helm_before - 0.01f; ++w)
      inv.apply_hit_armor_wear(wear_rng, true, false, false);
    if (inv.worn_iv6(0) >= helm_before - 0.01f) {
      std::printf("FAIL bullet IV6 wear on worn gear\n");
      ++fail;
    } else {
      std::printf("OK   bullet IV6 wear (helmet slot)\n");
    }
    mdaz::Rng def_rng(42);
    int blocks = 0;
    for (int t = 0; t < 500; ++t) {
      if (mdaz::roll_player_melee_def_block(def_rng, mdaz::MeleeDef_Heavy, 0.f)) ++blocks;
    }
    if (blocks < 80 || blocks > 220) {
      std::printf("FAIL melee def_percent block rate %d/500\n", blocks);
      ++fail;
    } else {
      std::printf("OK   melee def_percent blocks ~%d/500\n", blocks);
    }
  }

  {
    mdaz::DmgHintSystem hints;
    hints.spawn(mdaz::Vec2(10.f, 20.f), mdaz::Vec2(100.f, 20.f), 23, false);
    hints.spawn(mdaz::Vec2(0.f, 0.f), mdaz::Vec2(0.f, -10.f), 40, true);
    if (hints.count() < 2) {
      std::printf("FAIL dmg_hint spawn count %d\n", hints.count());
      ++fail;
    } else {
      const mdaz::DmgHint* a = hints.begin();
      const mdaz::DmgHint* b = hints.begin() + 1;
      if (!a->active || a->amount != 23 || a->crit ||
          !b->active || b->amount != 40 || !b->crit) {
        std::printf("FAIL dmg_hint fields\n");
        ++fail;
      } else {
        hints.update(0.5f);
        if (!a->active || a->life <= 0.f) {
          std::printf("FAIL dmg_hint lifetime\n");
          ++fail;
        } else {
          std::printf("OK   dmg_hint spawn/update (C2 floating -N)\n");
        }
      }
    }
  }

  return fail ? 1 : 0;
}
