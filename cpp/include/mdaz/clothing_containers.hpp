#pragma once

#include "mdaz/item_db.hpp"
#include "mdaz/types.hpp"

namespace mdaz {

// C2 Check_space chain order: inventory t381 -> pants (IV19/IV20, t185/t226) ->
// outerwear (IV17/IV18, t219/t227) -> vest (IV11/IV12, t267/t268) -> backpack (IV9/IV10, t189/t225).
enum ClothingRegion {
  ClothingRegion_Pants = 0,
  ClothingRegion_Outerwear,
  ClothingRegion_Vest,
  ClothingRegion_Backpack,
  ClothingRegion_Count
};

// Largest C2 container array ("set size" a198: backpack 452 = 7).
static const int kClothingContainerSlots = 7;

// One container array owned by a gear ITEM instance (C2 t924/t916/t921/t913 uid arrays).
struct GearBox {
  InvSlot s[kClothingContainerSlots];
  void clear() { for (int i = 0; i < kClothingContainerSlots; ++i) s[i] = InvSlot(); }
  bool empty() const {
    for (int i = 0; i < kClothingContainerSlots; ++i) if (!s[i].empty()) return false;
    return true;
  }
};

// Container arrays of the pieces currently worn in the four chain regions.
// size == 0 -> region gate 0 (nothing worn / piece has no pockets).
class ClothingContainers {
public:
  ClothingContainers();

  void clear();
  void set_region_size(ClothingRegion r, int n);
  int region_size(ClothingRegion r) const { return sizes_[static_cast<int>(r)]; }
  bool region_available(ClothingRegion r) const { return region_size(r) > 0; }

  const InvSlot& slot(ClothingRegion r, int i) const;
  InvSlot& slot_mut(ClothingRegion r, int i);

  // Drop_*: the box travels with the piece. take = copy out + clear; put = copy in.
  void take_box(ClothingRegion r, GearBox* out);
  void put_box(ClothingRegion r, const GearBox* in);

  void restore_region(ClothingRegion r, const ItemId* ids, const int* counts, int n);

private:
  GearBox boxes_[ClothingRegion_Count];
  int sizes_[ClothingRegion_Count];
};

ClothingRegion clothing_region_for_worn_item(ItemId id, const ItemDef* def);

} // namespace mdaz
