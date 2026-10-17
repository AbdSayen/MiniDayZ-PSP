#include "mdaz/player_worn_visual.hpp"

#include "mdaz/item_sprites.hpp"

#include <cstring>

namespace mdaz {

namespace {

bool is_head_gear(ItemId id) {
  if (id == 250) return true; // gasmask
  if (id >= 251 && id <= 267) return true;
  return false;
}

bool is_vest(ItemId id) { return id >= 400 && id <= 405; }

bool is_outerwear(ItemId id) { return id >= 350 && id <= 365; }

} // namespace

bool item_is_worn_visual_layer(ItemId id, WornVisualLayer layer) {
  if (id <= 0) return false;
  switch (layer) {
  case WornVisual_Head: return is_head_gear(id);
  case WornVisual_Vest: return is_vest(id);
  case WornVisual_Outerwear: return is_outerwear(id);
  default: return false;
  }
}

ItemId worn_visual_for_layer(const Inventory& inv, WornVisualLayer layer) {
  ItemId pick = 0;
  for (int i = 0; i < inv.worn_count(); ++i) {
    const ItemId id = inv.worn_at(i);
    if (item_is_worn_visual_layer(id, layer)) pick = id;
  }
  return pick;
}

const char* worn_item_sprite_base(ItemId id) {
  const ItemIcon* ic = find_item_icon(id);
  if (!ic || !ic->sprite) return 0;
  static char buf[64];
  const char* dash = std::strstr(ic->sprite, "-sheet");
  if (!dash) {
    std::snprintf(buf, sizeof(buf), "%s", ic->sprite);
    return buf;
  }
  const int n = static_cast<int>(dash - ic->sprite);
  if (n <= 0 || n >= static_cast<int>(sizeof(buf))) return 0;
  std::memcpy(buf, ic->sprite, static_cast<std::size_t>(n));
  buf[n] = 0;
  return buf;
}

} // namespace mdaz
