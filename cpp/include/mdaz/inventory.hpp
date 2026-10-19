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

class Inventory {
public:
  Inventory();

  void clear();
  // C2 Check_space_inventory / Check_space_pants — true iff full count fits.
  bool can_add_item(ItemId id, int count, const ItemDatabase& db) const;
  bool add_item(ItemId id, int count, const ItemDatabase& db);
  bool remove_at(int slot, int count);
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
  void sync_clothing_gates(const ItemDatabase& db);

  InvSlot& slot(int i);
  const InvSlot& slot(int i) const;
  int slot_count() const { return kInventorySlots; }
  // Usable slots: 1 base (C2 t381) + container sizes of worn pieces.
  int capacity() const;
  int worn_count() const { return worn_count_; }
  ItemId worn_at(int i) const { return (i >= 0 && i < worn_count_) ? worn_[i].id : 0; }

private:
  static const int kMaxWorn = 6;
  InvSlot slots_[kInventorySlots];
  WornSlot worn_[kMaxWorn];
  int worn_count_;
  ClothingContainers clothing_;
};

} // namespace mdaz
