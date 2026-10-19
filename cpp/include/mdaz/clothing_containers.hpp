#pragma once

#include "mdaz/item_db.hpp"
#include "mdaz/types.hpp"

namespace mdaz {

// C2 player IV19 / IV17 / IV11 / IV9 gate pants → outerwear → vest → backpack.
enum ClothingRegion {
  ClothingRegion_Pants = 0,
  ClothingRegion_Outerwear,
  ClothingRegion_Vest,
  ClothingRegion_Backpack,
  ClothingRegion_Count
};

static const int kClothingContainerSlots = 4; // Check_* params IV93 [0..3]

// C2 Function-object IV93 clothing stash (4 sub-slots per worn region).
class ClothingContainers {
public:
  ClothingContainers();

  void clear();
  void set_region_gate(ClothingRegion r, int value);

  bool region_available(ClothingRegion r) const;
  int region_gate(ClothingRegion r) const { return gates_[static_cast<int>(r)]; }

  bool can_add(ClothingRegion r, ItemId id, int count, const ItemDatabase& db) const;
  bool add(ClothingRegion r, ItemId id, int count, const ItemDatabase& db);

  int count_ammo(ItemId ammo_id) const;
  int consume_ammo(ItemId ammo_id, int need);

  bool can_add_cascade(ItemId id, int count, const ItemDatabase& db) const;
  bool add_cascade(ItemId id, int count, const ItemDatabase& db);

  const InvSlot& slot(ClothingRegion r, int i) const;
  InvSlot& slot_mut(ClothingRegion r, int i);

  void restore_region(ClothingRegion r, const ItemId* ids, const int* counts, int n);

private:
  InvSlot slots_[ClothingRegion_Count * kClothingContainerSlots];
  int gates_[ClothingRegion_Count];
};

ClothingRegion clothing_region_for_worn_item(ItemId id, const ItemDef* def);

} // namespace mdaz
