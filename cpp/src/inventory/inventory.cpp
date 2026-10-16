#include "mdaz/inventory.hpp"

namespace mdaz {

Inventory::Inventory() { clear(); }

void Inventory::clear() {
  for (int i = 0; i < kInventorySlots; ++i) {
    slots_[i].id = 0;
    slots_[i].count = 0;
  }
}

InvSlot& Inventory::slot(int i) { return slots_[i]; }
const InvSlot& Inventory::slot(int i) const { return slots_[i]; }

bool Inventory::add_item(ItemId id, int count, const ItemDatabase& db) {
  if (id <= 0 || count <= 0) return false;
  const ItemDef* def = db.find(id);
  const int stack_max = def ? def->stack_max : 1;

  for (int i = 0; i < kInventorySlots && count > 0; ++i) {
    if (slots_[i].id == id && slots_[i].count < stack_max) {
      const int space = stack_max - slots_[i].count;
      const int take = count < space ? count : space;
      slots_[i].count += take;
      count -= take;
    }
  }
  for (int i = 0; i < kInventorySlots && count > 0; ++i) {
    if (slots_[i].empty()) {
      const int take = count < stack_max ? count : stack_max;
      slots_[i].id = id;
      slots_[i].count = take;
      count -= take;
    }
  }
  return count == 0;
}

bool Inventory::remove_at(int slot, int count) {
  if (slot < 0 || slot >= kInventorySlots) return false;
  if (slots_[slot].empty() || slots_[slot].count < count) return false;
  slots_[slot].count -= count;
  if (slots_[slot].count <= 0) {
    slots_[slot].id = 0;
    slots_[slot].count = 0;
  }
  return true;
}

bool Inventory::use_consumable(int slot_i, SurvivalState& survival, const ItemDatabase& db) {
  if (slot_i < 0 || slot_i >= kInventorySlots) return false;
  InvSlot& s = slots_[slot_i];
  if (s.empty()) return false;
  const ItemDef* def = db.find(s.id);
  if (!def) return false;

  if (def->category == ItemCat_Food) {
    survival.food += def->food_restore > 0.f ? def->food_restore : 15.f;
    if (survival.food > 100.f) survival.food = 100.f;
  } else if (def->category == ItemCat_Drink) {
    survival.water += def->water_restore > 0.f ? def->water_restore : 15.f;
    if (survival.water > 100.f) survival.water = 100.f;
  } else if (def->category == ItemCat_Medical) {
    if (def->name.find("Bandage") != std::string::npos || def->name.find("Rags") != std::string::npos) {
      survival.bleeding = 0.f;
    } else if (def->name.find("Blood") != std::string::npos || def->name.find("Saline") != std::string::npos) {
      survival.blood += 30.f;
      if (survival.blood > 100.f) survival.blood = 100.f;
    } else {
      survival.blood += 10.f;
      if (survival.blood > 100.f) survival.blood = 100.f;
    }
  } else {
    return false;
  }
  return remove_at(slot_i, 1);
}

} // namespace mdaz
