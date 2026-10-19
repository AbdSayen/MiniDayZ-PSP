#pragma once
// Faithful port of the data.js map generator:
//   generate_array_locations -> generate_array_roads_1/_2 (+ secret spot) -> generate_array_tomap
//   -> generate_mapgen_toelements (generate_location per cell, array 518 rows)
//   -> generate_helicrashes (helicrash_point / hammercrash_point) -> generate_bunker_entrance.
// The C2 array 511 is indexed (ax, ay); a mapgen cell has IV1 = ay, IV2 = ax and sits at
// world (700 + ay*1020, 320 + ax*1020) (mapgen sprite hotspot = top-left, 1020 px per cell).
#include <cstdint>

namespace mdaz {

class Rng;

static const int kC2Grid = 16;
static const float kC2CellSize = 1020.f;
static const float kC2CellOriginX = 700.f;
static const float kC2CellOriginY = 320.f;

// One array-518 row: col0 = map_element category (IV1), col1 = subtype (IV2), col2/3 = X/Y,
// col5/col6 = extra IVs (col4 is the spawned map_element UID at load time - not stored).
struct C2ElementRow {
  std::int16_t cat;
  std::int16_t sub;
  float x;
  float y;
  std::int16_t c5;
  std::int16_t c6;
};

struct C2ZedResp {
  float x, y;
  std::int16_t frame;  // zed_resp_point animation frame = spawner kind
  std::int16_t iv0;
  std::int16_t iv2;    // zed_resp_point IV2 (sub-kind for frames 1/7/8)
};

struct C2Cell {
  int type;          // mapgen IV0 (location program id, 0..101)
  int ax, ay;        // C2 array coordinates
  bool iv9;          // mapgen IV9: something (heli / hummer / bunker) already placed here
  int row_first;     // index into C2Map::rows
  int row_count;
  int zr_first;
  int zr_count;
  float wx() const { return kC2CellOriginX + ay * kC2CellSize; }
  float wy() const { return kC2CellOriginY + ax * kC2CellSize; }
};

struct C2Map {
  static const int kMaxRows = 6000;  // ~3.5k rows per map (L0..L3 measured)
  static const int kMaxZr = 1024;
  static const int kMaxPoints = 16;
  unsigned char arr[kC2Grid][kC2Grid];   // array 511 after roads + secret
  C2Cell cells[kC2Grid * kC2Grid];        // mapgen instances, index = ay * 16 + ax (spawn order)
  C2ElementRow rows[kMaxRows];
  int row_count;
  C2ZedResp zr[kMaxZr];
  int zr_count;
  // helicrash_point (+500,+500) / hammercrash_point (+500,+600) / bunker_entrance (+500,+500)
  float heli_x[kMaxPoints], heli_y[kMaxPoints]; int heli_count;
  float hummer_x[kMaxPoints], hummer_y[kMaxPoints]; int hummer_count;
  float bunker_x, bunker_y; bool has_bunker;
  int dropped_rows;  // rows that did not fit kMaxRows (should stay 0)
};

// Runs the whole generator. current_level = C2 CurrentLevel (0..3).
void c2_generate_map(C2Map& m, Rng& rng, int current_level);

} // namespace mdaz
