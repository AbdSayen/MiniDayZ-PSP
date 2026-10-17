#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

// Original C2 inventory/world icon crop for an item id (sheet0 unless noted).
struct ItemIcon {
  const char* sprite; // e.g. "fnx_pistol-sheet0"
  int sx, sy, sw, sh;
};

// Returns null if no known sprite mapping.
const ItemIcon* find_item_icon(ItemId id);

} // namespace mdaz
