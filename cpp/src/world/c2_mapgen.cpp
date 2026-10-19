#include "mdaz/c2_mapgen.hpp"
#include "mdaz/rng.hpp"

#include <cmath>

namespace mdaz {

namespace {

const double kTileSize = 60.0;  // tilemap_* tile size; only feeds TilemapspotX/Y (no rows use it)

inline double c2int(double v) { return v < 0 ? std::ceil(v) : std::floor(v); }
inline double c2mod(double a, double b) { return b == 0 ? 0 : std::fmod(a, b); }

struct GenLocCtx {
  Rng* rng;
  C2Map* m;
  C2Cell* cell;
  double cell_x, cell_y, uid;
  // array 518 for this cell: [row][col], IndexOf(0) scans col0.
  static const int kCap = 400;
  double a[kCap][7];
  int width;  // current array width (rows); writes beyond kCap are dropped
  int zr_last;
  double rnd() { return static_cast<double>(rng->next_float()); }
  double rnd_range(double lo, double hi) { return lo + rnd() * (hi - lo); }
  int pick(int n) { int i = static_cast<int>(rnd() * n); return i >= n ? n - 1 : i; }
  double iv(int k) const {
    if (k == 0) return cell->type;
    if (k == 1) return cell->ay;
    if (k == 2) return cell->ax;
    if (k == 9) return cell->iv9 ? 1 : 0;
    return 0;
  }
  double grid(double x, double y) const {
    const int ix = static_cast<int>(std::floor(x)), iy = static_cast<int>(std::floor(y));
    if (ix < 0 || iy < 0 || ix >= kC2Grid || iy >= kC2Grid) return 0;
    return m->arr[ix][iy];
  }
  double index_of0() const {
    for (int i = 0; i < kCap; ++i) if (a[i][0] == 0) return i;
    return -1;
  }
  void set(double x, double y, double v) {
    const int ix = static_cast<int>(x), iy = static_cast<int>(y);
    if (ix < 0 || iy < 0 || ix >= kCap || iy >= 7) return;
    a[ix][iy] = v;
    if (ix + 1 > width) width = ix + 1;
  }
  void resize(double w) {
    int n = static_cast<int>(w);
    if (n < 0) n = 0;
    if (n > kCap) n = kCap;
    for (int i = n; i < kCap; ++i) for (int j = 0; j < 7; ++j) a[i][j] = 0;
    width = n;
  }
  void spawn_zed_resp(double x, double y) {
    if (m->zr_count >= C2Map::kMaxZr) { zr_last = -1; return; }
    zr_last = m->zr_count++;
    C2ZedResp& z = m->zr[zr_last];
    z.x = static_cast<float>(x); z.y = static_cast<float>(y); z.frame = 0; z.iv0 = 0; z.iv2 = 0;
  }
  void zed_resp_frame(double f) { if (zr_last >= 0) m->zr[zr_last].frame = static_cast<std::int16_t>(f); }
  void zed_resp_iv(int k, double v) { if (zr_last < 0) return; if (k == 0) m->zr[zr_last].iv0 = static_cast<std::int16_t>(v); else if (k == 2) m->zr[zr_last].iv2 = static_cast<std::int16_t>(v); }
};

#include "generate_location.inc"

struct LevelCounts { int villages, militaries, cities, hospitals, firestations, gas, helis, hummers; };
// generate_array_locations (per CurrentLevel) + generate_helicrashes (helispawned/hammerspawned).
const LevelCounts kLevels[4] = {
  {20, 0, 3, 1, 1, 1, 0, 1},
  {15, 2, 5, 2, 2, 2, 2, 3},
  {6, 4, 10, 3, 3, 3, 3, 5},
  {3, 6, 11, 4, 2, 4, 4, 6},
};

int rnd_int(Rng& r, int n) {  // floor(random(n))
  int i = static_cast<int>(r.next_float() * n);
  return i >= n ? n - 1 : i;
}

unsigned char at(const C2Map& m, int x, int y) {
  if (x < 0 || y < 0 || x >= kC2Grid || y >= kC2Grid) return 0;  // C2 out-of-range read = 0
  return m.arr[x][y];
}
void put(C2Map& m, int x, int y, unsigned char v) {
  if (x < 0 || y < 0 || x >= kC2Grid || y >= kC2Grid) return;  // C2 ignores out-of-range writes
  m.arr[x][y] = v;
}
bool block_free(const C2Map& m, int x, int y) {
  for (int dx = 0; dx < 3; ++dx)
    for (int dy = 0; dy < 3; ++dy)
      if (at(m, x + dx, y + dy) != 0) return false;
  return true;
}

void place(C2Map& m, Rng& rng, int count, unsigned char code) {
  int guard = 0;
  while (count > 0 && guard++ < 20000) {
    const int x = rnd_int(rng, kC2Grid);
    const int y = rnd_int(rng, kC2Grid);
    if (!block_free(m, x, y)) continue;
    put(m, x + 1, y + 1, code);
    --count;
  }
}

bool road_trigger(unsigned char v) { return v == 9 || v == 6 || v == 11 || v == 15; }

int pick_cell(C2Map& m, Rng& rng, bool (*pred)(int)) {
  int idx[kC2Grid * kC2Grid], n = 0;
  for (int i = 0; i < kC2Grid * kC2Grid; ++i) if (pred(m.cells[i].type)) idx[n++] = i;
  if (n == 0) return -1;
  return idx[rnd_int(rng, n)];
}

bool is_road(int t) { return t == 12 || t == 13; }
bool is_wild(int t) { return t == 2 || t == 8 || t == 1 || t == 10 || t == 3; }
bool is_type22(int t) { return t == 22; }
bool is_heli_site(int t) { return t == 1 || t == 10 || t == 2 || t == 8; }
bool is_hummer_site(int t) { return t == 5 || t == 15 || t == 13; }

} // namespace

void c2_generate_map(C2Map& m, Rng& rng, int level) {
  if (level < 0) level = 0;
  if (level > 3) level = 3;
  const LevelCounts& L = kLevels[level];
  for (int x = 0; x < kC2Grid; ++x) for (int y = 0; y < kC2Grid; ++y) m.arr[x][y] = 0;
  m.row_count = 0; m.zr_count = 0; m.heli_count = 0; m.hummer_count = 0; m.has_bunker = false;
  m.dropped_rows = 0;

  // generate_array_locations: village 9, military 5, city 6, hospital 11, firestation 15.
  place(m, rng, L.villages, 9);
  place(m, rng, L.militaries, 5);
  place(m, rng, L.cities, 6);
  place(m, rng, L.hospitals, 11);
  place(m, rng, L.firestations, 15);

  // generate_array_roads_1: per array row Y, the first two POIs along X get 13 between them.
  for (int row = 0; row < kC2Grid; ++row) {
    int a = -1, b = -1;
    for (int x = 0; x < kC2Grid; ++x) {
      if (!road_trigger(m.arr[x][row])) continue;
      if (a == -1) a = x; else if (b == -1) b = x;
    }
    if (a != -1 && b != -1)
      for (int i = 0; i < b - a; ++i) if (at(m, a + i, row) == 0) put(m, a + i, row, 13);
  }
  // generate_array_roads_2: per array column X, along Y: 13 -> 14 (crossing), 0 -> 12.
  for (int row = 0; row < kC2Grid; ++row) {
    int a = -1, b = -1;
    for (int y = 0; y < kC2Grid; ++y) {
      if (!road_trigger(m.arr[row][y])) continue;
      if (a == -1) a = y; else if (b == -1) b = y;
    }
    if (a != -1 && b != -1)
      for (int i = 0; i < b - a; ++i) {
        if (at(m, row, a + i) == 13) put(m, row, a + i, 14);
        if (at(m, row, a + i) == 0) put(m, row, a + i, 12);
      }
  }
  // Secret location: a random free 3x3 block with corner in [2,15]^2, centre = 2.
  {
    int cx[kC2Grid * kC2Grid], cy[kC2Grid * kC2Grid], n = 0;
    for (int x = 0; x < kC2Grid; ++x)
      for (int y = 0; y < kC2Grid; ++y)
        if (block_free(m, x, y) && x >= 2 && x <= 15 && y >= 2 && y <= 15) { cx[n] = x; cy[n] = y; ++n; }
    if (n > 0) { const int k = rnd_int(rng, n); put(m, cx[k] + 1, cy[k] + 1, 2); }
  }

  // Mapgen instances (spawn order: loopindex -> IV1 = li / 16, IV2 = li % 16) + generate_array_tomap.
  for (int li = 0; li < kC2Grid * kC2Grid; ++li) {
    C2Cell& c = m.cells[li];
    c.ay = li / kC2Grid; c.ax = li % kC2Grid; c.iv9 = false;
    c.row_first = 0; c.row_count = 0; c.zr_first = 0; c.zr_count = 0;
    const unsigned char v = m.arr[c.ax][c.ay];
    int t = 0;
    switch (v) {
    case 0: { static const int ch[] = {3, 1, 2, 8, 10, 1, 2, 8, 10}; t = ch[rnd_int(rng, 9)]; break; }
    case 9: { static const int ch[] = {9, 4, 16, 17}; t = ch[rnd_int(rng, 4)]; break; }
    case 6: { static const int ch[] = {6, 21}; t = ch[rnd_int(rng, 2)]; break; }
    case 5: t = 22; break;
    case 17: t = 51; break;
    case 1: t = 20; break;
    case 2: { static const int ch[] = {23, 24, 25, 26, 27, 28, 29, 40, 42, 43, 44, 45, 46, 47, 48, 49};
              t = ch[rnd_int(rng, 16)]; break; }
    default: t = v; break;  // 11, 12, 13, 14, 15, 50..57, 99..101 keep their code
    }
    c.type = t;
  }
  // Gas stations replace random road cells: 12 -> 30, 13 -> 31.
  for (int i = 0; i < L.gas; ++i) {
    const int k = pick_cell(m, rng, is_road);
    if (k >= 0) m.cells[k].type = (m.cells[k].type == 12) ? 30 : 31;
  }
  if (level >= 1) { const int k = pick_cell(m, rng, is_wild); if (k >= 0) m.cells[k].type = 32; }
  for (int i = 0; i < level && i < 3; ++i) {  // level 1: once, 2: twice, 3: three times: 22 -> 5
    const int k = pick_cell(m, rng, is_type22);
    if (k >= 0) m.cells[k].type = 5;
  }

  // generate_mapgen_toelements: generate_location for every mapgen, in instance order.
  static GenLocCtx G;  // ~22 KB array: keep it off the (small PSP) stack
  double V[kGenLocVars];
  for (int i = 0; i < kGenLocVars; ++i) V[i] = 0;
  for (int li = 0; li < kC2Grid * kC2Grid; ++li) {
    C2Cell& c = m.cells[li];
    G.rng = &rng; G.m = &m; G.cell = &c;
    G.cell_x = c.wx(); G.cell_y = c.wy(); G.uid = li;
    for (int r = 0; r < GenLocCtx::kCap; ++r) for (int j = 0; j < 7; ++j) G.a[r][j] = 0;
    G.width = 0; G.zr_last = -1;
    for (int k = 0; k < 7; ++k) V[k] = 0;  // function locals; camp_located / post_points are globals
    c.zr_first = m.zr_count;
    generate_location_c2(G, V);
    c.zr_count = m.zr_count - c.zr_first;
    c.row_first = m.row_count;
    for (int r = 0; r < G.width; ++r) {
      if (G.a[r][0] == 0) continue;  // empty row (create_map_element would get category 0)
      if (m.row_count >= C2Map::kMaxRows) { ++m.dropped_rows; continue; }
      C2ElementRow& o = m.rows[m.row_count++];
      o.cat = static_cast<std::int16_t>(G.a[r][0]);
      o.sub = static_cast<std::int16_t>(G.a[r][1]);
      o.x = static_cast<float>(G.a[r][2]);
      o.y = static_cast<float>(G.a[r][3]);
      o.c5 = static_cast<std::int16_t>(G.a[r][5]);
      o.c6 = static_cast<std::int16_t>(G.a[r][6]);
    }
    c.row_count = m.row_count - c.row_first;
  }

  // generate_helicrashes: random wild cell (1/10/2/8) not yet used -> helicrash_point (+500,+500);
  // hummer: random 5/15/13 cell not yet used -> hammercrash_point (+500,+600).
  for (int left = L.helis, guard = 0; left > 0 && guard < 5000; ++guard) {
    const int k = pick_cell(m, rng, is_heli_site);
    if (k < 0) break;
    C2Cell& c = m.cells[k];
    if (c.iv9) continue;
    c.iv9 = true; --left;
    if (m.heli_count < C2Map::kMaxPoints) {
      m.heli_x[m.heli_count] = c.wx() + 500.f; m.heli_y[m.heli_count] = c.wy() + 500.f; ++m.heli_count;
    }
  }
  for (int left = L.hummers, guard = 0; left > 0 && guard < 5000; ++guard) {
    const int k = pick_cell(m, rng, is_hummer_site);
    if (k < 0) break;
    C2Cell& c = m.cells[k];
    if (c.iv9) continue;
    c.iv9 = true; --left;
    if (m.hummer_count < C2Map::kMaxPoints) {
      m.hummer_x[m.hummer_count] = c.wx() + 500.f; m.hummer_y[m.hummer_count] = c.wy() + 600.f; ++m.hummer_count;
    }
  }
  // generate_bunker_entrance (CurrentLevel >= 1): a random unused wild cell, +500,+500.
  if (level >= 1) {
    int idx[kC2Grid * kC2Grid], n = 0;
    for (int i = 0; i < kC2Grid * kC2Grid; ++i)
      if (!m.cells[i].iv9 && is_heli_site(m.cells[i].type)) idx[n++] = i;
    if (n > 0) {
      const C2Cell& c = m.cells[idx[rnd_int(rng, n)]];
      m.has_bunker = true; m.bunker_x = c.wx() + 500.f; m.bunker_y = c.wy() + 500.f;
    }
  }
}

} // namespace mdaz
