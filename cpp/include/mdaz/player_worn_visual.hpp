#pragma once

#include "mdaz/inventory.hpp"
#include "mdaz/types.hpp"

namespace mdaz {

// Worn equipment drawn on top of player body skin (helmet / vest / jacket sheets).
enum WornVisualLayer {
  WornVisual_Outerwear = 0,
  WornVisual_Vest,
  WornVisual_Head,
  WornVisualLayer_Count
};

bool item_is_worn_visual_layer(ItemId id, WornVisualLayer layer);

// Highest-priority worn piece for a layer (last equipped wins), or 0.
ItemId worn_visual_for_layer(const Inventory& inv, WornVisualLayer layer);

// C2 sprite object base from item id (e.g. "helmet_army"); null if no world sheet mapping.
const char* worn_item_sprite_base(ItemId id);

} // namespace mdaz
