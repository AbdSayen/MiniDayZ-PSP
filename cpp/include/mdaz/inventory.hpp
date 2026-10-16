#pragma once

#include "mdaz/item_db.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/survival.hpp"
#include "mdaz/types.hpp"

namespace mdaz {

static const int kInventorySlots = 24;
static const int kHotbarSlots = 4;

struct InvSlot {
  ItemId id;
  int count;
  InvSlot() : id(0), count(0) {}
  bool empty() const { return id == 0 || count <= 0; }
};

class Inventory {
public:
  Inventory();

  void clear();
  bool add_item(ItemId id, int count, const ItemDatabase& db);
  bool remove_at(int slot, int count);
  bool use_consumable(int slot, SurvivalState& survival, const ItemDatabase& db, Rng* rng = 0);
  // Equip clothing from inventory into worn slot (does not remove from bag — stub).
  bool wear_clothing(int slot, const ItemDatabase& db);
  void restore_worn(const ItemId* ids, int count);
  float clothing_insulation() const;
  // Damage multiplier 0..1 after worn helmet/vest (1 = no armor).
  float armor_damage_mul() const;

  InvSlot& slot(int i);
  const InvSlot& slot(int i) const;
  int slot_count() const { return kInventorySlots; }
  // Usable bag slots (base 12 + backpack bonuses); physical array stays kInventorySlots.
  int capacity() const;
  int worn_count() const { return worn_count_; }
  ItemId worn_at(int i) const { return (i >= 0 && i < worn_count_) ? worn_[i] : 0; }

private:
  static const int kMaxWorn = 6;
  InvSlot slots_[kInventorySlots];
  ItemId worn_[kMaxWorn];
  int worn_count_;
};

} // namespace mdaz
