#include "mdaz/inventory.hpp"

#include "mdaz/combat.hpp"
#include "mdaz/rng.hpp"

namespace mdaz {

namespace {

bool is_helmet_id(ItemId id) {
  return id == 251 || id == 252 || id == 253 || id == 259 || id == 262 || id == 263 ||
         id == 265 || id == 267;
}

bool is_vest_id(ItemId id) {
  return id >= 400 && id <= 405;
}

bool is_outerwear_id(ItemId id) {
  return (id >= 350 && id <= 365) || id == 353;
}

bool is_pants_id(ItemId id) {
  return id >= 300 && id <= 306;
}

bool is_backpack_id(ItemId id) {
  return id == 454 || id == 455 || (id >= 450 && id <= 457);
}

// C2 PickByUID on player clothing: 8 helmet, 10 vest, 18 outer, 12 pants, 20 backpack.
int worn_index_for_dmg_randomizer(const Inventory& inv, int dmg_randomizer) {
  auto matches = [&](bool (*pred)(ItemId)) -> int {
    for (int i = 0; i < inv.worn_count(); ++i) {
      const ItemId id = inv.worn_at(i);
      if (id > 0 && pred(id)) return i;
    }
    return -1;
  };
  switch (dmg_randomizer) {
  case 3:
    return matches(is_helmet_id);
  case 4:
    return matches(is_vest_id);
  case 5:
    return matches(is_outerwear_id);
  case 6:
    return matches(is_pants_id);
  case 7:
    return matches(is_backpack_id);
  default:
    return -1;
  }
}

void subtract_iv6(WornSlot& w, float amount) {
  if (w.id <= 0 || amount <= 0.f) return;
  w.iv6 -= amount;
  if (w.iv6 <= 0.f) w.iv6 = 0.f;
}

} // namespace

float Inventory::worn_iv6(int i) const {
  if (i < 0 || i >= worn_count_) return 0.f;
  return worn_[i].iv6;
}

void Inventory::restore_worn_iv6(const ItemId* ids, const float* iv6, int count) {
  worn_count_ = 0;
  if (!ids || count <= 0) { clothing_.clear(); resync_sizes(); return; }
  if (count > kMaxWorn) count = kMaxWorn;
  for (int i = 0; i < count; ++i) {
    if (ids[i] <= 0) continue;
    worn_[worn_count_].id = ids[i];
    worn_[worn_count_].iv6 =
        (iv6 && iv6[i] > 0.f) ? iv6[i] : clothing_iv6_max(ids[i]);
    ++worn_count_;
  }
  clothing_.clear();
  resync_sizes();
}

void Inventory::apply_hit_armor_wear(Rng& rng, bool bullet_hit, bool heavy_melee_wear,
                                   bool skip_vest_slot_if_gasmask) {
  const int roll = rng.next_int(1, 7);
  if (roll < 3) return;
  if (roll == 4 && skip_vest_slot_if_gasmask) return;
  const int wi = worn_index_for_dmg_randomizer(*this, roll);
  if (wi < 0) return;
  const float lo = bullet_hit ? 10.f : 10.f;
  const float hi = (bullet_hit || !heavy_melee_wear) ? 15.f : 25.f;
  subtract_iv6(worn_[wi], rng.range(lo, hi));
}

} // namespace mdaz
