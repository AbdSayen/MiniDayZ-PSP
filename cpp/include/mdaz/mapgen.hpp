#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

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
  bool active;
};

static const int kMaxLocations = 128;

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

// Scaffold for generate_array_locations / generate_location / generate_helicrashes.
// POI clusters use poi_part_* tables; full tile/road Array placement not yet ported.
class MapGenerator {
public:
  MapGenerator();
  void clear();
  void generate_new_game(int seed);
  // Stamp POI footprints into World collision grid (approximate building blockers).
  void apply_collision(class World& world) const;
  int location_count() const { return count_; }
  const MapLocation& location(int i) const { return locs_[i]; }
  const RiverParams& river() const { return river_; }

private:
  MapLocation locs_[kMaxLocations];
  int count_;
  RiverParams river_;
  void add(LocationType t, int sx, int sy);
  void init_river(int seed);
};

} // namespace mdaz
