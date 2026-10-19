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
