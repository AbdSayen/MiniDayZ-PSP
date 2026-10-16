#include "mdaz/loot.hpp"

namespace mdaz {

LootSystem::LootSystem() : count_(0) {}

void LootSystem::clear() { count_ = 0; }

void LootSystem::push_item(LootPile& p, ItemId id, int n) {
  if (p.item_count >= 8) return;
  p.items[p.item_count] = id;
  p.counts[p.item_count] = n;
  p.item_count++;
}

LootPile* LootSystem::spawn_random(Vec2 pos, Rng& rng) {
  if (count_ >= kMaxLootPiles) return 0;
  LootPile& p = piles_[count_++];
  p.pos = pos;
  p.item_count = 0;
  p.active = true;

  // Common civilian loot IDs from l_eng_items.xml — weights TBD from Spawn_drop.
  static const ItemId kCommon[] = {
    1, 2, 6, 8, 18, 26, 54, 4, 27, 30
  };
  const int n = 1 + rng.next_int(0, 2);
  for (int i = 0; i < n; ++i) {
    ItemId id = kCommon[rng.next_int(0, static_cast<int>(sizeof(kCommon) / sizeof(kCommon[0])) - 1)];
    push_item(p, id, 1);
  }
  return &p;
}

LootPile* LootSystem::spawn_airdrop(Vec2 pos, Rng& rng) {
  if (count_ >= kMaxLootPiles) return 0;
  LootPile& p = piles_[count_++];
  p.pos = pos;
  p.item_count = 0;
  p.active = true;
  // Higher-tier IDs present in 1.4.1 catalog
  static const ItemId kAir[] = { 85, 158, 159, 18, 19, 11, 14, 400, 451 };
  const int n = 3 + rng.next_int(0, 2);
  for (int i = 0; i < n; ++i) {
    ItemId id = kAir[rng.next_int(0, static_cast<int>(sizeof(kAir) / sizeof(kAir[0])) - 1)];
    push_item(p, id, id >= 11 && id <= 17 ? rng.next_int(5, 20) : 1);
  }
  return &p;
}

} // namespace mdaz
