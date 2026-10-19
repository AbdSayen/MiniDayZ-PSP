#include "mdaz/zombie.hpp"
#include <string>
#include "mdaz/bandit.hpp"

#include <cstring>
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
  { const mdaz::ItemId tw_[4] = {303, 355, 403, 452}; inv.restore_worn(tw_, 4); } // test: 16 C2 slots
  mdaz::Rng qr(99);
  mdaz::MapGenerator mg;
  mg.generate_new_game(42);
  // Probe at a real POI (layout depends on the seed; the old fixed point (5000,5000)
  // relied on the pre-2026-10-19 compiler-specific draw order).
  mg.reload_locations(mg.location(0).pos, mdaz::LocationReload_Start);
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
  if (mdaz::quest_reward_item_id(3) != 18 || mdaz::quest_reward_item_id(99) != 0) {
    std::printf("FAIL quest_reward_item_id table\n");
    ++fail;
  } else
    std::printf("OK   Quest_spawn_reward_box id map (e.g. 3->18)\n");

  q.generate_quest_item(qr);
  if (q.q_task_id != 1 || q.q_target_sid <= 0 || q.q_reward_id <= 0) {
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
    } else if (!q.awaiting_reward()) {
      std::printf("FAIL quest awaiting_reward after tick\n");
      ++fail;
    } else {
      const int rolled = q.q_reward_id;
      mdaz::LootSystem loot;
      q.spawn_reward_box(loot, mdaz::Vec2(0, 0));
      q.finish_reward();
      std::printf("OK   C2 quest task1 fetch sid %d reward roll %d\n", q.q_target_sid, rolled);
    }
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
  {
    mdaz::BanditSystem bs;
    mdaz::spawn_bandit_camp(bs, mdaz::Vec2(500.f, 500.f), qr);
    int gun = 0, mel = 0;
    for (const mdaz::Bandit* b = bs.begin(); b != bs.end(); ++b) {
      if (!b->alive) continue;
      if (b->has_gun) ++gun;
      else ++mel;
    }
    if (bs.alive_count() != 4 || gun != 2 || mel != 2) {
      std::printf("FAIL spawn_bandit_camp want 4 (2 gun 2 mel) got %d (%d/%d)\n", bs.alive_count(),
                  gun, mel);
      ++fail;
    } else
      std::printf("OK   Spawn_bandit_camp 2 firearm + 2 puncher\n");
    for (const mdaz::Bandit* b = bs.begin(); b != bs.end(); ++b) {
      if (!b->alive || !b->has_gun) continue;
      if (b->mag_rounds != mdaz::bot_mag_capacity_for_weapon_iv14(b->weapon_iv14)) {
        std::printf("FAIL bot IV9 mag want %d got %d iv14=%d\n",
                    mdaz::bot_mag_capacity_for_weapon_iv14(b->weapon_iv14), b->mag_rounds,
                    b->weapon_iv14);
        ++fail;
        break;
      }
    }
    {
      mdaz::Rng sfx_rng;
      if (std::strcmp(mdaz::bot_bandit_fire_sfx(1, 8, sfx_rng), "akm_5") != 0) {
        std::printf("FAIL bot IV11=8 fire sfx\n");
        ++fail;
      } else if (std::strcmp(mdaz::bot_bandit_fire_sfx(2, 4, sfx_rng), "SKS_close_0") != 0) {
        std::printf("FAIL bot IV11=4 fire sfx\n");
        ++fail;
      } else if (mdaz::bot_bandit_iv11_for_iv14(10) != 7) {
        std::printf("FAIL bot IV14=10 -> IV11 7\n");
        ++fail;
      } else {
        std::printf("OK   bot bandit827 fire SFX map\n");
      }
    }
    if (!fail) {
      mdaz::Bandit gun_test;
      gun_test.setup(mdaz::Vec2(0, 0), 99, true, false, 11);
      gun_test.mag_rounds = 0;
      gun_test.reloading = true;
      gun_test.reload_timer = mdaz::bot_reload_duration_sec(11);
      if (gun_test.reload_timer != 2.5f) {
        std::printf("FAIL bot reload IV14=11 want 2.5s\n");
        ++fail;
      } else {
        mdaz::bot_tick_mag_reload(gun_test, 3.f);
        if (!gun_test.reloading && gun_test.mag_rounds == 6)
          std::printf("OK   bot_reloading IV14 mag + wait\n");
        else {
          std::printf("FAIL bot_tick_mag_reload\n");
          ++fail;
        }
      }
    }
  }
  q.reset();
  q.generate_quest_item(qr);
  if (!q.cancel() || q.karma != static_cast<int>(mdaz::balance::karma_points)) {
    std::printf("FAIL quest cancel karma\n");
    ++fail;
  } else
    std::printf("OK   Quest_canceled Karma karma_points\n");
  q.generate_quest_bandits(qr, mg);
  if (q.q_task_id != 2 || !q.map_pin) {
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

  {
    mdaz::LootSystem ls;
    mdaz::Rng lr(42);
    mdaz::LootPile* p = ls.spawn_civilian(mdaz::Vec2(100.f, 100.f), lr);
    if (!p) {
      std::printf("FAIL LOOT_respawn spawn pile\n");
      ++fail;
    } else {
      ls.schedule_respawn(*p);
      if (p->active || p->respawn_t != 3.f) {
        std::printf("FAIL LOOT_respawn stagger want 3s inactive got t=%g active=%d\n",
                    p->respawn_t, p->active ? 1 : 0);
        ++fail;
      } else {
        ls.tick_respawn(3.1f, lr);
        if (!p->active || p->item_count < 1) {
          std::printf("FAIL LOOT_respawn refill after 3s\n");
          ++fail;
        } else
          std::printf("OK   LOOT_respawn 3s pile refill\n");
      }
    }
  }

  {
    mdaz::BanditSystem qb;
    mdaz::Rng qr(9);
    mdaz::spawn_bandit_camp(qb, mdaz::Vec2(200.f, 200.f), qr);
    mdaz::TeamCampSystem nc;
    // Quest camp (controller IV2) is never removed by distance.
    mdaz::bot_bandits_check_situation(qb, nc, mdaz::Vec2(5000.f, 5000.f), 1, true, 1, qr);
    const int kept_quest = qb.alive_count();
    // Q_status 0 -> no removal.
    mdaz::bot_bandits_check_situation(qb, nc, mdaz::Vec2(5000.f, 5000.f), 1, false, 0, qr);
    const int kept_q0 = qb.alive_count();
    // Nearest bandit within 1200 -> kept.
    mdaz::bot_bandits_check_situation(qb, nc, mdaz::Vec2(1200.f, 200.f), 1, false, 1, qr);
    const int kept_near = qb.alive_count();
    // Nearest bandit > 1200 -> Remove_bandit_bots removes all.
    mdaz::bot_bandits_check_situation(qb, nc, mdaz::Vec2(5000.f, 5000.f), 1, false, 1, qr);
    if (kept_quest != 4 || kept_q0 != 4 || kept_near != 4 || qb.alive_count() != 0) {
      std::printf("FAIL bandit removal branch %d %d %d %d\n", kept_quest, kept_q0, kept_near, qb.alive_count());
      ++fail;
    } else
      std::printf("OK   Remove_bandit_bots distance branch (>1200, not quest camp, Q_status!=0)\n");
  }

  {
    mdaz::MapGenerator mg2;
    mdaz::Rng br(12);
    mg2.generate_new_game(12);
    mg2.ensure_bunker_entrance(br, 0);
    bool has0 = false;
    for (int i = 0; i < mg2.location_count(); ++i)
      if (mg2.location(i).type == mdaz::Loc_BunkerEntrance) has0 = true;
    mg2.ensure_bunker_entrance(br, 1);
    bool has1 = false;
    for (int i = 0; i < mg2.location_count(); ++i)
      if (mg2.location(i).type == mdaz::Loc_BunkerEntrance) has1 = true;
    if (has0 || !has1) {
      std::printf("FAIL generate_bunker_entrance level gate has0=%d has1=%d\n",
                  has0 ? 1 : 0, has1 ? 1 : 0);
      ++fail;
    } else
      std::printf("OK   generate_bunker_entrance CurrentLevel>=1\n");
  }

  {
    // C2 pad_list strings: row1 id by task, row3 reward text, time-left format.
    mdaz::ItemDatabase rdb;
    rdb.load_from_json_string("{\"items\":[{\"id\":4,\"name\":\"Apple\"},{\"id\":181,\"name\":\"Gun\"},{\"id\":100,\"name\":\"Ammo\"}]}");
    mdaz::QuestState qs; qs.reset();
    const int r0 = mdaz::quest_task_row_string_id(qs);
    qs.q_task_id = 1; qs.q_status = 1; const int r1 = mdaz::quest_task_row_string_id(qs);
    qs.q_task_id = 2; const int r2 = mdaz::quest_task_row_string_id(qs);
    qs.q_status = 2; const int r2d = mdaz::quest_task_row_string_id(qs);
    qs.q_task_id = 3; qs.q_status = 1; const int r3 = mdaz::quest_task_row_string_id(qs);
    const std::string a = mdaz::quest_reward_text(1, rdb);
    const std::string b = mdaz::quest_reward_text(37, rdb);
    char tl[64]; mdaz::quest_time_left_text(125.f, tl, sizeof(tl));
    const std::string tls = tl;
    if (r0 != 367 || r1 != 371 || r2 != 383 || r2d != 386 || r3 != 382 || a != "Apple x2" ||
        b != "Gun\nAmmo x25" || tls.find("2:5") == std::string::npos) {
      std::printf("FAIL quest pad strings %d %d %d %d %d [%s] [%s] [%s]\n", r0, r1, r2, r2d, r3,
                  a.c_str(), b.c_str(), tl);
      ++fail;
    } else std::printf("OK   quest pad strings (row1 ids, reward text, time left)\n");
  }

  {
    // C2 zed eyes: MainLook 335 cone 180 (facing), ShortLook 70; night needs player light + <=175.
    bool hit = false; float dmg = 0.f;
    mdaz::ZombieSystem zs;
    mdaz::Zombie* za = zs.spawn(mdaz::Zed_Normal, mdaz::Vec2(1000.f, 1000.f)); // faces down (+y)
    za->update_ai(1.1f, mdaz::Vec2(1000.f, 800.f), hit, dmg, 1.f);   // behind, 200 px
    const bool behind = za->c2_aggro;
    za->update_ai(1.1f, mdaz::Vec2(1000.f, 1200.f), hit, dmg, 1.f);  // in front, 200 px
    const bool front = za->c2_aggro;
    mdaz::Zombie* zb = zs.spawn(mdaz::Zed_Normal, mdaz::Vec2(3000.f, 3000.f));
    zb->update_ai(1.1f, mdaz::Vec2(3000.f, 3060.f), hit, dmg, 1.f);  // ShortLook 60 px
    const bool shortl = zb->c2_aggro;
    if (behind || !front || !shortl) {
      std::printf("FAIL zed C2 vision behind=%d front=%d short=%d\n", behind, front, shortl); ++fail;
    } else std::printf("OK   zed C2 vision (cone 180 / 335, short 70)\n");
  }

  return fail ? 1 : 0;
}
