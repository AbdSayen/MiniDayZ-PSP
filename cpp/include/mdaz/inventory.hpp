#pragma once

#include "mdaz/clothing_containers.hpp"
#include "mdaz/item_db.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/survival.hpp"
#include "mdaz/types.hpp"

namespace mdaz {

static const int kInventorySlots = 24;
static const int kHotbarSlots = 4;

// Worn clothing IV6 durability (C2 item instance var).
struct WornSlot {
  ItemId id;
  float iv6;
  WornSlot() : id(0), iv6(0.f) {}
};

int clothing_container_slots(ItemId id);
int clothing_region(ItemId id);
bool is_gear_item(ItemId id);

class Inventory {
public:
  Inventory();

  void clear();
  // C2 Check_space_inventory / Check_space_pants — true iff full count fits.
  bool can_add_item(ItemId id, int count, const ItemDatabase& db) const;
  bool add_item(ItemId id, int count, const ItemDatabase& db);
  bool remove_at(int slot, int count);
  // C2 Combine_ammo on bag slots; -1 = Combine not offered, else the C2 return value.
  int combine_slots(int src, int dst, const ItemDatabase& db);
  // C2 Check_space_*: write id+count into ONE empty slot within capacity (no stack merge).
  bool add_item_new_slot(ItemId id, int count);
  // C2 Pick_player_item: gear (ids 250..499) is worn straight from the ground; the piece
  // previously worn in that region is returned in *old_out (C2 Drop_* puts it on the ground).
  // C2 Pick_player_item: the new piece brings its own container array (*new_box, may be 0 =
  // empty); the old piece's array is returned in *old_box (Drop_* moves it with the piece).
  bool equip_gear(ItemId id, ItemId* old_out, const GearBox* new_box = 0, GearBox* old_box = 0);
  bool use_consumable(int slot, SurvivalState& survival, const ItemDatabase& db, Rng* rng = 0);
  // Equip clothing: moves it out of the bag into its worn region (swap with old piece).
  bool wear_clothing(int slot, const ItemDatabase& db);
  void restore_worn(const ItemId* ids, int count);
  void restore_worn_iv6(const ItemId* ids, const float* iv6, int count);
  float clothing_insulation() const;
  // Damage multiplier 0..1 after worn helmet/vest (1 = no armor).
  float armor_damage_mul() const;
  // Player_get_hit / Bullets_dmg_deal: subtract IV6 on hit slot (dmg_randomizer 3–7).
  void apply_hit_armor_wear(class Rng& rng, bool bullet_hit, bool heavy_melee_wear,
                            bool skip_vest_slot_if_gasmask);
  float worn_iv6(int i) const;

  ClothingContainers& clothing() { return clothing_; }
  const ClothingContainers& clothing() const { return clothing_; }
  // Port-only wear-from-bag path: items that could not stay (old piece's pockets, old piece
  // when no free slot) are parked here for the caller to drop on the ground.
  int take_spill(InvSlot* out, int max);

  InvSlot& slot(int i);
  const InvSlot& slot(int i) const;
  int slot_count() const { return kInventorySlots; }
  // Flat view: index 0 = base t381, then pants/outerwear/vest/backpack arrays in Check_space
  // chain order (sizes = worn piece's container). Indices >= capacity() are an empty dummy.
  int capacity() const;
  // Flat index of container cell (region, sub); -1 if the region has no such cell.
  int flat_index(ClothingRegion r, int sub) const;
  int worn_count() const { return worn_count_; }
  ItemId worn_at(int i) const { return (i >= 0 && i < worn_count_) ? worn_[i].id : 0; }

private:
  static const int kMaxWorn = 6;
  void resync_sizes();
  void spill(ItemId id, int count);
  InvSlot base_;          // C2 t381/t382 (1 slot)
  InvSlot spill_[kInventorySlots];
  int spill_n_;
  WornSlot worn_[kMaxWorn];
  int worn_count_;
  ClothingContainers clothing_;
};

} // namespace mdaz
