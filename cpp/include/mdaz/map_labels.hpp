#pragma once

#include "mdaz/mapgen.hpp"

namespace mdaz {

inline const char* location_label(LocationType t) {
  switch (t) {
  case Loc_Village: return "VILLAGE";
  case Loc_City: return "CITY";
  case Loc_Military: return "MIL";
  case Loc_GasStation: return "GAS";
  case Loc_Hospital: return "HOSP";
  case Loc_Firestation: return "FIRE";
  case Loc_Helicrash: return "HELI";
  case Loc_Secret: return "???";
  case Loc_BunkerEntrance: return "BUNKER";
  case Loc_Police: return "POLICE";
  case Loc_Church: return "CHURCH";
  default: return "POI";
  }
}

} // namespace mdaz
