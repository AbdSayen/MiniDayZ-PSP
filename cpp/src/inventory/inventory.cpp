#include "mdaz/inventory.hpp"

#include "mdaz/combat.hpp"

#include <string>

namespace mdaz {

Inventory::Inventory() { clear(); }

void Inventory::sync_clothing_gates(const ItemDatabase& db) {
  for (int i = 0; i < ClothingRegion_Count; ++i)
    clothing_.set_region_gate(static_cast<ClothingRegion>(i), 0);
  for (int wi = 0; wi < worn_count_; ++wi) {
    const ItemId wid = worn_[wi].id;
    const ItemDef* def = db.find(wid);
    const ClothingRegion r = clothing_region_for_worn_item(wid, def);
    if (r >= 0 && r < ClothingRegion_Count) clothing_.set_region_gate(r, 1);
  }
}

void Inventory::clear() {
  for (int i = 0; i < kInventorySlots; ++i) {
    slots_[i].id = 0;
    slots_[i].count = 0;
  }
  worn_count_ = 0;
  for (int i = 0; i < kMaxWorn; ++i) worn_[i] = WornSlot();
  clothing_.clear();
}

InvSlot& Inventory::slot(int i) { return slots_[i]; }
const InvSlot& Inventory::slot(int i) const { return slots_[i]; }

// C2 container sizes (data.js a198 "set size" when the piece is equipped):
// pants t185, outerwear t219, vest t267, backpack t189. Unknown ids -> 0 slots.
int clothing_container_slots(ItemId id) {
  switch (id) {
    case 300: return 2; case 301: return 3; case 302: return 1; case 303: return 3;
    case 304: return 2; case 305: return 2; case 306: return 3;
    case 350: return 0; case 351: case 352: return 1; case 353: return 2; case 354: return 1;
    case 355: return 3; case 356: case 357: return 2; case 358: return 1; case 359: return 2;
    case 360: return 2; case 361: return 1; case 362: return 1; case 363: return 2;
    case 364: return 3; case 365: return 1;
    case 401: return 1; case 402: return 2; case 403: return 3; case 405: return 2;
    case 450: return 3; case 451: return 5; case 452: return 7; case 453: return 2;
    case 454: return 3; case 455: return 5; case 456: return 0; case 457: return 6;
    default: return 0;
  }
}

// Worn region: one piece per region (head 25x, pants 30x, outerwear 35x, vest 40x,
// backpack 45x); other wearables are their own region.
int clothing_region(ItemId id) {
  if (id >= 250 && id < 500) return id / 50;
  return 1000 + id;
}

// C2: base "inventory" array t381 = 1 slot (hands; starter knife) + worn containers.
int Inventory::capacity() const {
  int cap = 1;
  for (int i = 0; i < worn_count_; ++i) cap += clothing_container_slots(worn_[i].id);
  if (cap > kInventorySlots) cap = kInventorySlots;
  return cap;
}

namespace {

int stack_max_for(const Inventory& inv, ItemId id, int count, const ItemDatabase& db) {
  if (id <= 0 || count <= 0) return 0;
  const ItemDef* def = db.find(id);
  int stack_max = def ? def->stack_max : 1;
  if (def && def->category == ItemCat_Ammo) {
    bool has_band = false;
    for (int i = 0; i < kInventorySlots; ++i) {
      if (inv.slot(i).id == 83 && inv.slot(i).count > 0) { has_band = true; break; }
    }
    for (int i = 0; i < inv.worn_count() && !has_band; ++i) {
      if (inv.worn_at(i) == 83) has_band = true;
    }
    if (has_band) stack_max *= 2;
  }
  return stack_max;
}

} // namespace

bool Inventory::can_add_item(ItemId id, int count, const ItemDatabase& db) const {
  if (id <= 0 || count <= 0) return false;
  const int stack_max = stack_max_for(*this, id, count, db);
  if (stack_max <= 0) return false;
  int remaining = count;
  const int cap = capacity();
  for (int i = 0; i < cap && remaining > 0; ++i) {
    const InvSlot& s = slots_[i];
    if (s.id == id && s.count < stack_max) {
      const int space = stack_max - s.count;
      remaining -= (remaining < space ? remaining : space);
    }
  }
  for (int i = 0; i < cap && remaining > 0; ++i) {
    if (slots_[i].empty()) {
      const int take = remaining < stack_max ? remaining : stack_max;
      remaining -= take;
    }
  }
  return remaining == 0;
}

bool Inventory::add_item(ItemId id, int count, const ItemDatabase& db) {
  if (id <= 0 || count <= 0) return false;
  if (!can_add_item(id, count, db)) return false;
  const ItemDef* def = db.find(id);
  int stack_max = def ? def->stack_max : 1;
  // Bandolier (83) doubles ammo stack size
  if (def && def->category == ItemCat_Ammo) {
    bool has_band = false;
    for (int i = 0; i < kInventorySlots; ++i) {
      if (slots_[i].id == 83 && slots_[i].count > 0) { has_band = true; break; }
    }
    for (int i = 0; i < worn_count_ && !has_band; ++i) {
      if (worn_[i].id == 83) has_band = true;
    }
    if (has_band) stack_max *= 2;
  }
  const int cap = capacity();

  for (int i = 0; i < cap && count > 0; ++i) {
    if (slots_[i].id == id && slots_[i].count < stack_max) {
      const int space = stack_max - slots_[i].count;
      const int take = count < space ? count : space;
      slots_[i].count += take;
      count -= take;
    }
  }
  for (int i = 0; i < cap && count > 0; ++i) {
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
  (void)rng; // reserved: C2 infection rolls on eat were not proven for cook-only raws
  if (slot_i < 0 || slot_i >= kInventorySlots) return false;
  InvSlot& s = slots_[slot_i];
  if (s.empty()) return false;
  const ItemDef* def = db.find(s.id);
  if (!def) return false;

  if (def->category == ItemCat_Food) {
    // Only apply IV13/IV14 literals from item_activate — no invented restores.
    if (def->food_restore <= 0.f && def->water_restore <= 0.f) return false;
    // Canned goods need a knife (stand-in for can opener / stone knife)
    const bool canned = def->name.find("Canned") != std::string::npos;
    if (canned) {
      bool knife = false;
      for (int i = 0; i < kInventorySlots; ++i) {
        const ItemId id = slots_[i].id;
        if (id == 42 || id == 43 || id == 44 || id == 205) { knife = true; break; }
      }
      if (!knife) return false;
    }
    if (def->food_restore > 0.f) {
      survival.food += def->food_restore;
      if (survival.food > 100.f) survival.food = 100.f;
    }
    if (def->water_restore > 0.f) {
      survival.water += def->water_restore;
      if (survival.water > 100.f) survival.water = 100.f;
    }
    if (def->heal_amount > 0.f) {
      survival.blood += def->heal_amount;
      if (survival.blood > 100.f) survival.blood = 100.f;
    }
  } else if (def->category == ItemCat_Drink) {
    if (def->water_restore <= 0.f && def->food_restore <= 0.f) return false;
    if (def->water_restore > 0.f) {
      survival.water += def->water_restore;
      if (survival.water > 100.f) survival.water = 100.f;
    }
    if (def->food_restore > 0.f) {
      survival.food += def->food_restore;
      if (survival.food > 100.f) survival.food = 100.f;
    }
    // Whiskey — C2 item_activate id25: AddInstanceVar IV16 +40 (temperature)
    if (def->id == 25) {
      survival.temperature += 40.f;
      if (survival.temperature > 100.f) survival.temperature = 100.f;
    }
  } else if (def->name.find("Heatpack") != std::string::npos || def->id == 34) {
    // item_activate id34: IV16 +25
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
      // C2: no IV13/14/15 on use — IV32 timer handled in Game (item_activate id29)
      return false;
    } else if (def->name.find("Morphine") != std::string::npos) {
      // No CompareParam 45 in C2 item_activate export — see item_activate_medicals_sverka.json
      return false;
    } else if (def->heal_amount > 0.f) {
      // Blood bag / saline use heal_amount from item_db (IV15)
      survival.blood += def->heal_amount;
      if (survival.blood > 100.f) survival.blood = 100.f;
    } else {
      return false; // no invented medical heal
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
    if (ids[i] > 0) {
      worn_[worn_count_].id = ids[i];
      worn_[worn_count_].iv6 = clothing_iv6_max(ids[i]);
      ++worn_count_;
    }
  }
}

bool Inventory::wear_clothing(int slot_i, const ItemDatabase& db) {
  if (slot_i < 0 || slot_i >= kInventorySlots) return false;
  if (slots_[slot_i].empty()) return false;
  const ItemDef* def = db.find(slots_[slot_i].id);
  if (!def) return false;
  const bool wearable = def->category == ItemCat_Clothing ||
                        def->category == ItemCat_Backpack || def->id == 250 ||
                        def->id == 83 || // Bandolier
                        def->name.find("mask") != std::string::npos ||
                        def->name.find("Mask") != std::string::npos ||
                        def->name.find("bag") != std::string::npos ||
                        def->name.find("Bag") != std::string::npos ||
                        def->name.find("backpack") != std::string::npos ||
                        def->name.find("Bandolier") != std::string::npos;
  if (!wearable) return false;
  const ItemId nid = slots_[slot_i].id;
  const int reg = clothing_region(nid);
  int wi = -1;
  for (int i = 0; i < worn_count_; ++i)
    if (clothing_region(worn_[i].id) == reg) { wi = i; break; }
  if (wi < 0 && worn_count_ >= kMaxWorn) return false;
  // Equipping moves the piece out of the bag; a piece already worn in that region
  // goes back into the vacated slot (swap).
  ItemId old_id = 0;
  if (wi >= 0) old_id = worn_[wi].id; else wi = worn_count_++;
  worn_[wi].id = nid;
  worn_[wi].iv6 = clothing_iv6_max(nid);
  if (slots_[slot_i].count > 1) {
    slots_[slot_i].count -= 1;
    if (old_id) { add_item(old_id, 1, db); }
  } else {
    slots_[slot_i] = InvSlot();
    if (old_id) { slots_[slot_i].id = old_id; slots_[slot_i].count = 1; }
  }
  return true;
}

float Inventory::clothing_insulation() const {
  // Per-piece warmth toward Day_cold reduction (ushanka/down/gorka warmer)
  float v = 0.f;
  for (int i = 0; i < worn_count_; ++i) {
    const ItemId id = worn_[i].id;
    float piece = 1.2f;
    if (id == 254 || id == 258) piece = 2.2f;      // Warm hat / Ushanka
    else if (id == 264) piece = 1.4f;                   // Police hat
    else if (id == 266) piece = 0.9f;                   // Bandana
    else if (id == 267) piece = 1.6f;                   // Helm
    else if (id == 363 || id == 364) piece = 2.8f; // Down / Hunter jacket
    else if (id == 355 || id == 303) piece = 2.4f; // Gorka jacket / pants
    else if (id == 353) piece = 1.8f;              // Raincoat (wind block)
    else if (id == 354 || id == 365 || id == 356 || id == 357 ||
             id == 358 || id == 359 || id == 360 || id == 361 || id == 362)
      piece = 2.0f; // Jacket/sweater/hoodie/coat family
    else if (id == 306 || id == 300 || id == 301 || id == 302 || id == 304 || id == 305)
      piece = 1.5f; // pants / cargo
    else if (id == 350 || id == 351 || id == 352) piece = 1.1f; // t-shirts / thin tops
    else if (id == 250) piece = 0.8f;              // Gasmask (little warmth)
    else if (id == 251 || id == 263) piece = 1.0f; // helmets — little warmth
    v += piece;
  }
  if (v > 12.f) v = 12.f;
  return v;
}

float Inventory::armor_damage_mul() const {
  float mul = 1.f;
  for (int i = 0; i < worn_count_; ++i) {
    const ItemId id = worn_[i].id;
    if (id <= 0 || worn_[i].iv6 <= 0.f) continue;
    // Helmets
    if (id == 251 || id == 263) mul *= 0.85f;      // Army / Gorka
    else if (id == 252 || id == 253) mul *= 0.9f;  // Moto
    else if (id == 262) mul *= 0.88f;              // Welding mask
    // Vests
    else if (id == 400 || id == 404) mul *= 0.65f; // Bulletproof / Kevlar
    else if (id == 402 || id == 403) mul *= 0.75f; // Assault / High capacity
    else if (id == 401 || id == 405) mul *= 0.8f;  // Press / Soviet
  }
  if (mul < 0.35f) mul = 0.35f;
  return mul;
}

} // namespace mdaz
