#include "mdaz/inventory.hpp"
#include "mdaz/loot.hpp"

#include "mdaz/combat.hpp"

#include <string>

namespace mdaz {

Inventory::Inventory() { clear(); }

// Check_space chain region of a worn piece: pants 30x, outerwear 35x, vest 40x, backpack 45x
// (same id/50 grouping as clothing_region()).
static ClothingRegion chain_region(ItemId id) {
  if (id < 300 || id >= 500) return ClothingRegion_Count;
  return static_cast<ClothingRegion>(id / 50 - 6);
}

void Inventory::resync_sizes() {
  for (int i = 0; i < ClothingRegion_Count; ++i)
    clothing_.set_region_size(static_cast<ClothingRegion>(i), 0);
  for (int wi = 0; wi < worn_count_; ++wi) {
    const ClothingRegion r = chain_region(worn_[wi].id);
    if (r >= 0 && r < ClothingRegion_Count)
      clothing_.set_region_size(r, worn_[wi].cut ? 0 : clothing_container_slots(worn_[wi].id));
  }
}

ClothingRegion Inventory::chain_region_of(ItemId id) { return chain_region(id); }

void Inventory::cut_region(ClothingRegion r) {
  for (int wi = 0; wi < worn_count_; ++wi) {
    if (chain_region(worn_[wi].id) != r) continue;
    GearBox gone;
    clothing_.take_box(r, &gone);   // Arr.Clear
    worn_[wi].cut = true;           // SetSize 0
  }
  resync_sizes();
}

void Inventory::spill(ItemId id, int count, float iv6) {
  if (id <= 0 || count < 0 || spill_n_ >= kInventorySlots) return;
  spill_[spill_n_].id = id;
  spill_[spill_n_].count = count;
  spill_[spill_n_].iv6 = c2_item_has_iv6(id) ? (iv6 < -1.5f ? 100.f : iv6) : -1.f;
  ++spill_n_;
}

int Inventory::take_spill(InvSlot* out, int max) {
  const int n = spill_n_;
  for (int i = 0; i < n && i < max; ++i) if (out) out[i] = spill_[i];
  spill_n_ = 0;
  return n;
}

void Inventory::clear() {
  base_ = InvSlot();
  spill_n_ = 0;
  worn_count_ = 0;
  for (int i = 0; i < kMaxWorn; ++i) worn_[i] = WornSlot();
  clothing_.clear();
}

static InvSlot g_inv_dummy;

InvSlot& Inventory::slot(int i) {
  if (i == 0) return base_;
  int k = i - 1;
  if (k >= 0) {
    for (int r = 0; r < ClothingRegion_Count; ++r) {
      const ClothingRegion cr = static_cast<ClothingRegion>(r);
      const int n = clothing_.region_size(cr);
      if (k < n) return clothing_.slot_mut(cr, k);
      k -= n;
    }
  }
  g_inv_dummy = InvSlot();
  return g_inv_dummy;
}
const InvSlot& Inventory::slot(int i) const { return const_cast<Inventory*>(this)->slot(i); }

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
  for (int r = 0; r < ClothingRegion_Count; ++r)
    cap += clothing_.region_size(static_cast<ClothingRegion>(r));
  if (cap > kInventorySlots) cap = kInventorySlots;
  return cap;
}

int Inventory::flat_index(ClothingRegion r, int sub) const {
  if (r < 0 || r >= ClothingRegion_Count || sub < 0 || sub >= clothing_.region_size(r)) return -1;
  int idx = 1;
  for (int k = 0; k < r; ++k) idx += clothing_.region_size(static_cast<ClothingRegion>(k));
  return idx + sub < kInventorySlots ? idx + sub : -1;
}

// C2 Check_space_inventory -> pants -> outerwear -> vest -> backpack: each step only
// tests Contains(0) and writes id+count into IndexOf(0). No merge into existing stacks,
// no split by stack size (stack limits apply only to Combine_ammo).
bool Inventory::can_add_item(ItemId id, int count, const ItemDatabase& db) const {
  (void)db;
  if (id <= 0 || count < 0) return false;
  const int cap = capacity();
  for (int i = 0; i < cap; ++i) if (slot(i).empty()) return true;
  return false;
}

bool Inventory::add_item(ItemId id, int count, const ItemDatabase& db) {
  if (!can_add_item(id, count, db)) return false;
  return add_item_new_slot(id, count);
}

// C2 sub_menu "Combine" -> Combine_ammo(param0 = source amount, param1 = target uid):
//   target X >= stack: return param0 (nothing moves);
//   target X <  stack: X += param0, return X - stack; then X > stack -> X = stack.
// Caller: return > 0 -> source amount = return; return <= 0 -> source id/amount = 0.
int Inventory::combine_slots(int src, int dst, const ItemDatabase& db) {
  (void)db;
  const int cap = capacity();
  if (src < 0 || dst < 0 || src >= cap || dst >= cap || src == dst) return -1;
  InvSlot& a = slot(src);
  InvSlot& b = slot(dst);
  if (a.empty() || b.empty() || a.id != b.id || !c2_combine_menu(a.id)) return -1;
  const int stack = c2_combine_stack(a.id);
  int ret;
  if (b.count >= stack) ret = a.count;
  else {
    b.count += a.count;
    ret = b.count - stack;
    if (b.count > stack) b.count = stack;
  }
  if (ret > 0) a.count = ret;
  else { a.id = 0; a.count = 0; }
  return ret;
}

bool is_gear_item(ItemId id) { return id >= 250 && id < 500; }

bool Inventory::add_item_new_slot(ItemId id, int count, float iv6) {
  if (id <= 0 || count < 0) return false;   // count 0 = empty bottle/canteen state
  const int cap = capacity();
  for (int i = 0; i < cap; ++i) {
    if (slot(i).empty()) {
      slot(i).id = id;
      slot(i).count = count;
      // t909 instance: its own IV6, or the layout default IV6 = 100 (every t909 type).
      slot(i).iv6 = !c2_item_has_iv6(id) ? -1.f : (iv6 < -1.5f ? 100.f : iv6);
      return true;
    }
  }
  return false;
}

bool Inventory::equip_gear(ItemId id, ItemId* old_out, const GearBox* new_box, GearBox* old_box,
                           float new_iv6, float* old_iv6) {
  if (old_out) *old_out = 0;
  if (old_iv6) *old_iv6 = -1.f;
  if (old_box) old_box->clear();
  if (!is_gear_item(id)) return false;
  const int reg = clothing_region(id);
  int wi = -1;
  for (int i = 0; i < worn_count_; ++i)
    if (clothing_region(worn_[i].id) == reg) { wi = i; break; }
  if (wi < 0) {
    if (worn_count_ >= kMaxWorn) return false;
    wi = worn_count_++;
  } else {
    if (old_out) *old_out = worn_[wi].id;
    if (old_iv6) *old_iv6 = worn_[wi].iv6;
  }
  const ClothingRegion cr = chain_region(id);
  if (cr >= 0 && cr < ClothingRegion_Count) {
    GearBox tmp;
    clothing_.take_box(cr, &tmp);  // Drop_<region>: array leaves with the old piece
    if (old_box) *old_box = tmp;
    clothing_.put_box(cr, new_box);
  }
  worn_[wi].id = id;
  worn_[wi].cut = false;
  worn_[wi].iv6 = new_iv6 < -1.5f ? clothing_iv6_max(id) : (new_iv6 < 0.f ? 0.f : new_iv6);
  resync_sizes();
  return true;
}

bool Inventory::remove_at(int slot_i, int count) {
  if (slot_i < 0 || slot_i >= kInventorySlots) return false;
  if (slot(slot_i).empty() || slot(slot_i).count < count) return false;
  slot(slot_i).count -= count;
  if (slot(slot_i).count <= 0) slot(slot_i) = InvSlot();
  return true;
}

bool Inventory::use_consumable(int slot_i, SurvivalState& survival, const ItemDatabase& db, Rng* rng) {
  if (slot_i < 0 || slot_i >= kInventorySlots) return false;
  InvSlot& s = slot(slot_i);
  // C2 eat: raw steak 60, rabbit meat 71, herring/salmon/ruffe/perch 91-94 ->
  // choose(1,2)==1 sets IV34 (sick).
  const ItemId raw_id = s.id;
  const bool raw_sick_food = raw_id == 60 || raw_id == 71 || (raw_id >= 91 && raw_id <= 94);
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
        const ItemId id = slot(i).id;
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
    if (raw_sick_food && rng && rng->next_int(1, 2) == 1) survival.infected = true;
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
  if (!ids || count <= 0) { clothing_.clear(); resync_sizes(); return; }
  if (count > kMaxWorn) count = kMaxWorn;
  for (int i = 0; i < count; ++i) {
    if (ids[i] > 0) {
      worn_[worn_count_].id = ids[i];
      worn_[worn_count_].cut = false;
      worn_[worn_count_].iv6 = clothing_iv6_max(ids[i]);
      ++worn_count_;
    }
  }
  clothing_.clear();
  resync_sizes();
}

bool Inventory::wear_clothing(int slot_i, const ItemDatabase& db) {
  if (slot_i < 0 || slot_i >= kInventorySlots) return false;
  if (slot(slot_i).empty()) return false;
  const ItemDef* def = db.find(slot(slot_i).id);
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
  const ItemId nid = slot(slot_i).id;
  const int reg = clothing_region(nid);
  int wi = -1;
  for (int i = 0; i < worn_count_; ++i)
    if (clothing_region(worn_[i].id) == reg) { wi = i; break; }
  if (wi < 0 && worn_count_ >= kMaxWorn) return false;
  // Equipping moves the piece out of the bag; a piece already worn in that region
  // goes back into the vacated slot (swap). Port-only path (C2 wears gear from the ground via
  // Pick_player_item); the old piece's pockets cannot live in a bag slot, so they spill.
  ItemId old_id = 0;
  float old_iv6 = -1.f;
  const float new_iv6 = slot(slot_i).iv6;
  if (wi >= 0) { old_id = worn_[wi].id; old_iv6 = worn_[wi].iv6; } else wi = worn_count_++;
  if (slot(slot_i).count > 1) slot(slot_i).count -= 1;
  else slot(slot_i) = InvSlot();
  const ClothingRegion cr = chain_region(nid);
  if (cr >= 0 && cr < ClothingRegion_Count) {
    GearBox old;
    clothing_.take_box(cr, &old);
    for (int k = 0; k < kClothingContainerSlots; ++k)
      if (!old.s[k].empty()) spill(old.s[k].id, old.s[k].count, old.s[k].iv6);
  }
  worn_[wi].id = nid;
  worn_[wi].cut = false;
  worn_[wi].iv6 = new_iv6 >= 0.f ? new_iv6 : clothing_iv6_max(nid);   // the instance's IV6
  resync_sizes();
  (void)db;
  if (old_id && !add_item_new_slot(old_id, 1, old_iv6)) spill(old_id, 1, old_iv6);
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
