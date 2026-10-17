#include "mdaz/bandit.hpp"
#include "mdaz/bullet.hpp"
#include "mdaz/campfire.hpp"
#include "mdaz/combat.hpp"
#include "mdaz/inventory.hpp"
#include "mdaz/item_db.hpp"
#include "mdaz/loot.hpp"
#include "mdaz/mapgen.hpp"
#include "mdaz/quest.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/balance_globals.hpp"
#include "mdaz/types.hpp"
#include "mdaz/vehicle.hpp"
#include "mdaz/weapons.hpp"

#include <cmath>
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
  else if (v->fuel != mdaz::vehicle_balance::kFuelMax ||
           v->hp != mdaz::vehicle_balance::kHpMax) {
    std::printf("FAIL vehicle spawn fuel/hp %.0f/%.0f\n", v->fuel, v->hp);
    ++fail;
  } else {
    const float fuel0 = v->fuel;
    vs.update(0.1f, pos, 1.f, 0.f, true); // mount
    vs.update(0.1f, pos, 1.f, 0.f, false);
    if (!vs.player_driving()) { std::printf("FAIL drive\n"); ++fail; }
    else if (v->fuel >= fuel0) {
      std::printf("FAIL vehicle fuel drain (C2 -8/0.1s)\n");
      ++fail;
    } else std::printf("OK   vehicle mount/drive/fuel tick\n");
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
    vs.on_destroyed(*v);
    if (v->active || v->respawn_t <= 0.f) {
      std::printf("FAIL Uaz_respawn timer\n");
      ++fail;
    } else std::printf("OK   Uaz_respawn delay %.0fs\n", v->respawn_t);
  }

  mdaz::ItemDatabase db;
  db.load_from_json_file("data/items.json");
  mdaz::Inventory inv;
  mdaz::Rng qr(99);
  mdaz::MapGenerator mg;
  mg.generate_new_game(42);
  mg.reload_locations(mdaz::Vec2(5000.f, 5000.f), mdaz::LocationReload_Start);
  int active = 0;
  for (int i = 0; i < mg.location_count(); ++i)
    if (mg.location(i).active) ++active;
  if (active < 1) {
    std::printf("FAIL Reload_locations_start activated none\n");
    ++fail;
  } else {
    std::printf("OK   Reload_locations_start %d POIs active\n", active);
  }
  const mdaz::Vec2 map_mid(8925.f, 10000.f);
  mg.reload_locations(map_mid, mdaz::LocationReload_Runtime);
  int near = 0, far_inactive = 0;
  for (int i = 0; i < mg.location_count(); ++i) {
    const mdaz::MapLocation& L = mg.location(i);
    const float dx = L.pos.x - map_mid.x;
    const float dy = L.pos.y - map_mid.y;
    const float d = std::sqrt(dx * dx + dy * dy);
    if (d <= static_cast<float>(mdaz::balance::respawn_radius2) && L.active) ++near;
    if (d > static_cast<float>(mdaz::balance::respawn_radius2) && !L.active) ++far_inactive;
  }
  if (near < 1) {
    std::printf("FAIL Reload_locations near load\n");
    ++fail;
  } else
    std::printf("OK   Reload_locations runtime near=%d far_off=%d\n", near, far_inactive);

  mdaz::QuestState q;
  q.generate_quest_item(qr);
  if (q.q_task_id != 1 || q.q_target_sid <= 0) {
    std::printf("FAIL Generate_quest_item task/target\n");
    ++fail;
  } else if (q.tick(inv, db)) {
    std::printf("FAIL quest completed without item\n");
    ++fail;
  } else {
    inv.add_item(static_cast<mdaz::ItemId>(q.q_target_sid), 1, db);
    if (!q.tick(inv, db)) {
      std::printf("FAIL quest fetch item\n");
      ++fail;
    } else
      std::printf("OK   C2 quest task1 fetch sid %d\n", q.q_target_sid);
  }
  q.generate_quest_case();
  if (q.q_task_id != 3 || q.q_target_sid != 88) {
    std::printf("FAIL Generate_quest_case\n");
    ++fail;
  } else {
    inv.add_item(88, 1, db);
    if (!q.tick(inv, db, 0))
      std::printf("FAIL quest case item 88\n");
    else
      std::printf("OK   quest task3 case sid 88\n");
  }
  q.reset();
  q.generate_quest_bandits(qr, mg);
  if (q.q_task_id != 2) {
    std::printf("FAIL Generate_quest_bandits\n");
    ++fail;
  } else if (!q.tick(inv, db, 1)) {
    if (!q.tick(inv, db, 0))
      std::printf("FAIL quest bandits clear\n");
    else
      std::printf("OK   quest task2 bandits cleared\n");
  } else
    std::printf("FAIL quest bandits tick with hostiles\n");

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

  // generate_array_roads_1/_2: paired POI sectors on a row produce road markers.
  {
    mdaz::MapGenerator roads;
    roads.clear();
    roads.generate_new_game(777);
    if (roads.road_sector_count() < 1) {
      std::printf("FAIL generate_array_roads (no road sectors seed 777)\n");
      ++fail;
    } else {
      std::printf("OK   generate_array_roads sectors=%d\n", roads.road_sector_count());
    }
  }

  return fail ? 1 : 0;
}
