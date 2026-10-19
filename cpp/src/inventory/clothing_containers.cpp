#include "mdaz/clothing_containers.hpp"

#include "mdaz/inventory.hpp"
#include "mdaz/inventory.hpp"
#include "mdaz/item_db.hpp"
#include "mdaz/inventory.hpp"

namespace mdaz {

namespace {

int slot_index(ClothingRegion r, int i) {
  return static_cast<int>(r) * kClothingContainerSlots + i;
}

int stack_max_for_cloth(const ItemDatabase& db, ItemId id) {
  const ItemDef* def = db.find(id);
  return def ? def->stack_max : 1;
}

} // namespace

ClothingContainers::ClothingContainers() { clear(); }

void ClothingContainers::clear() {
  for (int i = 0; i < ClothingRegion_Count * kClothingContainerSlots; ++i) {
    slots_[i].id = 0;
    slots_[i].count = 0;
  }
  for (int i = 0; i < ClothingRegion_Count; ++i) gates_[i] = 0;
}

const InvSlot& ClothingContainers::slot(ClothingRegion r, int i) const {
  return slots_[slot_index(r, i)];
}

InvSlot& ClothingContainers::slot_mut(ClothingRegion r, int i) {
  return slots_[slot_index(r, i)];
}

ClothingRegion clothing_region_for_worn_item(ItemId id, const ItemDef* def) {
  if (id <= 0) return ClothingRegion_Count;
  if (def && def->category == ItemCat_Backpack) return ClothingRegion_Backpack;
  if (id >= 450 && id <= 457) return ClothingRegion_Backpack;
  if (id == 454 || id == 455) return ClothingRegion_Backpack;
  if (id >= 400 && id <= 405) return ClothingRegion_Vest;
  if (id == 355 || id == 303 || id == 300 || id == 301 || id == 302 || id == 304 || id == 305 ||
      id == 306)
    return ClothingRegion_Pants;
  if (def && def->category == ItemCat_Clothing) return ClothingRegion_Outerwear;
  if (id == 353 || id == 350 || id == 351 || id == 352 || id == 354 || id == 356 || id == 357 ||
      id == 358 || id == 359 || id == 360 || id == 361 || id == 362 || id == 363 || id == 364 ||
      id == 365)
    return ClothingRegion_Outerwear;
  return ClothingRegion_Count;
}

void ClothingContainers::set_region_gate(ClothingRegion r, int value) {
  if (r >= 0 && r < ClothingRegion_Count) gates_[r] = value;
}

bool ClothingContainers::region_available(ClothingRegion r) const {
  if (r < 0 || r >= ClothingRegion_Count) return false;
  return gates_[r] > 0;
}

bool ClothingContainers::can_add(ClothingRegion r, ItemId id, int count,
                                 const ItemDatabase& db) const {
  if (id <= 0 || count <= 0 || !region_available(r)) return false;
  const int stack_max = stack_max_for_cloth(db, id);
  if (stack_max <= 0) return false;
  int remaining = count;
  for (int i = 0; i < kClothingContainerSlots && remaining > 0; ++i) {
    const InvSlot& s = slot(r, i);
    if (s.id == id && s.count < stack_max) {
      const int space = stack_max - s.count;
      remaining -= (remaining < space ? remaining : space);
    }
  }
  for (int i = 0; i < kClothingContainerSlots && remaining > 0; ++i) {
    if (slot(r, i).empty()) {
      const int take = remaining < stack_max ? remaining : stack_max;
      remaining -= take;
    }
  }
  return remaining == 0;
}

bool ClothingContainers::add(ClothingRegion r, ItemId id, int count, const ItemDatabase& db) {
  if (!can_add(r, id, count, db)) return false;
  int stack_max = stack_max_for_cloth(db, id);
  for (int i = 0; i < kClothingContainerSlots && count > 0; ++i) {
    InvSlot& s = slot_mut(r, i);
    if (s.id == id && s.count < stack_max) {
      const int space = stack_max - s.count;
      const int take = count < space ? count : space;
      s.count += take;
      count -= take;
    }
  }
  for (int i = 0; i < kClothingContainerSlots && count > 0; ++i) {
    InvSlot& s = slot_mut(r, i);
    if (s.empty()) {
      const int take = count < stack_max ? count : stack_max;
      s.id = id;
      s.count = take;
      count -= take;
    }
  }
  return count == 0;
}

bool ClothingContainers::can_add_cascade(ItemId id, int count, const ItemDatabase& db) const {
  static const ClothingRegion kOrder[] = {ClothingRegion_Pants, ClothingRegion_Outerwear,
                                          ClothingRegion_Vest, ClothingRegion_Backpack};
  for (int o = 0; o < 4; ++o) {
    const ClothingRegion r = kOrder[o];
    if (region_available(r) && can_add(r, id, count, db)) return true;
  }
  return false;
}

bool ClothingContainers::add_cascade(ItemId id, int count, const ItemDatabase& db) {
  static const ClothingRegion kOrder[] = {ClothingRegion_Pants, ClothingRegion_Outerwear,
                                          ClothingRegion_Vest, ClothingRegion_Backpack};
  for (int o = 0; o < 4; ++o) {
    const ClothingRegion r = kOrder[o];
    if (region_available(r) && add(r, id, count, db)) return true;
  }
  return false;
}

int ClothingContainers::count_ammo(ItemId ammo_id) const {
  if (ammo_id <= 0) return 0;
  int n = 0;
  for (int ri = 0; ri < ClothingRegion_Count; ++ri) {
    for (int si = 0; si < kClothingContainerSlots; ++si) {
      const InvSlot& s = slots_[slot_index(static_cast<ClothingRegion>(ri), si)];
      if (s.id == ammo_id) n += s.count;
    }
  }
  return n;
}

int ClothingContainers::consume_ammo(ItemId ammo_id, int need) {
  if (ammo_id <= 0 || need <= 0) return 0;
  static const ClothingRegion kOrder[] = {ClothingRegion_Pants, ClothingRegion_Outerwear,
                                          ClothingRegion_Vest, ClothingRegion_Backpack};
  int taken = 0;
  for (int o = 0; o < 4 && taken < need; ++o) {
    const ClothingRegion r = kOrder[o];
    if (!region_available(r)) continue;
    for (int i = 0; i < kClothingContainerSlots && taken < need; ++i) {
      InvSlot& s = slot_mut(r, i);
      if (s.id != ammo_id || s.count <= 0) continue;
      const int want = need - taken;
      const int chunk = s.count < want ? s.count : want;
      s.count -= chunk;
      if (s.count <= 0) {
        s.id = 0;
        s.count = 0;
      }
      taken += chunk;
    }
  }
  return taken;
}

void ClothingContainers::restore_region(ClothingRegion r, const ItemId* ids, const int* counts,
                                        int n) {
  if (r < 0 || r >= ClothingRegion_Count || !ids || !counts) return;
  if (n > kClothingContainerSlots) n = kClothingContainerSlots;
  for (int i = 0; i < kClothingContainerSlots; ++i) {
    InvSlot& s = slot_mut(r, i);
    s.id = 0;
    s.count = 0;
  }
  for (int i = 0; i < n; ++i) {
    slot_mut(r, i).id = ids[i];
    slot_mut(r, i).count = counts[i];
  }
}

} // namespace mdaz
