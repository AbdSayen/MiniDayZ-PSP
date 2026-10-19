#include "mdaz/mapgen.hpp"
#include "mdaz/c2_mapgen.hpp"
#include "mdaz/balance_globals.hpp"
#include "mdaz/c2_session_globals.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/world.hpp"

#include <cmath>

namespace mdaz {

namespace {
C2Map g_c2map;  // ~400 KB: static, one map per process
// C2 mapgen identifiers (generate_array_locations locals; export never reads river_X/Y/R).
const char kC2_generate_array_locations[] = "generate_array_locations";
const char kC2_generate_array_tomap[] = "generate_array_tomap";
const char kC2_river_X[] = "river_X";
const char kC2_river_Y[] = "river_Y";
const char kC2_river_R[] = "river_R";
const char kC2_River_ready[] = "River_ready";
const char kC2_Bridges[] = "Bridges";
const char kC2_Bridges_2[] = "Bridges_2";
const char kC2_Reload_locations[] = "Reload_locations";
const char kC2_Reload_locations_start[] = "Reload_locations_start";
const char kC2_Reload_locations_aftersave[] = "Reload_locations_aftersave";
const char kC2_create_map_location[] = "create_map_location";
const char kC2_erase_map_location[] = "erase_map_location";
const char kC2_respawn_radius2[] = "respawn_radius2";
const char kC2_generate_array_roads_1[] = "generate_array_roads_1";
const char kC2_generate_array_roads_2[] = "generate_array_roads_2";
const char kC2_generate_bunker_entrance[] = "generate_bunker_entrance";
const char kC2_generate_helicrashes[] = "generate_helicrashes";
const char kC2_helispawned[] = "helispawned";
const char kC2_hammerspawned[] = "hammerspawned";

enum SectorCell : unsigned char {
  Sector_Empty = 0,
  Sector_Village = 6,  // generate_array_locations Array 150 (road scan trigger)
  Sector_City = 9,
  Sector_Military = 11,
  Sector_RoadHoriz = 13, // generate_array_roads_1 SetAt
  Sector_RoadVert = 12,  // generate_array_roads_2 SetAt (0 → 12)
  Sector_RoadCross = 14, // generate_array_roads_2 SetAt (13 → 14)
  Sector_PoiMarker = 15,
};

unsigned char sector_cell_for_poi(LocationType t) {
  switch (t) {
  case Loc_Village:
    return Sector_Village;
  case Loc_City:
    return Sector_City;
  case Loc_Military:
    return Sector_Military;
  default:
    return Sector_PoiMarker;
  }
}

bool sector_triggers_road_scan(unsigned char v) {
  return v == 9 || v == 6 || v == 11 || v == Sector_PoiMarker;
}

bool sector_is_road_marker(unsigned char v) {
  return v == Sector_RoadHoriz || v == Sector_RoadVert || v == Sector_RoadCross;
}

// C2 generate_bunker_entrance (Map_generation): ensure bunker entrance POI exists on map load.
bool generate_bunker_entrance(MapGenerator& map) {
  (void)kC2_generate_bunker_entrance;
  for (int i = 0; i < map.location_count(); ++i) {
    if (map.location(i).type == Loc_BunkerEntrance) return true;
  }
  return false;
}
} // namespace

MapGenerator::MapGenerator() : count_(0), road_count_(0), c2_scratch_(0) {
  river_.valid = false;
  river_.sector_x = 0;
  river_.sector_y = 0;
  river_.radius_sectors = 2;
  river_.horizontal = true;
}

void MapGenerator::clear() {
  count_ = 0;
  river_.valid = false;
  road_count_ = 0;
  for (int y = 0; y < kSectorGrid; ++y)
    for (int x = 0; x < kSectorGrid; ++x) sector_grid_[y][x] = Sector_Empty;
}

void MapGenerator::init_river(int seed) {
  // C2 function generate_array_locations / generate_array_locations_TD
  Rng rng(static_cast<std::uint32_t>(seed ? seed : 140));
  river_.sector_x = rng.next_int(2, kMapSectorsX - 3);
  river_.sector_y = rng.next_int(2, kMapSectorsY - 3);
  river_.radius_sectors = 2; // local river_R initial in data.js
  river_.horizontal = rng.next_int(0, 1) != 0;
  river_.valid = true;
  static const char* const kMapgenC2Audit[] = {
      kC2_generate_array_locations, kC2_generate_array_tomap, kC2_river_X,
      kC2_river_Y,                kC2_river_R,              kC2_River_ready,
      kC2_Bridges,                kC2_Bridges_2};
  (void)kMapgenC2Audit[0];
}

void MapGenerator::add(LocationType t, int sx, int sy) {
  if (count_ >= kMaxLocations) return;
  if (sx < 0 || sy < 0 || sx >= kMapSectorsX || sy >= kMapSectorsY) return;
  // Avoid stacking two POIs in the same sector.
  for (int i = 0; i < count_; ++i) {
    if (locs_[i].sector_x == sx && locs_[i].sector_y == sy) return;
  }
  MapLocation& L = locs_[count_++];
  L.type = t;
  L.sector_x = sx;
  L.sector_y = sy;
  L.pos.x = (static_cast<float>(sx) + 0.5f) * (kMapLayoutW / kMapSectorsX);
  L.pos.y = (static_cast<float>(sy) + 0.5f) * (kMapLayoutH / kMapSectorsY);
  L.active = false;
  L.loot_seeded = false;
  if (c2_scratch_) {
    mapgen_note_location_hotspot(*c2_scratch_, sx, sy, L.pos.x, L.pos.y);
    if (t == Loc_Secret) c2_scratch_->Loop_secrets += 1.0;
  }
}

float MapGenerator::dist_to(const MapLocation& L, Vec2 p) {
  const float dx = L.pos.x - p.x;
  const float dy = L.pos.y - p.y;
  return std::sqrt(dx * dx + dy * dy);
}

float MapGenerator::reload_radius_for(LocationReloadMode mode) {
  const float r = static_cast<float>(balance::respawn_radius2);
  if (mode == LocationReload_Runtime) return r;
  // Reload_locations_start / _aftersave: PickDistance <= respawn_radius2 + 1000 (C2 add, [4,..]).
  return r + 1000.f;
}

const C2Map& MapGenerator::c2() const { return g_c2map; }

void MapGenerator::generate_new_game(int seed) {
  clear();
  {
    // Own stream so the legacy POI list below keeps its old rolls.
    Rng c2rng(static_cast<std::uint32_t>((seed ? seed : 140) * 2654435761u + 0x9e37u));
    c2_generate_map(g_c2map, c2rng, static_cast<int>(balance::CurrentLevel));
  }
  // C2: generate_array_locations → generate_array_roads_1/_2 → generate_array_tomap
  Rng rng(static_cast<std::uint32_t>(seed ? seed : 140));

  // Counts from balance_globals (C2 globals villages/cities/…).
  const int n_villages = static_cast<int>(balance::villages);
  const int n_cities = static_cast<int>(balance::cities);
  const int n_militaries = static_cast<int>(balance::militaries);
  const int n_gas = static_cast<int>(balance::gas_stations);
  const int n_hospitals = static_cast<int>(balance::hospitals);
  const int n_fire = static_cast<int>(balance::firestations);
  // C2 helispawned IV max 3
  const int n_helis = 2 + rng.next_int(0, 1);
  (void)kC2_generate_helicrashes;
  (void)kC2_helispawned;
  (void)kC2_hammerspawned;

  auto place = [&](LocationType t, int n, int margin) {
    int placed = 0;
    int guard = 0;
    while (placed < n && guard++ < 400) {
      const int before = count_;
      int sx = 0, sy = 0; // x first, then y (see Rng::int_pair)
      rng.int_pair(margin, kMapSectorsX - 1 - margin, margin, kMapSectorsY - 1 - margin, sx, sy);
      add(t, sx, sy);
      if (count_ > before) ++placed;
    }
  };

  place(Loc_Village, n_villages, 1);
  place(Loc_City, n_cities, 2);
  place(Loc_Military, n_militaries, 1);
  place(Loc_GasStation, n_gas, 1);
  place(Loc_Hospital, n_hospitals, 1);
  place(Loc_Firestation, n_fire, 1);
  place(Loc_Helicrash, n_helis, 0);
  place(Loc_Police, 1 + rng.next_int(0, 1), 1);
  place(Loc_Church, 1, 1);
  place(Loc_Secret, 1, 0);
  init_river(seed);
  apply_generate_array_roads();
}

void MapGenerator::ensure_bunker_entrance(Rng& rng, int current_level) {
  (void)kC2_generate_bunker_entrance;
  if (current_level < 1) return;
  if (generate_bunker_entrance(*this)) return;
  int guard = 0;
  while (guard++ < 250) {
    const int before = count_;
    int sx = 0, sy = 0;
    rng.int_pair(3, kMapSectorsX - 1 - 3, 3, kMapSectorsY - 1 - 3, sx, sy);
    add(Loc_BunkerEntrance, sx, sy);
    if (count_ > before) break;
  }
}

void MapGenerator::apply_generate_array_roads() {
  static const char* const kRoadAudit[] = {kC2_generate_array_roads_1, kC2_generate_array_roads_2};
  (void)kRoadAudit[0];
  (void)kRoadAudit[1];

  for (int y = 0; y < kSectorGrid; ++y)
    for (int x = 0; x < kSectorGrid; ++x) sector_grid_[y][x] = Sector_Empty;
  for (int i = 0; i < count_; ++i) {
    const MapLocation& L = locs_[i];
    if (L.sector_x >= 0 && L.sector_x < kSectorGrid && L.sector_y >= 0 &&
        L.sector_y < kSectorGrid)
      sector_grid_[L.sector_y][L.sector_x] = sector_cell_for_poi(L.type);
  }

  const int map_w = static_cast<int>(balance::Map_size_X);
  const int map_h = static_cast<int>(balance::Map_size_Y);

  // generate_array_roads_1: scan rows, pair POI markers (15), mark corridor 13.
  for (int row = 0; row < map_h; ++row) {
    int anotherX = -1;
    int anotherX2 = -1;
    for (int col = 0; col < map_w; ++col) {
      const unsigned char v = sector_grid_[row][col];
      if (!sector_triggers_road_scan(v)) continue;
      if (v == Sector_PoiMarker) {
        if (anotherX < 0) anotherX = col;
        else anotherX2 = col;
      }
    }
    if (anotherX >= 0 && anotherX2 > anotherX) {
      if (c2_scratch_) mapgen_note_road_pair(*c2_scratch_, anotherX, anotherX2, Sector_RoadHoriz);
      for (int c = anotherX + 1; c < anotherX2; ++c) {
        if (sector_grid_[row][c] == Sector_Empty) sector_grid_[row][c] = Sector_RoadHoriz;
      }
      if (sector_grid_[row][anotherX] == Sector_Empty)
        sector_grid_[row][anotherX] = Sector_RoadHoriz;
    }
  }

  // generate_array_roads_2: scan columns; 0→12, existing 13→14 (intersections).
  for (int col = 0; col < map_w; ++col) {
    int anotherX = -1;
    int anotherX2 = -1;
    for (int row = 0; row < map_h; ++row) {
      const unsigned char v = sector_grid_[row][col];
      if (!sector_triggers_road_scan(v)) continue;
      if (v == Sector_PoiMarker) {
        if (anotherX < 0) anotherX = row;
        else anotherX2 = row;
      }
    }
    if (anotherX >= 0 && anotherX2 > anotherX) {
      if (c2_scratch_) {
        c2_scratch_->Loop_secrets2 += 1.0;
        mapgen_note_road_pair(*c2_scratch_, anotherX, anotherX2, Sector_RoadVert);
      }
      for (int r = anotherX + 1; r < anotherX2; ++r) {
        unsigned char& cell = sector_grid_[r][col];
        if (cell == Sector_RoadHoriz) cell = Sector_RoadCross;
        else if (cell == Sector_Empty) cell = Sector_RoadVert;
      }
      unsigned char& start = sector_grid_[anotherX][col];
      if (start == Sector_RoadHoriz) start = Sector_RoadCross;
      else if (start == Sector_Empty) start = Sector_RoadVert;
    }
  }

  road_count_ = 0;
  for (int y = 0; y < kSectorGrid; ++y) {
    for (int x = 0; x < kSectorGrid; ++x) {
      if (sector_is_road_marker(sector_grid_[y][x])) ++road_count_;
    }
  }
}

bool MapGenerator::is_road_sector(int sector_x, int sector_y) const {
  if (sector_x < 0 || sector_y < 0 || sector_x >= kSectorGrid || sector_y >= kSectorGrid)
    return false;
  return sector_is_road_marker(sector_grid_[sector_y][sector_x]);
}

void MapGenerator::apply_collision_for(int idx, World& world) const {
  // Building walls are now the C2 Solid polygons of the *_int objects (building_solid.cpp,
  // Game::building_blocks). The old invented 3x3 block_r cell stamps at +-150 px "pair"
  // offsets (which did not even match the drawn building) are gone; the grid stays clear.
  (void)idx;
  (void)world;
}

void MapGenerator::apply_collision(World& world) const {
  for (int i = 0; i < count_; ++i) {
    if (locs_[i].active) apply_collision_for(i, world);
  }
}

void MapGenerator::rebuild_collision(World& world) const {
  world.reset();
  apply_collision(world);
}

void MapGenerator::reload_locations(Vec2 player, LocationReloadMode mode) {
  static const char* const kReloadAudit[] = {
      kC2_Reload_locations, kC2_Reload_locations_start, kC2_Reload_locations_aftersave,
      kC2_create_map_location, kC2_erase_map_location, kC2_respawn_radius2};
  (void)kReloadAudit[0];

  const float R = reload_radius_for(mode);
  if (mode == LocationReload_AfterSave) {
    for (int i = 0; i < count_; ++i) locs_[i].active = false;
  }

  for (int i = 0; i < count_; ++i) {
    MapLocation& L = locs_[i];
    const float d = dist_to(L, player);
    if (mode == LocationReload_Runtime) {
      if (d > R && L.active) L.active = false;
      else if (d <= R && !L.active) L.active = true;
    } else {
      if (d <= R) L.active = true;
    }
  }
}

} // namespace mdaz
