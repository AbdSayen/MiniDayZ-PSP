#pragma once

#include "mdaz/item_db.hpp"
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
  bool use_consumable(int slot, SurvivalState& survival, const ItemDatabase& db);

  InvSlot& slot(int i);
  const InvSlot& slot(int i) const;
  int slot_count() const { return kInventorySlots; }

private:
  InvSlot slots_[kInventorySlots];
};

} // namespace mdaz
