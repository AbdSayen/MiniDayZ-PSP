#pragma once

#include "mdaz/rng.hpp"
#include "mdaz/types.hpp"

namespace mdaz {

// Ground loot pile — driven by original Spawn_drop / Check_loot / Trigger_spawn.
// Full weighted tables still being extracted; starter tables use verified item IDs.

// Respawn table kind (LOOT_respawn → Trigger_spawn at pile).
enum LootPileKind : unsigned char {
  LootKind_Generic = 0,
  LootKind_Civilian,
  LootKind_Military,
  LootKind_Hospital,
  LootKind_Gas,
  LootKind_Helicrash,
  LootKind_Police,
  LootKind_Fire,
  LootKind_Church,
  LootKind_Secret,
  LootKind_Airdrop,
};

struct LootPile {
  Vec2 pos;
  ItemId items[8];
  int counts[8];
  int item_count;
  bool active;
  bool is_airdrop;
  bool quest_reward_case; // Quest_spawn_reward_box ground sprite
  LootPileKind kind;
  float respawn_t;       // seconds until pile respawns after depleted (LOOT_respawn 3/4/5)
  unsigned char respawn_sec; // last stagger duration (3, 4, or 5)
};

static const int kMaxLootPiles = 96;

class LootSystem {
public:
  LootSystem();
  void clear();
  LootPile* spawn_random(Vec2 pos, Rng& rng);
  LootPile* spawn_airdrop(Vec2 pos, Rng& rng, const struct WeaponUnlockState* unlocks = 0);
  LootPile* spawn_hospital(Vec2 pos, Rng& rng);
  LootPile* spawn_gas_station(Vec2 pos, Rng& rng);
  LootPile* spawn_civilian(Vec2 pos, Rng& rng);
  LootPile* spawn_military(Vec2 pos, Rng& rng, const struct WeaponUnlockState* unlocks = 0);
  LootPile* spawn_bandit(Vec2 pos, Rng& rng);
  LootPile* spawn_secret(Vec2 pos, Rng& rng);
  LootPile* spawn_helicrash(Vec2 pos, Rng& rng);
  LootPile* spawn_police(Vec2 pos, Rng& rng);
  LootPile* spawn_firestation(Vec2 pos, Rng& rng);
  LootPile* spawn_church(Vec2 pos, Rng& rng);
  LootPile* spawn_bunker(Vec2 pos, Rng& rng);
  LootPile* spawn_item(Vec2 pos, ItemId id, int count);
  // Quest_spawn_reward_box — quest_case sprite + item inside.
  LootPile* spawn_quest_reward(Vec2 pos, ItemId id, int count);
  // C2 Spawn_drop_from_player — item id + count at player position (obj172 XY).
  LootPile* spawn_drop_from_player(Vec2 player_pos, ItemId id, int count);
  LootPile* spawn_table(Vec2 pos, Rng& rng, const ItemId* table, int table_len, int min_n, int max_n);
  int count() const { return count_; }
  LootPile* at(int i) { return &piles_[i]; }
  const LootPile* at(int i) const { return &piles_[i]; }
  // Restore piles from save (Load_loot subset); clears existing piles first.
  void load_from_save(const struct SavedLootPile* piles, int n);
  // Per-pile LOOT_respawn stagger (obj209 anim 3/4/5 → wait 3/4/5s then Trigger_spawn).
  void schedule_respawn(LootPile& p);
  void tick_respawn(float dt, Rng& rng);

private:
  LootPile piles_[kMaxLootPiles];
  int count_;
  void push_item(LootPile& p, ItemId id, int n);
};

} // namespace mdaz
