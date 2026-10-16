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

LootPile* LootSystem::spawn_table(Vec2 pos, Rng& rng, const ItemId* table, int table_len,
                                  int min_n, int max_n) {
  if (count_ >= kMaxLootPiles || !table || table_len <= 0) return 0;
  LootPile& p = piles_[count_++];
  p.pos = pos;
  p.item_count = 0;
  p.active = true;
  p.is_airdrop = false;
  if (min_n < 1) min_n = 1;
  if (max_n < min_n) max_n = min_n;
  const int n = rng.next_int(min_n, max_n);
  for (int i = 0; i < n; ++i) {
    ItemId id = table[rng.next_int(0, table_len - 1)];
    int qty = 1;
    if (id >= 11 && id <= 17) qty = rng.next_int(4, 16);
    if (id == 69 || id == 12 || id == 58) qty = rng.next_int(4, 14);
    push_item(p, id, qty);
  }
  return &p;
}

LootPile* LootSystem::spawn_random(Vec2 pos, Rng& rng) {
  // Ranked candidates from extract_spawn_drop_ids (civilian-ish band) + known commons
  // Top IDs near Spawn_drop (extract_spawn_drop_ids) filtered to valid catalog items
  static const ItemId kCommon[] = {
    1, 5, 7, 12, 15, 18, 26, 52, 100, 152, 200, 203, 257, 300, 350,
    2, 4, 6, 8, 27, 30, 54, 69, 201, 24, 50
  };
  LootPile* p = spawn_table(pos, rng, kCommon,
                            static_cast<int>(sizeof(kCommon) / sizeof(kCommon[0])), 1, 3);
  if (!p) return 0;
  if (rng.next_float() < 0.12f) {
    static const ItemId kRare[] = { 204, 205, 151, 170, 42, 43, 354 };
    push_item(*p, kRare[rng.next_int(0, static_cast<int>(sizeof(kRare) / sizeof(kRare[0])) - 1)], 1);
  }
  return p;
}

LootPile* LootSystem::spawn_item(Vec2 pos, ItemId id, int n) {
  if (count_ >= kMaxLootPiles) return 0;
  LootPile& p = piles_[count_++];
  p.pos = pos;
  p.item_count = 0;
  p.active = true;
  p.is_airdrop = false;
  push_item(p, id, n > 0 ? n : 1);
  return &p;
}

LootPile* LootSystem::spawn_airdrop(Vec2 pos, Rng& rng) {
  // Military / airdrop-ish IDs from catalog + extract hints
  static const ItemId kAir[] = {
    11, 12, 13, 14, 15, 18, 19, 28, 85, 151, 156, 157, 158, 159, 160, 168, 204, 400, 451
  };
  LootPile* p = spawn_table(pos, rng, kAir,
                            static_cast<int>(sizeof(kAir) / sizeof(kAir[0])), 3, 5);
  if (p) p->is_airdrop = true;
  return p;
}

LootPile* LootSystem::spawn_hospital(Vec2 pos, Rng& rng) {
  static const ItemId kMed[] = {
    18, 19, 28, 29, 45, 46, 18, 19, 28, 34, 265
  };
  return spawn_table(pos, rng, kMed,
                     static_cast<int>(sizeof(kMed) / sizeof(kMed[0])), 2, 4);
}

LootPile* LootSystem::spawn_gas_station(Vec2 pos, Rng& rng) {
  static const ItemId kGas[] = {
    53, 53, 59, 24, 50, 8, 30, 1, 200, 205
  };
  return spawn_table(pos, rng, kGas,
                     static_cast<int>(sizeof(kGas) / sizeof(kGas[0])), 1, 3);
}

LootPile* LootSystem::spawn_civilian(Vec2 pos, Rng& rng) {
  static const ItemId kCiv[] = {
    1, 2, 5, 8, 10, 18, 26, 30, 42, 54, 203, 300, 350, 24, 50
  };
  return spawn_table(pos, rng, kCiv,
                     static_cast<int>(sizeof(kCiv) / sizeof(kCiv[0])), 1, 3);
}

} // namespace mdaz
