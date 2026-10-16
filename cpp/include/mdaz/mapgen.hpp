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

// Scaffold for generate_array_locations / generate_location / generate_helicrashes.
// Full tile placement from C2 not yet ported — this tracks logical POIs on the 16x16 grid.
class MapGenerator {
public:
  MapGenerator();
  void clear();
  void generate_new_game(int seed);
  int location_count() const { return count_; }
  const MapLocation& location(int i) const { return locs_[i]; }

private:
  MapLocation locs_[kMaxLocations];
  int count_;
  void add(LocationType t, int sx, int sy);
};

} // namespace mdaz
