#include "mdaz/clothing_containers.hpp"

namespace mdaz {

namespace {
InvSlot g_dummy;
}

ClothingContainers::ClothingContainers() { clear(); }

void ClothingContainers::clear() {
  for (int i = 0; i < ClothingRegion_Count; ++i) { boxes_[i].clear(); sizes_[i] = 0; }
}

void ClothingContainers::set_region_size(ClothingRegion r, int n) {
  if (r < 0 || r >= ClothingRegion_Count) return;
  if (n < 0) n = 0;
  if (n > kClothingContainerSlots) n = kClothingContainerSlots;
  sizes_[r] = n;
}

const InvSlot& ClothingContainers::slot(ClothingRegion r, int i) const {
  if (r < 0 || r >= ClothingRegion_Count || i < 0 || i >= kClothingContainerSlots) return g_dummy;
  return boxes_[r].s[i];
}

InvSlot& ClothingContainers::slot_mut(ClothingRegion r, int i) {
  if (r < 0 || r >= ClothingRegion_Count || i < 0 || i >= kClothingContainerSlots) {
    g_dummy = InvSlot();
    return g_dummy;
  }
  return boxes_[r].s[i];
}

void ClothingContainers::take_box(ClothingRegion r, GearBox* out) {
  if (r < 0 || r >= ClothingRegion_Count) { if (out) out->clear(); return; }
  if (out) *out = boxes_[r];
  boxes_[r].clear();
}

void ClothingContainers::put_box(ClothingRegion r, const GearBox* in) {
  if (r < 0 || r >= ClothingRegion_Count) return;
  if (in) boxes_[r] = *in; else boxes_[r].clear();
}

void ClothingContainers::restore_region(ClothingRegion r, const ItemId* ids, const int* counts,
                                        int n) {
  if (r < 0 || r >= ClothingRegion_Count) return;
  boxes_[r].clear();
  for (int i = 0; i < n && i < kClothingContainerSlots; ++i) {
    boxes_[r].s[i].id = ids ? ids[i] : 0;
    boxes_[r].s[i].count = counts ? counts[i] : 0;
  }
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

} // namespace mdaz
