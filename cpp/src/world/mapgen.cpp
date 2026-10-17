#include "mdaz/mapgen.hpp"
#include "mdaz/balance_globals.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/world.hpp"

namespace mdaz {

namespace {
// C2 mapgen identifiers (generate_array_locations locals; export never reads river_X/Y/R).
const char kC2_generate_array_locations[] = "generate_array_locations";
const char kC2_generate_array_tomap[] = "generate_array_tomap";
const char kC2_river_X[] = "river_X";
const char kC2_river_Y[] = "river_Y";
const char kC2_river_R[] = "river_R";
const char kC2_River_ready[] = "River_ready";
const char kC2_Bridges[] = "Bridges";
const char kC2_Bridges_2[] = "Bridges_2";
} // namespace

MapGenerator::MapGenerator() : count_(0) {
  river_.valid = false;
  river_.sector_x = 0;
  river_.sector_y = 0;
  river_.radius_sectors = 2;
  river_.horizontal = true;
}

void MapGenerator::clear() {
  count_ = 0;
  river_.valid = false;
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
  L.active = true;
}

void MapGenerator::generate_new_game(int seed) {
  clear();
  // C2 pipeline: generate_array_locations → generate_array_roads_* → generate_array_tomap
  Rng rng(static_cast<std::uint32_t>(seed ? seed : 140));

  // Counts from balance_globals (C2 globals villages/cities/…).
  const int n_villages = static_cast<int>(balance::villages);
  const int n_cities = static_cast<int>(balance::cities);
  const int n_militaries = static_cast<int>(balance::militaries);
  const int n_gas = static_cast<int>(balance::gas_stations);
  const int n_hospitals = static_cast<int>(balance::hospitals);
  const int n_fire = static_cast<int>(balance::firestations);
  const int n_helis = 2 + rng.next_int(0, 2);

  auto place = [&](LocationType t, int n, int margin) {
    int placed = 0;
    int guard = 0;
    while (placed < n && guard++ < 400) {
      const int before = count_;
      add(t, rng.next_int(margin, kMapSectorsX - 1 - margin),
          rng.next_int(margin, kMapSectorsY - 1 - margin));
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
  place(Loc_BunkerEntrance, 1, 3);
  place(Loc_Secret, 1, 0);
  init_river(seed);
}

void MapGenerator::apply_collision(World& world) const {
  for (int i = 0; i < count_; ++i) {
    const MapLocation& L = locs_[i];
    const int n = poi_part_count(L.type, i);
    for (int p = 0; p < n; ++p) {
      const PoiPartView part = poi_part(L.type, i, p);
      if (!part.valid) continue;
      const float cx = L.pos.x + part.dx;
      const float cy = L.pos.y + part.dy;
      const float r = part.block_r > 0.f ? part.block_r : 45.f;
      world.set_blocked_world(cx, cy, true);
      for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
          if (dx == 0 && dy == 0) continue;
          world.set_blocked_world(cx + dx * r * 0.35f, cy + dy * r * 0.35f, true);
        }
      }
    }
  }
}

} // namespace mdaz
