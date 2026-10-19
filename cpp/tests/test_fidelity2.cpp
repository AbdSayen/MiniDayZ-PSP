#include "mdaz/building_interior.hpp"
#include "mdaz/c2_mapgen.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/map_props.hpp"
// Fidelity list (2026-10-19 20:40): building Solid polygons from data.js.
#include "mdaz/building_solid.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

using namespace mdaz;

static int g_fail = 0;
#define CHECK(c, msg) do { if (c) std::printf("  OK %s\n", msg); else { std::printf("  FAIL %s\n", msg); ++g_fail; } } while (0)

static void test_building_polygons() {
  std::printf("[fidelity2] building Solid polygons\n");
  CHECK(building_shape_count() == 35, "35 Solid *_int building types exported from data.js");
  const BuildingShape* s = building_shape_by_int_sprite("b_police_station_int-sheet0");
  CHECK(s != 0, "police interior shape present");
  if (!s) return;
  CHECK(s->c2_type == 291 && s->int_w == 232 && s->int_h == 160 && s->pt_count == 46,
        "police t291 232x160, 46-point polygon (data.js frame)");
  const Vec2 p0 = building_shape_pt(*s, 0);
  CHECK(std::fabs(p0.x - 0.3275856f * 232.f) < 0.05f && std::fabs(p0.y - 0.3218760f * 160.f) < 0.05f,
        "point 0 = data.js fraction * frame size (hotspot relative)");
  const Vec2 o(5000.f, 6000.f);
  CHECK(building_blocks_move(*s, o, Vec2(o.x - 130.f, o.y + 30.f), Vec2(o.x - 90.f, o.y + 30.f), 1.f),
        "walking into the west wall is blocked");
  CHECK(!building_blocks_move(*s, o, Vec2(o.x - 14.f, o.y + 110.f), Vec2(o.x - 14.f, o.y + 60.f), 1.f),
        "walking in through the south door gap is free");
  CHECK(!building_blocks_move(*s, o, Vec2(o.x - 60.f, o.y + 30.f), Vec2(o.x - 60.f, o.y + 31.f), 1.f),
        "room floor is walkable");
  CHECK(building_in_warmzone(*s, o, Vec2(o.x - 60.f, o.y + 30.f)) &&
        !building_in_warmzone(*s, o, Vec2(o.x - 200.f, o.y)),
        "warmzone (w1..w2) marks indoor");
  const BuildingShape* h = building_shape_by_int_sprite("b_hospital_int-sheet0");
  CHECK(h && h->ext_w == 512 && h->ext_h == 404 && std::strcmp(h->ext_sprite, "b_hospital-sheet0") == 0,
        "hospital exterior frame 512x404 paired by name");
  CHECK(building_shape_by_int_sprite("heli_crash-sheet0") == 0, "heli_crash has no Solid");
}

int test_fidelity2() {
  g_fail = 0;
  test_building_polygons();
  return g_fail ? 1 : 0;
}

// ---- fidelity #2 / #3 ----
#include "mdaz/weapons.hpp"
#include "mdaz/zombie.hpp"
#include "mdaz/animal.hpp"

static void test_weapon_table() {
  std::printf("[fidelity2] weapon damage / magazine / reload (data.js)\n");
  const WeaponDef* fnx = find_weapon(151);
  CHECK(fnx && fnx->stats.damage == 23.f && fnx->mag_capacity == 15 && fnx->reload_sec == 2.f,
        "FNX: pistol_bullet IV0 23 (event override), mag 15, Mag_reload 2s");
  const WeaponDef* mosin = find_weapon(156);
  CHECK(mosin && mosin->stats.damage == 90.f && mosin->mag_capacity == 5 &&
        mosin->reload_sec == 4.f && mosin->bullet_speed == 1000.f,
        "Mosin: mosin_bullet IV0 90, mag 5, reload 4s (no fast-mag), speed 1000");
  const WeaponDef* izh = find_weapon(154);
  CHECK(izh && izh->pellets == 5 && izh->stats.damage == 20.f && izh->mag_capacity == 2,
        "IZH-43 (bm16): 5 shotgun_pellet x IV0 20, mag 2");
  const WeaponDef* bizon = find_weapon(183);
  CHECK(bizon && bizon->mag_capacity == 40 && bizon->stats.damage == 16.f && bizon->reload_sec == 3.5f,
        "Bizon: mag 40 (was 64), IV0 16, reload 3.5s");
  const WeaponDef* svd = find_weapon(168);
  CHECK(svd && svd->stats.damage == 80.f && svd->mag_capacity == 10, "SVD: IV0 80, mag 10");
  int placeholder = 0;
  for (int i = 0; i < weapon_def_count(); ++i) {
    const WeaponDef* w = weapon_def_at(i);
    if (!w->stats.is_melee && w->c2_bullet == 0) ++placeholder;
  }
  CHECK(placeholder == 0, "every firearm row is tied to a C2 bullet type");
}

static void test_npc_stats() {
  std::printf("[fidelity2] zed / animal HP and speed (data.js)\n");
  Zombie z;
  z.setup(Zed_Normal, Vec2(0, 0), 7);
  CHECK(z.max_hp == 60.f && z.speed >= 90.f && z.speed <= 100.f, "normal zed: HP 60, speed 90..100");
  z.setup(Zed_Fast, Vec2(0, 0), 8);
  CHECK(z.max_hp == 80.f && z.speed >= 120.f && z.speed <= 140.f, "fast zed: HP 80, speed 120..140");
  z.setup(Zed_Army, Vec2(0, 0), 9);
  CHECK(z.max_hp == 100.f && z.speed >= 90.f && z.speed <= 100.f, "army zed: HP 100, speed 90..100");
  z.setup(Zed_Tank, Vec2(0, 0), 10);
  CHECK(z.max_hp == 1000.f && z.speed == 110.f, "tank: HP 1000, speed 110");
  z.setup(Zed_Screamer, Vec2(0, 0), 11);
  CHECK(z.max_hp == 50.f && z.speed >= 30.f && z.speed <= 40.f, "screamer: HP 50, speed 30..40");
  Animal a;
  a.setup(Animal_Wolf, Vec2(0, 0), 3);
  CHECK(a.hp == 50.f && a.speed >= 90.f && a.speed <= 110.f, "wolf: HP 50, speed 90..110");
  a.setup(Animal_Deer, Vec2(0, 0), 4);
  CHECK(a.hp == 80.f && a.speed >= 110.f && a.speed <= 130.f, "deer: HP 80, speed 110..130");
  a.setup(Animal_Rabbit, Vec2(0, 0), 5);
  CHECK(a.hp == 5.f && a.speed >= 180.f, "rabbit: HP 5, speed 180..200");
  a.setup(Animal_Dog, Vec2(0, 0), 6);
  CHECK(a.hp == 250.f && a.speed == 140.f, "dog: HP 250, speed 140");
}

static void test_interiors() {
  const mdaz::BuildingInterior* pol = mdaz::building_interior_by_int_type(291);
  if (!pol || pol->loot_count != 4) { std::printf("FAIL police interior loot\n"); ++g_fail; }
  else std::printf("OK   police_station_int: 4 spawn_loot_points\n");
  const mdaz::BuildingInterior* hos = mdaz::building_interior_by_int_type(294);
  bool blocked = false;
  if (hos && hos->prop_count > 0) {
    const mdaz::InteriorProp& pr = mdaz::interior_prop(*hos, 0);
    const mdaz::Vec2 o(1000.f, 1000.f), c(o.x + pr.x, o.y + pr.y);
    blocked = mdaz::interior_props_block(*hos, o, mdaz::Vec2(c.x - 60.f, c.y), c, 1.f);
  }
  if (!blocked) { std::printf("FAIL hospital prop Solid\n"); ++g_fail; }
  else std::printf("OK   hospital prop blocks movement (Solid obst_base)\n");
  if (mdaz::building_interior_count() != 36) { std::printf("FAIL interior count\n"); ++g_fail; }
}

static mdaz::C2Map g_tm;
static void test_c2_mapgen() {
  mdaz::Rng r(140);
  mdaz::c2_generate_map(g_tm, r, 0);
  int trees = 0, bld = 0, villages = 0, roads = 0;
  for (int i = 0; i < g_tm.row_count; ++i) { if (g_tm.rows[i].cat == 1) ++trees; if (g_tm.rows[i].cat == 7) ++bld; }
  for (int i = 0; i < 256; ++i) {
    const int t = g_tm.cells[i].type;
    if (t == 9 || t == 4 || t == 16 || t == 17) ++villages;
    if (t == 12 || t == 13 || t == 14) ++roads;
  }
  const bool ok = trees > 1000 && bld > 50 && villages > 0 && roads > 0 && g_tm.hummer_count == 1 &&
                  g_tm.heli_count == 0 && g_tm.dropped_rows == 0;
  std::printf("%s c2 mapgen L0: trees %d buildings %d villages %d road cells %d hummer %d\n",
              ok ? "OK  " : "FAIL", trees, bld, villages, roads, g_tm.hummer_count);
  if (!ok) ++g_fail;
  // Props: every cat 3/4/5/6 + 7:1 row resolves to a data.js object; a fence blocks a crossing.
  static MapProps props;
  mdaz::Rng pr(1);
  props.build(g_tm, pr);
  int want = 0;
  for (int i = 0; i < g_tm.row_count; ++i) {
    const int c = g_tm.rows[i].cat;
    if (c == 6 && g_tm.rows[i].sub == 0) continue;  // no cat-6 sub-0 branch in create_map_element
    if (c == 3 || c == 4 || c == 5 || c == 6 || (c == 7 && g_tm.rows[i].sub == 1)) ++want;
  }
  int blocked = 0;
  for (int i = 0; i < props.count(); ++i) {
    const MapProp& p = props.at(i);
    if (props.blocks(Vec2(p.x, p.y - 200.f), Vec2(p.x, p.y + 4.f), 0.f)) ++blocked;
  }
  const bool pok = props.count() == want && props.solid_count() > 0 && blocked > 0;
  std::printf("%s c2 props: %d/%d rows -> objects, %d with Solid base, %d block a N->S crossing\n",
              pok ? "OK  " : "FAIL", props.count(), want, props.solid_count(), blocked);
  if (!pok) ++g_fail;
}

int test_fidelity2_more() {
  g_fail = 0;
  test_interiors();
  test_c2_mapgen();
  test_weapon_table();
  test_npc_stats();
  return g_fail ? 1 : 0;
}
