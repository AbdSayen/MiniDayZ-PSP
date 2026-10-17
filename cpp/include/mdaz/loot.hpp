#pragma once

#include "mdaz/rng.hpp"
#include "mdaz/types.hpp"

namespace mdaz {

// Ground loot pile — driven by original Spawn_drop / Check_loot / Trigger_spawn.
// Full weighted tables still being extracted; starter tables use verified item IDs.

struct LootPile {
  Vec2 pos;
  ItemId items[8];
  int counts[8];
  int item_count;
  bool active;
  bool is_airdrop;
};

static const int kMaxLootPiles = 96;

class LootSystem {
public:
  LootSystem();
  void clear();
  LootPile* spawn_random(Vec2 pos, Rng& rng);
  LootPile* spawn_airdrop(Vec2 pos, Rng& rng);
  LootPile* spawn_hospital(Vec2 pos, Rng& rng);
  LootPile* spawn_gas_station(Vec2 pos, Rng& rng);
  LootPile* spawn_civilian(Vec2 pos, Rng& rng);
  LootPile* spawn_military(Vec2 pos, Rng& rng);
  LootPile* spawn_bandit(Vec2 pos, Rng& rng);
  LootPile* spawn_secret(Vec2 pos, Rng& rng);
  LootPile* spawn_helicrash(Vec2 pos, Rng& rng);
  LootPile* spawn_police(Vec2 pos, Rng& rng);
  LootPile* spawn_firestation(Vec2 pos, Rng& rng);
  LootPile* spawn_church(Vec2 pos, Rng& rng);
  LootPile* spawn_bunker(Vec2 pos, Rng& rng);
  LootPile* spawn_item(Vec2 pos, ItemId id, int count);
  LootPile* spawn_table(Vec2 pos, Rng& rng, const ItemId* table, int table_len, int min_n, int max_n);
  int count() const { return count_; }
  LootPile* at(int i) { return &piles_[i]; }
  const LootPile* at(int i) const { return &piles_[i]; }

private:
  LootPile piles_[kMaxLootPiles];
  int count_;
  void push_item(LootPile& p, ItemId id, int n);
};

} // namespace mdaz
