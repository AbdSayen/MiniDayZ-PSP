#include "mdaz/c2_session_globals.hpp"
#include "mdaz/inventory.hpp"
#include "mdaz/item_db.hpp"
#include "mdaz/loot.hpp"
#include "mdaz/mapgen.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/survival.hpp"
#include "mdaz/weapons.hpp"

#include <cmath>
#include <cstdio>

int test_c2_session_globals() {
  std::printf("--- test_c2_session_globals ---\n");
  int fail = 0;

  mdaz::C2SessionGlobals g;
  mdaz::Inventory inv;
  mdaz::ItemDatabase db;
  db.load_from_json_file("data/items.json");
  inv.add_item(80, 1, db);
  inv.add_item(76, 1, db);
  inv.add_item(83, 1, db);
  inv.add_item(54, 1, db);
  const mdaz::WeaponDef* ak = mdaz::find_weapon(159);
  if (!ak) {
    std::printf("FAIL find AKM\n");
    return 1;
  }
  mdaz::sync_weapon_attachment_globals(g, inv, ak);
  if (g.Silencer_name_id != 80.0) {
    std::printf("FAIL Silencer_name_id got %g\n", g.Silencer_name_id);
    ++fail;
  } else std::printf("OK   Silencer_name_id\n");
  if (g.Scope_name_id != 76.0) {
    std::printf("FAIL Scope_name_id got %g\n", g.Scope_name_id);
    ++fail;
  } else std::printf("OK   Scope_name_id\n");
  if (g.Belt_name_id != 83.0) {
    std::printf("FAIL Belt_name_id got %g\n", g.Belt_name_id);
    ++fail;
  } else std::printf("OK   Belt_name_id\n");

  mdaz::SurvivalState surv;
  surv.water = 42.f;
  mdaz::sync_water_globals(g, inv, surv);
  if (g.player_water_left != 42.0 || g.water_left != 100.0) {
    std::printf("FAIL water globals %.0f/%.0f\n", g.water_left, g.player_water_left);
    ++fail;
  } else std::printf("OK   water_left / player_water_left\n");

  mdaz::sync_melee_mode(g, ak);
  if (g.Melee_mode != 0.0) {
    std::printf("FAIL Melee_mode firearm\n");
    ++fail;
  }
  const mdaz::WeaponDef* knife = mdaz::find_weapon(42);
  mdaz::sync_melee_mode(g, knife);
  if (g.Melee_mode != 1.0) {
    std::printf("FAIL Melee_mode knife\n");
    ++fail;
  } else std::printf("OK   Melee_mode\n");

  mdaz::MapGenerator mg;
  g.Loop_secrets = 0;
  mg.set_c2_scratch(&g);
  mg.generate_new_game(99);
  if (g.Loop_secrets < 1.0) {
    std::printf("FAIL mapgen hotspots Loop_secrets=%g\n", g.Loop_secrets);
    ++fail;
  } else std::printf("OK   mapgen hotspot / Loop_secrets\n");

  if (g.anotherX < 0 && g.anotherX2 < 0) {
    std::printf("FAIL anotherX pair not set\n");
    ++fail;
  } else std::printf("OK   anotherX/anotherX2 road scratch\n");

  const float rad = mdaz::compute_rad_dist(mdaz::Vec2(5000.f, 5000.f), mg);
  if (rad <= 0.f || !std::isfinite(rad)) {
    std::printf("FAIL rad_dist\n");
    ++fail;
  } else std::printf("OK   rad_dist %.0f\n", rad);

  mdaz::LootSystem loot;
  mdaz::Rng rng(1);
  loot.spawn_civilian(mdaz::Vec2(10.f, 0.f), rng);
  mdaz::sync_pickpile_globals(g, loot, mdaz::Vec2(0.f, 0.f), inv, 50.f);
  if (g.avalible_interaction != 1.0 || g.pickpile_index < 0.0) {
    std::printf("FAIL pickpile globals\n");
    ++fail;
  } else std::printf("OK   pickpile_index / avalible_interaction\n");

  mdaz::c2_stage_craft_removal(g, 38, 5);
  if (g.item_for_remove_id != 38.0 || g.required_count != 5.0) {
    std::printf("FAIL craft removal globals\n");
    ++fail;
  } else std::printf("OK   item_for_remove_id / required_count\n");

  return fail ? 1 : 0;
}
