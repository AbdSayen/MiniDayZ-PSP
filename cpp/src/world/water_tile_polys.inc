// GENERATED from data.js t199 (tilemap_water, Solid) tile collision polys, fractions of the
// 60x60 tile. n = 0: null in data.js = full tile box. Tile 11 is a degenerate (zero-area)
// triangle: no collision. Ids >= 12: no custom poly = full box.
struct WaterTilePoly { int n; float pts[6]; };
static const WaterTilePoly kWaterTilePolys[12] = {
  {3, {1, 0, 1, 1, 0, 1}}, {0, {}}, {3, {0, 0, 1, 1, 0, 1}}, {0, {}}, {0, {}}, {0, {}},
  {3, {0, 0, 1, 0, 1, 1}}, {0, {}}, {3, {0, 0, 1, 0, 0, 1}}, {0, {}}, {0, {}},
  {3, {0, 1, 0, 1, 0, 1}},
};
static bool water_tile_solid(int id, float lx, float ly) {
  if (id < 0) return false;
  if (id >= 12 || kWaterTilePolys[id].n == 0) return true;
  const float* p = kWaterTilePolys[id].pts;
  bool in = false;
  for (int i = 0, j = 2; i < 3; j = i++) {
    const float xi = p[i * 2], yi = p[i * 2 + 1], xj = p[j * 2], yj = p[j * 2 + 1];
    if (((yi > ly) != (yj > ly)) && (lx < (xj - xi) * (ly - yi) / (yj - yi) + xi)) in = !in;
  }
  return in;
}
