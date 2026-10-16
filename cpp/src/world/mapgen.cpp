#include "mdaz/mapgen.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/world.hpp"

namespace mdaz {

MapGenerator::MapGenerator() : count_(0) {}

void MapGenerator::clear() { count_ = 0; }

void MapGenerator::add(LocationType t, int sx, int sy) {
  if (count_ >= kMaxLocations) return;
  if (sx < 0 || sy < 0 || sx >= kMapSectorsX || sy >= kMapSectorsY) return;
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
  Rng rng(static_cast<std::uint32_t>(seed ? seed : 140));

  // Counts roughly guided by globals villages/cities/militaries/… (names present;
  // exact spawn counts still being extracted from generate_array_locations).
  const int villages = 8 + rng.next_int(0, 4);
  const int cities = 3 + rng.next_int(0, 2);
  const int militaries = 2 + rng.next_int(0, 2);
  const int gas = 3 + rng.next_int(0, 2);
  const int hospitals = 2;
  const int fire = 2;
  const int helis = 2 + rng.next_int(0, 2);

  for (int i = 0; i < villages; ++i)
    add(Loc_Village, rng.next_int(1, kMapSectorsX - 2), rng.next_int(1, kMapSectorsY - 2));
  for (int i = 0; i < cities; ++i)
    add(Loc_City, rng.next_int(2, kMapSectorsX - 3), rng.next_int(2, kMapSectorsY - 3));
  for (int i = 0; i < militaries; ++i)
    add(Loc_Military, rng.next_int(1, kMapSectorsX - 2), rng.next_int(1, kMapSectorsY - 2));
  for (int i = 0; i < gas; ++i)
    add(Loc_GasStation, rng.next_int(1, kMapSectorsX - 2), rng.next_int(1, kMapSectorsY - 2));
  for (int i = 0; i < hospitals; ++i)
    add(Loc_Hospital, rng.next_int(1, kMapSectorsX - 2), rng.next_int(1, kMapSectorsY - 2));
  for (int i = 0; i < fire; ++i)
    add(Loc_Firestation, rng.next_int(1, kMapSectorsX - 2), rng.next_int(1, kMapSectorsY - 2));
  for (int i = 0; i < helis; ++i)
    add(Loc_Helicrash, rng.next_int(0, kMapSectorsX - 1), rng.next_int(0, kMapSectorsY - 1));
  // 1–2 police stations
  const int police = 1 + rng.next_int(0, 1);
  for (int i = 0; i < police; ++i)
    add(Loc_Police, rng.next_int(1, kMapSectorsX - 2), rng.next_int(1, kMapSectorsY - 2));
  // 1 church (meds / papers / whiskey)
  add(Loc_Church, rng.next_int(1, kMapSectorsX - 2), rng.next_int(1, kMapSectorsY - 2));

  // One bunker entrance
  add(Loc_BunkerEntrance, rng.next_int(3, kMapSectorsX - 4), rng.next_int(3, kMapSectorsY - 4));
  add(Loc_Secret, rng.next_int(0, kMapSectorsX - 1), rng.next_int(0, kMapSectorsY - 1));
}

void MapGenerator::apply_collision(World& world) const {
  // Approximate solid footprints around POIs (full building tiles TBD).
  for (int i = 0; i < count_; ++i) {
    const MapLocation& L = locs_[i];
    float span = 48.f;
    if (L.type == Loc_City) span = 90.f;
    if (L.type == Loc_Military) span = 70.f;
    if (L.type == Loc_Village) span = 55.f;
    // Leave a walkable ring; block a cross of cells in the center
    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        if (dx == 0 && dy == 0) continue; // keep center walkable for loot
        world.set_blocked_world(L.pos.x + dx * span * 0.35f, L.pos.y + dy * span * 0.35f, true);
      }
    }
  }
}

} // namespace mdaz
