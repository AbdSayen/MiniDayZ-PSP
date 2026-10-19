#include "mdaz/mapgen.hpp"

#include <cstdio>
#include <cstring>

namespace mdaz {

namespace {

struct PoiPart {
  const char* sprite;
  float dx;
  float dy;
  float block_r;
};

// Offsets: verified ±300 in create_map_location SetTile windows (mdaz::kC2PoiPairSpacing).
static const float kPairSpacing = kC2PoiPairSpacing;

static float half_spacing() { return kPairSpacing * 0.5f; }

// Layout tags from original_docs/data.js → create_map_location / Buildings_base spawns.
// Each row: primary @ -half, secondary @ +half unless noted.

static const PoiPart kVillageWhite[] = {
  {"b_village_brown", -half_spacing(), 0.f, 48.f},
  {"b_village_red", half_spacing(), 0.f, 48.f},
};
static const PoiPart kVillageBrown2[] = {
  {"b_village_red", -half_spacing(), 0.f, 48.f},
  {"b_village_green", half_spacing(), 0.f, 42.f},
};
static const PoiPart kVillageBrown1[] = {
  {"b_village_yellow", -half_spacing(), 0.f, 42.f},
  {"b_hostel", half_spacing(), 0.f, 40.f},
};
static const PoiPart kVillageGreenYellow[] = {
  {"b_village_green", -half_spacing(), 0.f, 42.f},
  {"b_village_yellow", half_spacing(), 0.f, 42.f},
};
static const PoiPart kVillageRed2[] = {
  {"b_village_red2", -half_spacing(), 0.f, 48.f},
  {"b_red_brick", half_spacing(), 0.f, 40.f},
};

static const PoiPart kCityA[] = {
  {"b_city_house_1", -half_spacing(), 0.f, 52.f},
  {"b_city_house_2", half_spacing(), 0.f, 52.f},
};
static const PoiPart kCityB[] = {
  {"b_city_house_2", -half_spacing(), 0.f, 52.f},
  {"b_city_house_3", half_spacing(), 0.f, 52.f},
};
static const PoiPart kCityC[] = {
  {"b_city_house_3", -half_spacing(), 0.f, 52.f},
  {"b_city_house_4", half_spacing(), 0.f, 52.f},
};
static const PoiPart kCityD[] = {
  {"b_city_house_4", -half_spacing(), 0.f, 52.f},
  {"b_car_bus", half_spacing(), 0.f, 55.f},
};

static const PoiPart kMilBarrack[] = {
  {"b_military_barrack2", -half_spacing(), 0.f, 50.f},
  {"b_hunter_shelter", half_spacing(), 0.f, 32.f},
};
static const PoiPart kMilShtab[] = {
  {"b_military_shtab", -half_spacing(), 0.f, 55.f},
  {"b_city_house_1", half_spacing(), 0.f, 52.f},
};

static const PoiPart kGas[] = {
  {"b_gas_station", -half_spacing() * 0.6f, 0.f, 38.f},
  {"b_garage_blue", half_spacing() * 0.6f, 0.f, 36.f},
};

static const PoiPart kHospital[] = {
  {"b_hospital", 0.f, 0.f, 90.f},
};

static const PoiPart kFire[] = {
  {"b_fire_station", -half_spacing(), 0.f, 70.f},
  {"b_hospital", half_spacing(), 0.f, 90.f},
};

static const PoiPart kPolice[] = {
  {"b_police_station", -half_spacing(), 0.f, 55.f},
  {"b_fire_station", half_spacing(), 0.f, 70.f},
};

static const PoiPart kChurch[] = {
  {"b_church", -half_spacing() * 0.7f, 0.f, 75.f},
  {"b_supermarket", half_spacing() * 0.7f, 0.f, 50.f},
};

static const PoiPart kHeli[] = {{"heli_crash", 0.f, 0.f, 40.f}};
static const PoiPart kHunter[] = {{"b_hunter_shelter", 0.f, 0.f, 32.f}};

struct LayoutSet {
  const PoiPart* parts;
  int count;
};

static LayoutSet village_layout(int variant) {
  switch (variant % 5) {
  case 0: return {kVillageWhite, 2};
  case 1: return {kVillageBrown2, 2};
  case 2: return {kVillageBrown1, 2};
  case 3: return {kVillageGreenYellow, 2};
  default: return {kVillageRed2, 2};
  }
}

static LayoutSet city_layout(int variant) {
  switch (variant % 4) {
  case 0: return {kCityA, 2};
  case 1: return {kCityB, 2};
  case 2: return {kCityC, 2};
  default: return {kCityD, 2};
  }
}

static LayoutSet military_layout(int variant) {
  if (variant & 1) return {kMilShtab, 2};
  return {kMilBarrack, 2};
}

static LayoutSet layout_for(LocationType t, int variant) {
  switch (t) {
  case Loc_Village: return village_layout(variant);
  case Loc_City: return city_layout(variant);
  case Loc_Military: return military_layout(variant);
  case Loc_GasStation: return {kGas, 2};
  case Loc_Hospital: return {kHospital, 1};
  case Loc_Firestation: return {kFire, 2};
  case Loc_Police: return {kPolice, 2};
  case Loc_Church: return {kChurch, 2};
  case Loc_Helicrash: return {kHeli, 1};
  case Loc_BunkerEntrance:
  case Loc_Secret: return {kHunter, 1};
  default: return {kHunter, 1};
  }
}

} // namespace

int poi_part_count(LocationType t, int variant) {
  return layout_for(t, variant).count;
}

PoiPartView poi_part(LocationType t, int variant, int part_index) {
  PoiPartView out = {};
  const LayoutSet set = layout_for(t, variant);
  if (part_index < 0 || part_index >= set.count) return out;
  const PoiPart& p = set.parts[part_index];
  out.sprite = p.sprite;
  out.dx = p.dx;
  out.dy = p.dy;
  out.block_r = p.block_r;
  out.valid = true;
  return out;
}

} // namespace mdaz
