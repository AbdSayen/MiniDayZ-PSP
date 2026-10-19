#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

class Rng;
struct C2SessionGlobals;
struct C2Map;

// Location types inferred from Map generation / create_map_element strings.
enum LocationType {
  Loc_Village = 0,
  Loc_City,
  Loc_Military,
  Loc_GasStation,
  Loc_Hospital,
  Loc_Firestation,
  Loc_Helicrash,
  Loc_Secret,
  Loc_BunkerEntrance,
  Loc_Police,
  Loc_Church,
  Loc_Count
};

struct MapLocation {
  LocationType type;
  Vec2 pos;
  int sector_x;
  int sector_y;
  bool active;       // C2 map_element IV3 (1 = create_map_location loaded)
  bool loot_seeded;  // Spawn_drop / POI piles spawned while loaded
  // C2 create_map_element category 7 row: map_element IV2 (building subtype), index into the
  // building_solid shape table (-1 = no *_int Solid object) and the owning mapgen cell.
  short c2_sub;
  short shape;
  short c2_cell;
  bool hostile_anchor;  // first building of its cell: carries the port's per-POI hostile budget
};

// C2 Reload_locations / Reload_locations_start / Reload_locations_aftersave
enum LocationReloadMode {
  LocationReload_Runtime = 0,
  LocationReload_Start,
  LocationReload_AfterSave
};

static const int kMaxLocations = 512;

// C2 generate_array_locations locals: river_X, river_Y, river_R (default R=2), River_ready, Bridges.
// Export never reads/writes these; we derive a deterministic river from seed for tile 4 stamps.
struct RiverParams {
  int sector_x;
  int sector_y;
  int radius_sectors; // river_R default 2
  bool horizontal;    // axis from seed
  bool valid;
};

// One building/prop in a POI cluster (C2 create_map_location layout tags in data.js).
struct PoiPartView {
  const char* sprite;
  float dx;
  float dy;
  float block_r;
  bool valid;
};

int poi_part_count(LocationType t, int variant);
PoiPartView poi_part(LocationType t, int variant, int part_index);

// C2 create_map_location: SetTile footprint uses ±300 layout units (data.js literals -300, 300).
constexpr float kC2PoiPairSpacing = 300.f;
constexpr float kC2PoiPairHalfSpacing = kC2PoiPairSpacing * 0.5f;

// C2 item_activate seed plant: Timer behavior StartTimer duration 20s, tag "grow".
constexpr float kC2CropGrowSeconds = 20.f;

// C2 generate_array_roads_1/_2 sector markers (Array uid 150, Map_size 16×16).
// 15 = POI marker; 12/13/14 = road cells consumed by generate_array_tomap → dirt tile 15.
class MapGenerator {
public:
  MapGenerator();
  void clear();
  void generate_new_game(int seed);
  // C2 generate_bunker_entrance when CurrentLevel >= 1.
  void ensure_bunker_entrance(Rng& rng, int current_level);
  // Stamp POI footprints into World collision grid (approximate building blockers).
  void apply_collision(class World& world) const;
  void apply_collision_for(int index, class World& world) const;
  void rebuild_collision(class World& world) const;
  // Distance-based POI load/unload (respawn_radius2; start/aftersave cap min(r,1000)).
  void reload_locations(Vec2 player, LocationReloadMode mode);
  int location_count() const { return count_; }
  const MapLocation& location(int i) const { return locs_[i]; }
  MapLocation& location_mut(int i) { return locs_[i]; }
  const RiverParams& river() const { return river_; }
  // True when sector grid holds C2 road marker (12/13/14) after generate_array_roads_*.
  bool is_road_sector(int sector_x, int sector_y) const;
  int road_sector_count() const { return road_count_; }
  // FNV-1a over (type, sector_x, sector_y) of every location: equal on desktop and PSP.
  unsigned poi_hash() const {
    unsigned hsh = 2166136261u;
    for (int i = 0; i < count_; ++i) {
      const int v[3] = { static_cast<int>(locs_[i].type), locs_[i].sector_x, locs_[i].sector_y };
      for (int k = 0; k < 3; ++k) { hsh ^= static_cast<unsigned>(v[k]) & 0xffu; hsh *= 16777619u; }
    }
    return hsh;
  }
  // Raw C2 Array-150 cell value (0 empty, 6/9/11 POI, 12/13/14 road, 15 POI marker).
  unsigned char sector_cell(int sector_x, int sector_y) const {
    if (sector_x < 0 || sector_y < 0 || sector_x >= kSectorGrid || sector_y >= kSectorGrid) return 0;
    return sector_grid_[sector_y][sector_x];
  }

  // Faithful data.js generator output (cells, array-518 element rows, heli/hummer sites).
  const C2Map& c2() const;

  // Optional C2 global scratch (hotspotX / anotherX / Loop_secrets) during generate.
  void set_c2_scratch(C2SessionGlobals* g) { c2_scratch_ = g; }

private:
  C2SessionGlobals* c2_scratch_;
  MapLocation locs_[kMaxLocations];
  int count_;
  RiverParams river_;
  static const int kSectorGrid = 16;
  unsigned char sector_grid_[kSectorGrid][kSectorGrid];
  int road_count_;
  void add(LocationType t, int sx, int sy);
  void add_c2(LocationType t, float x, float y, int sx, int sy, int sub, int shape, int cell);
  void init_river(int seed);
  void apply_generate_array_roads();
  static float reload_radius_for(LocationReloadMode mode);
  static float dist_to(const MapLocation& L, Vec2 p);
};

} // namespace mdaz
