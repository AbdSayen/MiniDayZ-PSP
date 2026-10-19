#include "mdaz/bandit.hpp"
#include "mdaz/bunker.hpp"
#include "mdaz/create_map_element_decode.hpp"
#include "mdaz/map_decode_tables.hpp"
#include "mdaz/mapgen.hpp"
#include "mdaz/menu_camera.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/world_tilemaps.hpp"

#include <cstdio>

int test_map_fidelity() {
  std::printf("--- test_map_fidelity ---\n");
  int fail = 0;

  if (mdaz::kC2PoiPairSpacing != 300.f || mdaz::kC2PoiPairHalfSpacing != 150.f) {
    std::printf("FAIL POI pair spacing (want 300/150 from create_map_location)\n");
    ++fail;
  } else {
    std::printf("OK   create_map_location POI spacing 300\n");
  }

  if (mdaz::kC2CropGrowSeconds != 20.f) {
    std::printf("FAIL crop grow timer (want 20s from item_activate)\n");
    ++fail;
  } else {
    std::printf("OK   crop grow timer 20s\n");
  }

  {
    mdaz::Rng rng(99);
    mdaz::MenuCamera cam;
    cam.reset(rng);
    const mdaz::Vec2 p0 = cam.position();
    for (int i = 0; i < 120; ++i) cam.update(1.f / 60.f, rng);
    const float dx = cam.position().x - p0.x;
    const float dy = cam.position().y - p0.y;
    if (dx * dx + dy * dy < 1.f) {
      std::printf("FAIL menu camera did not move toward pin\n");
      ++fail;
    } else {
      std::printf("OK   menu camera_roll_menu chase (approx)\n");
    }
  }

  if (mdaz::kForestStampCount < 50) {
    std::printf("FAIL forest stamp table sparse (%d)\n", mdaz::kForestStampCount);
    ++fail;
  } else {
    std::printf("OK   generate_location forest stamps %d\n", mdaz::kForestStampCount);
  }

  if (mdaz::kBunkerZedSpawnerCount < 10) {
    std::printf("FAIL bunker zed spawner table (%d)\n", mdaz::kBunkerZedSpawnerCount);
    ++fail;
  } else {
    std::printf("OK   bunker_zed_spawner XY count %d\n", mdaz::kBunkerZedSpawnerCount);
  }

  mdaz::BunkerMap map;
  const char* paths[] = {"data/bunker_json", "cpp/data/bunker_json", "cpp/build/data/bunker_json", 0};
  for (int i = 0; paths[i]; ++i) {
    if (map.load_from_file(paths[i])) break;
  }
  if (map.width == 225 && map.height == 74) {
    int walk = 0;
    for (int i = 0; i < mdaz::kBunkerZedSpawnerCount; ++i) {
      if (!map.is_blocked_world(mdaz::kBunkerZedSpawners[i].x, mdaz::kBunkerZedSpawners[i].y))
        ++walk;
    }
    std::printf("OK   spawner walkable %d/%d (modulo layout approx)\n", walk,
                mdaz::kBunkerZedSpawnerCount);
    if (walk < mdaz::kBunkerZedSpawnerCount / 3) {
      std::printf("FAIL too few walkable spawner tiles\n");
      ++fail;
    }
  } else {
    std::printf("NOTE bunker RLE not loaded; skip spawner walk test\n");
  }

  if (mdaz::map_element_iv10_for_zed_resp_tile(0) != 1 ||
      mdaz::map_element_iv10_for_zed_resp_tile(1) != 2 ||
      mdaz::map_element_iv10_for_zed_resp_tile(2) != 3 ||
      mdaz::map_element_iv10_for_zed_resp_tile(6) != 6 ||
      mdaz::map_element_iv10_for_zed_resp_tile(99) != 0) {
    std::printf("FAIL create_map_element zed_resp tile → IV10 table\n");
    ++fail;
  } else {
    std::printf("OK   create_map_element zed_resp tile → IV10 (%d rows)\n",
                mdaz::kMapElementIv10Count);
  }

  if (!mdaz::team_camp_iv0_allows_bandit_camp(9) || !mdaz::team_camp_iv0_allows_bandit_camp(21) ||
      mdaz::team_camp_iv0_allows_bandit_camp(3)) {
    std::printf("FAIL Spawn_bandit_camp IV0 filter\n");
    ++fail;
  } else {
    std::printf("OK   Spawn_bandit_camp IV0 filter\n");
  }

  mdaz::TeamCampSystem camps;
  mdaz::BanditSystem bandits;
  mdaz::Rng rng(42);
  camps.spawn_team_camp(mdaz::Vec2(1000.f, 1000.f), 9);
  mdaz::bot_bandits_check_situation(bandits, camps, mdaz::Vec2(500.f, 500.f), 0, rng);
  if (bandits.alive_count() != 4) {
    std::printf("FAIL bot_bandits_check_situation spawn got %d\n", bandits.alive_count());
    ++fail;
  } else {
    std::printf("OK   Bot_bandits_check_situation camp spawn\n");
  }

  {
    // tilemap_forest is Solid in C2 with per-tile collision polygons (data.js t202[13]).
    mdaz::WorldTilemaps* wt = new mdaz::WorldTilemaps();
    wt->stamp_fill("tilemap_forest", 0.f, 0.f, 1, 1, 4, 150, 150, 3);   // full-box tile
    wt->stamp_fill("tilemap_forest", 150.f, 0.f, 1, 1, 1, 150, 150, 3); // bottom band y>=0.667
    const bool a = wt->forest_solid_at(75.f, 75.f);
    const bool b = wt->forest_solid_at(225.f, 50.f);
    const bool c = wt->forest_solid_at(225.f, 130.f);
    const bool d = wt->forest_solid_at(400.f, 75.f);
    if (!a || b || !c || d) {
      std::printf("FAIL forest_solid_at a=%d b=%d c=%d d=%d\n", a, b, c, d);
      ++fail;
    } else {
      std::printf("OK   tilemap_forest Solid per-tile polygons\n");
    }
    delete wt;
  }

  return fail ? 1 : 0;
}
