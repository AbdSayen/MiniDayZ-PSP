#include "mdaz/inventory.hpp"

namespace mdaz {

Inventory::Inventory() { clear(); }

void Inventory::clear() {
  for (int i = 0; i < kInventorySlots; ++i) {
    slots_[i].id = 0;
    slots_[i].count = 0;
  }
  worn_count_ = 0;
  for (int i = 0; i < kMaxWorn; ++i) worn_[i] = 0;
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

bool Inventory::use_consumable(int slot_i, SurvivalState& survival, const ItemDatabase& db, Rng* rng) {
  if (slot_i < 0 || slot_i >= kInventorySlots) return false;
  InvSlot& s = slots_[slot_i];
  if (s.empty()) return false;
  const ItemDef* def = db.find(s.id);
  if (!def) return false;

  if (def->category == ItemCat_Food) {
    float restore = def->food_restore > 0.f ? def->food_restore : 15.f;
    const bool raw = def->name.find("Raw") != std::string::npos ||
                     def->name == "Perch" || def->name == "Rabbit meat";
    if (raw) restore *= 0.5f;
    survival.food += restore;
    if (survival.food > 100.f) survival.food = 100.f;
    if (raw && rng && !survival.infected && rng->next_float() < 0.35f) {
      survival.infected = true;
    }
  } else if (def->category == ItemCat_Drink) {
    survival.water += def->water_restore > 0.f ? def->water_restore : 15.f;
    if (survival.water > 100.f) survival.water = 100.f;
  } else if (def->name.find("Heatpack") != std::string::npos) {
    survival.temperature += 25.f;
    if (survival.temperature > 100.f) survival.temperature = 100.f;
  } else if (def->id == 20 || def->id == 265) {
    // Flare / NVG handled by Game (side effects beyond SurvivalState)
    return false;
  } else if (def->category == ItemCat_Medical) {
    if (def->name.find("Bandage") != std::string::npos || def->name.find("Rags") != std::string::npos) {
      survival.bleeding = 0.f;
    } else if (def->name.find("Tetracycline") != std::string::npos) {
      survival.infected = false;
    } else if (def->name.find("Vitamins") != std::string::npos) {
      survival.blood += def->heal_amount > 0.f ? def->heal_amount : 5.f;
      if (survival.blood > 100.f) survival.blood = 100.f;
      // Mild infection clear — vitamins are not full antibiotics
      if (survival.infected && rng && rng->next_float() < 0.35f) survival.infected = false;
    } else if (def->name.find("Morphine") != std::string::npos) {
      survival.blood += 5.f;
      if (survival.blood > 100.f) survival.blood = 100.f;
      if (survival.bleeding > 0.f) survival.bleeding = 0.f;
    } else if (def->heal_amount > 0.f) {
      // Blood bag / saline use heal_amount from item_db
      survival.blood += def->heal_amount;
      if (survival.blood > 100.f) survival.blood = 100.f;
    } else {
      survival.blood += 10.f;
      if (survival.blood > 100.f) survival.blood = 100.f;
    }
  } else if (def->category == ItemCat_Clothing || def->id == 250) {
    return wear_clothing(slot_i, db);
  } else {
    return false;
  }
  return remove_at(slot_i, 1);
}

void Inventory::restore_worn(const ItemId* ids, int count) {
  worn_count_ = 0;
  if (!ids || count <= 0) return;
  if (count > kMaxWorn) count = kMaxWorn;
  for (int i = 0; i < count; ++i) {
    if (ids[i] > 0) worn_[worn_count_++] = ids[i];
  }
}

bool Inventory::wear_clothing(int slot_i, const ItemDatabase& db) {
  if (slot_i < 0 || slot_i >= kInventorySlots) return false;
  if (slots_[slot_i].empty()) return false;
  const ItemDef* def = db.find(slots_[slot_i].id);
  if (!def || def->category != ItemCat_Clothing) return false;
  if (worn_count_ >= kMaxWorn) return false;
  // Avoid duplicates
  for (int i = 0; i < worn_count_; ++i) {
    if (worn_[i] == slots_[slot_i].id) return true;
  }
  worn_[worn_count_++] = slots_[slot_i].id;
  return true;
}

float Inventory::clothing_insulation() const {
  // Stub: each worn piece gives 1.5 toward Day_cold reduction
  float v = static_cast<float>(worn_count_) * 1.5f;
  if (v > 9.f) v = 9.f;
  return v;
}

} // namespace mdaz
