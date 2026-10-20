#include "mdaz/loot.hpp"
#include "mdaz/save_game.hpp"
#include "mdaz/weapon_unlocks.hpp"

namespace mdaz {

namespace {

// Weighted entries from tools/extract_spawn_drop_ids → docs_gen/spawn_drop_ids.json
// (literal counts near Spawn_drop in Game_events). Not full C2 RNG, but frequency-ranked.
struct DropW { ItemId id; int w; };

const DropW kSpawnDropW[] = {
  { 15, 213 }, { 100, 195 }, { 1, 72 }, { 300, 42 }, { 350, 34 }, { 7, 16 },
  { 12, 10 }, { 203, 8 }, { 52, 7 }, { 200, 7 }, { 257, 7 }, { 5, 6 },
  { 18, 6 }, { 26, 6 }, { 152, 6 }, { 163, 6 }, { 253, 6 }, { 256, 6 },
  { 261, 6 }, { 306, 6 }, { 361, 6 }, { 362, 6 }, { 400, 6 }, { 454, 6 },
  { 250, 5 }, { 266, 5 }, { 302, 5 }, { 305, 5 }, { 354, 5 }, { 3, 4 },
  { 4, 4 }, { 9, 4 }, { 10, 4 }, { 13, 4 }, { 42, 4 }, { 44, 4 },
  { 6, 3 }, { 8, 3 }, { 11, 3 }, { 353, 3 },
};
const int kSpawnDropN = static_cast<int>(sizeof(kSpawnDropW) / sizeof(kSpawnDropW[0]));

ItemId pick_weighted(Rng& rng, const DropW* tab, int n) {
  int sum = 0;
  for (int i = 0; i < n; ++i) sum += tab[i].w;
  if (sum <= 0) return tab[0].id;
  int roll = rng.next_int(0, sum - 1);
  for (int i = 0; i < n; ++i) {
    roll -= tab[i].w;
    if (roll < 0) return tab[i].id;
  }
  return tab[n - 1].id;
}

struct SpawnDropCount { int id, kind, a, b, farmer, n; int list[5]; };
#include "spawn_drop_count.inc"
struct SpawnDropIv6 { int id, n; int list[6]; };
#include "spawn_drop_iv6.inc"

} // namespace

// C2 Spawn_drop IV1 (array count): stack size or item state; ids without an IV1 row -> 1.
int c2_spawn_drop_iv1(ItemId id, Rng& rng, bool farmer) {
  for (unsigned i = 0; i < sizeof(kSpawnDropCount) / sizeof(kSpawnDropCount[0]); ++i) {
    const SpawnDropCount& r = kSpawnDropCount[i];
    if (r.id != id) continue;
    int n = r.kind == 1 ? rng.next_int(r.a, r.b - 1)     // int(random(a, b))
                        : r.list[rng.next_int(0, r.n - 1)];
    if (r.farmer && farmer) n += r.farmer;
    return n;
  }
  return 1;
}

bool c2_item_has_iv6(ItemId id) {
  for (unsigned i = 0; i < sizeof(kSpawnDropIv6) / sizeof(kSpawnDropIv6[0]); ++i)
    if (kSpawnDropIv6[i].id == id) return true;
  return false;
}

float c2_spawn_drop_iv6(ItemId id, Rng& rng) {
  for (unsigned i = 0; i < sizeof(kSpawnDropIv6) / sizeof(kSpawnDropIv6[0]); ++i) {
    const SpawnDropIv6& r = kSpawnDropIv6[i];
    if (r.id == id) return static_cast<float>(r.list[rng.next_int(0, r.n - 1)]);
  }
  return -1.f;
}

namespace {
bool g_loot_farmer = false;
int ammo_qty(ItemId id, Rng& rng) { return c2_spawn_drop_iv1(id, rng, g_loot_farmer); }

const float kLootRespawnStaggerSec[] = {3.f, 4.f, 5.f};
const char kC2_LOOT_respawn[] = "LOOT_respawn";
void touch_loot_respawn_audit() { (void)kC2_LOOT_respawn; }

static unsigned char next_loot_stagger_sec(unsigned char prev) {
  if (prev == 3) return 4;
  if (prev == 4) return 5;
  return 3;
}

} // namespace

LootSystem::LootSystem() : count_(0), iv6_rng_(0x6d64617au) { touch_loot_respawn_audit(); }

void LootSystem::set_farmer(bool on) { g_loot_farmer = on; }

void LootSystem::clear() { count_ = 0; }

void LootSystem::load_from_save(const SavedLootPile* piles, int n, const float (*iv6)[8]) {
  clear();
  if (!piles || n <= 0) return;
  if (n > kMaxLootPiles) n = kMaxLootPiles;
  for (int i = 0; i < n; ++i) {
    const SavedLootPile& s = piles[i];
    if (s.item_count <= 0) continue;
    if (count_ >= kMaxLootPiles) break;
    LootPile& p = piles_[count_++];
    for (int bx = 0; bx < 8; ++bx) p.boxes[bx].clear();
    p.pos = Vec2(s.x, s.y);
    p.item_count = 0;
    p.active = true;
    p.is_airdrop = s.is_airdrop != 0;
    p.quest_reward_case = false;
    p.kind = LootKind_Generic;
    p.respawn_t = 0.f;
    p.respawn_sec = 0;
    const int ic = s.item_count > 8 ? 8 : s.item_count;
    for (int j = 0; j < ic; ++j) {
      if (s.items[j] <= 0 || s.counts[j] < 0) continue;
      push_item(p, s.items[j], s.counts[j], iv6 ? iv6[i][j] : kIv6Roll);
    }
    if (p.item_count <= 0) {
      --count_;
      continue;
    }
  }
}

void LootSystem::push_item(LootPile& p, ItemId id, int n, float iv6) {
  if (p.item_count >= 8) return;
  p.items[p.item_count] = id;
  p.counts[p.item_count] = n;
  // Spawn_drop: IV6 = choose(...); "param3 != -1" -> IV6 = param3 (an instance's own IV6).
  p.iv6[p.item_count] = (iv6 == kIv6Roll) ? c2_spawn_drop_iv6(id, iv6_rng_)
                                          : (c2_item_has_iv6(id) ? iv6 : -1.f);
  p.item_count++;
}

static bool loot_unlock_allows(const WeaponUnlockState* unlocks, ItemId id) {
  if (!unlocks) return true;
  return weapon_unlock_allows_loot_item(*unlocks, id);
}

void LootSystem::push_item_unlock(LootPile& p, ItemId id, int n, const WeaponUnlockState* unlocks) {
  if (!loot_unlock_allows(unlocks, id)) return;
  push_item(p, id, n);
}

LootPile* LootSystem::spawn_table(Vec2 pos, Rng& rng, const ItemId* table, int table_len,
                                  int min_n, int max_n) {
  if (count_ >= kMaxLootPiles || !table || table_len <= 0) return 0;
  LootPile& p = piles_[count_++];
    for (int bx = 0; bx < 8; ++bx) p.boxes[bx].clear();
  p.pos = pos;
  p.item_count = 0;
  p.active = true;
  p.is_airdrop = false;
  p.quest_reward_case = false;
  p.kind = LootKind_Generic;
  p.respawn_t = 0.f;
  p.respawn_sec = 0;
  if (min_n < 1) min_n = 1;
  if (max_n < min_n) max_n = min_n;
  const int n = rng.next_int(min_n, max_n);
  for (int i = 0; i < n; ++i) {
    ItemId id = table[rng.next_int(0, table_len - 1)];
    push_item(p, id, ammo_qty(id, rng));
  }
  return &p;
}

LootPile* LootSystem::spawn_random(Vec2 pos, Rng& rng) {
  if (count_ >= kMaxLootPiles) return 0;
  LootPile& p = piles_[count_++];
    for (int bx = 0; bx < 8; ++bx) p.boxes[bx].clear();
  p.pos = pos;
  p.item_count = 0;
  p.active = true;
  p.is_airdrop = false;
  p.quest_reward_case = false;
  p.kind = LootKind_Generic;
  p.respawn_t = 0.f;
  p.respawn_sec = 0;
  const int n = rng.next_int(1, 3);
  for (int i = 0; i < n; ++i) {
    const ItemId id = pick_weighted(rng, kSpawnDropW, kSpawnDropN);
    push_item(p, id, ammo_qty(id, rng));
  }
  if (rng.next_float() < 0.08f) {
    static const ItemId kRare[] = { 204, 205, 151, 170, 42, 43, 354, 163 };
    push_item(p, kRare[rng.next_int(0, static_cast<int>(sizeof(kRare) / sizeof(kRare[0])) - 1)], 1);
  }
  return &p;
}

LootPile* LootSystem::spawn_item(Vec2 pos, ItemId id, int n) {
  if (count_ >= kMaxLootPiles) return 0;
  LootPile& p = piles_[count_++];
    for (int bx = 0; bx < 8; ++bx) p.boxes[bx].clear();
  p.pos = pos;
  p.item_count = 0;
  p.active = true;
  p.is_airdrop = false;
  p.quest_reward_case = false;
  p.kind = LootKind_Generic;
  p.respawn_t = 0.f;
  p.respawn_sec = 0;
  push_item(p, id, n > 0 ? n : 1);
  return &p;
}

LootPile* LootSystem::spawn_quest_reward(Vec2 pos, ItemId id, int n) {
  LootPile* p = spawn_item(pos, id, n);
  if (p) p->quest_reward_case = true;
  return p;
}

LootPile* LootSystem::spawn_item_iv6(Vec2 pos, ItemId id, int count, float iv6) {
  LootPile* p = spawn_item(pos, id, 1);
  if (p && p->item_count > 0) {
    p->counts[p->item_count - 1] = count;
    p->iv6[p->item_count - 1] = c2_item_has_iv6(id) ? iv6 : -1.f;
  }
  return p;
}

LootPile* LootSystem::spawn_drop_from_player(Vec2 player_pos, ItemId id, int count) {
  if (id <= 0 || count <= 0) return 0;
  return spawn_item(Vec2(player_pos.x, player_pos.y + 15.f), id, count);
}

void LootSystem::schedule_respawn(LootPile& p) {
  p.active = false;
  p.item_count = 0;
  p.respawn_sec = next_loot_stagger_sec(p.respawn_sec);
  p.respawn_t = static_cast<float>(p.respawn_sec);
}

void LootSystem::tick_respawn(float dt, Rng& rng) {
  for (int i = 0; i < count_; ++i) {
    LootPile& p = piles_[i];
    if (p.active || p.respawn_t <= 0.f) continue;
    p.respawn_t -= dt;
    if (p.respawn_t > 0.f) continue;
    const Vec2 pos = p.pos;
    const LootPileKind kind = p.kind;
    const bool air = p.is_airdrop;
    const unsigned char stagger = p.respawn_sec;
    LootPile* fresh = 0;
    switch (kind) {
    case LootKind_Military: fresh = spawn_military(pos, rng); break;
    case LootKind_Hospital: fresh = spawn_hospital(pos, rng); break;
    case LootKind_Gas: fresh = spawn_gas_station(pos, rng); break;
    case LootKind_Helicrash: fresh = spawn_helicrash(pos, rng); break;
    case LootKind_Police: fresh = spawn_police(pos, rng); break;
    case LootKind_Fire: fresh = spawn_firestation(pos, rng); break;
    case LootKind_Church: fresh = spawn_church(pos, rng); break;
    case LootKind_Secret: fresh = spawn_secret(pos, rng); break;
    case LootKind_Airdrop: fresh = spawn_airdrop(pos, rng); break;
    case LootKind_Civilian: fresh = spawn_civilian(pos, rng); break;
    default: fresh = spawn_random(pos, rng); break;
    }
    if (!fresh) {
      p.respawn_t = kLootRespawnStaggerSec[0];
      continue;
    }
    if (fresh != &p) {
      p = *fresh;
      p.pos = pos;
      p.kind = kind;
      p.is_airdrop = air;
      p.respawn_sec = stagger;
      fresh->active = false;
      fresh->item_count = 0;
      fresh->respawn_t = 0.f;
    } else {
      p.respawn_t = 0.f;
    }
  }
}

LootPile* LootSystem::spawn_airdrop(Vec2 pos, Rng& rng, const WeaponUnlockState* unlocks) {
  // Military / airdrop-ish IDs from catalog + extract hints
  static const ItemId kAir[] = {
    11, 12, 13, 14, 15, 18, 19, 28, 85, 151, 156, 157, 158, 159, 160, 168,
    169, 204, 251, 265, 400, 404, 451, 22, 45, 36
  };
  LootPile* p = spawn_table(pos, rng, kAir,
                            static_cast<int>(sizeof(kAir) / sizeof(kAir[0])), 4, 6);
  if (p) {
    p->kind = LootKind_Airdrop;
    p->is_airdrop = true;
    p->quest_reward_case = false;
    // Re-roll table picks through unlock filter (spawn_table has no unlock param).
    if (unlocks) {
      for (int i = 0; i < p->item_count; ++i) {
        if (!loot_unlock_allows(unlocks, p->items[i])) {
          p->items[i] = p->items[p->item_count - 1];
          p->counts[i] = p->counts[p->item_count - 1];
          p->iv6[i] = p->iv6[p->item_count - 1];
          --p->item_count;
          --i;
        }
      }
    }
    push_item_unlock(*p, 18, rng.next_int(1, 3), unlocks);
    if (rng.next_float() < 0.55f) push_item_unlock(*p, 19, 1, unlocks);
    if (rng.next_float() < 0.4f) {
      static const ItemId kArmor[] = { 251, 263, 400, 404, 405 };
      push_item_unlock(*p, kArmor[rng.next_int(0, 4)], 1, unlocks);
    }
    if (rng.next_float() < 0.25f) {
      static const ItemId kGun[] = { 158, 159, 168, 156 };
      push_item_unlock(*p, kGun[rng.next_int(0, 3)], 1, unlocks);
    }
    if (rng.next_float() < 0.35f) {
      static const ItemId kAtt[] = { 88, 89, 90, 76, 80 };
      push_item_unlock(*p, kAtt[rng.next_int(0, 4)], 1, unlocks);
    }
    if (rng.next_float() < 0.4f) push_item_unlock(*p, 34, 1, unlocks);
  }
  return p;
}

LootPile* LootSystem::spawn_hospital(Vec2 pos, Rng& rng) {
  static const ItemId kMed[] = {
    18, 18, 19, 28, 28, 29, 45, 46, 46, 18, 19, 28, 34, 265, 250, 35, 359, 305
  };
  LootPile* p = spawn_table(pos, rng, kMed,
                            static_cast<int>(sizeof(kMed) / sizeof(kMed[0])), 3, 5);
  if (p) {
    p->kind = LootKind_Hospital;
    push_item(*p, 18, rng.next_int(1, 2)); // guaranteed bandage
    if (rng.next_float() < 0.55f) push_item(*p, 46, 1); // tetracycline common in hospitals
    if (rng.next_float() < 0.25f) push_item(*p, 359, 1); // paramedic jacket
    if (rng.next_float() < 0.15f) {
      push_item(*p, (rng.next_float() < 0.5f) ? 251 : 400, 1); // rare helmet/vest
    }
  }
  return p;
}

LootPile* LootSystem::spawn_gas_station(Vec2 pos, Rng& rng) {
  static const ItemId kGas[] = {
    53, 53, 53, 59, 59, 24, 50, 8, 30, 1, 200, 205, 353, 39, 22, 54, 55
  };
  LootPile* p = spawn_table(pos, rng, kGas,
                            static_cast<int>(sizeof(kGas) / sizeof(kGas[0])), 2, 4);
  if (p) p->kind = LootKind_Gas;
  // Guaranteed gasoline can at stations
  if (p && rng.next_float() < 0.7f) push_item(*p, 53, 1);
  if (p && rng.next_float() < 0.35f) push_item(*p, 59, 1); // toolbox bias
  return p;
}

LootPile* LootSystem::spawn_civilian(Vec2 pos, Rng& rng) {
  static const ItemId kCiv[] = {
    1, 2, 5, 6, 7, 8, 9, 10, 18, 25, 26, 30, 31, 33, 35, 36, 42, 54, 55, 201, 203, 210,
    300, 301, 350, 351, 24, 50, 353, 354, 356, 365, 40,
    47, 48, 68, 82 // shovel / hacksaw + seed packs for farmer/cook loop
  };
  LootPile* p = spawn_table(pos, rng, kCiv,
                            static_cast<int>(sizeof(kCiv) / sizeof(kCiv[0])), 1, 3);
  if (p) p->kind = LootKind_Civilian;
  if (p && rng.next_float() < 0.25f) {
    static const ItemId kSeed[] = { 47, 48, 68 };
    push_item(*p, kSeed[rng.next_int(0, 2)], 1);
  }
  return p;
}

LootPile* LootSystem::spawn_military(Vec2 pos, Rng& rng, const WeaponUnlockState* unlocks) {
  static const ItemId kMil[] = {
    11, 14, 15, 18, 19, 43, 56, 69, 70, 76, 77, 78, 79, 80, 81, 88, 89, 90, 98, 99, 151, 158, 159, 160, 169, 177, 179,
    251, 263, 303, 355, 400, 402, 404, 405, 22, 265, 57
  };
  LootPile* p = spawn_table(pos, rng, kMil,
                            static_cast<int>(sizeof(kMil) / sizeof(kMil[0])), 2, 4);
  if (p) {
    p->kind = LootKind_Military;
    if (unlocks) {
      for (int i = 0; i < p->item_count; ++i) {
        if (!loot_unlock_allows(unlocks, p->items[i])) {
          p->items[i] = p->items[p->item_count - 1];
          p->counts[i] = p->counts[p->item_count - 1];
          p->iv6[i] = p->iv6[p->item_count - 1];
          --p->item_count;
          --i;
        }
      }
    }
  }
  if (p && rng.next_float() < 0.35f) {
    static const ItemId kArmor[] = { 251, 263, 400, 404, 405 };
    push_item_unlock(*p, kArmor[rng.next_int(0, 4)], 1, unlocks);
  }
  if (p && rng.next_float() < 0.2f) push_item_unlock(*p, 355, 1, unlocks);
  return p;
}

LootPile* LootSystem::spawn_bandit(Vec2 pos, Rng& rng) {
  static const ItemId kBot[] = {
    12, 69, 18, 1, 8, 152, 170, 42, 53, 26, 45, 151, 251
  };
  LootPile* p = spawn_table(pos, rng, kBot,
                            static_cast<int>(sizeof(kBot) / sizeof(kBot[0])), 1, 3);
  if (p) p->kind = LootKind_Generic;
  return p;
}

LootPile* LootSystem::spawn_police(Vec2 pos, Rng& rng) {
  // Station lockers: handguns, ammo, batons, hats, meds
  static const ItemId kPol[] = {
    12, 69, 18, 26, 42, 151, 152, 170, 179, 180, 203, 251, 264, 36, 45, 22, 39
  };
  LootPile* p = spawn_table(pos, rng, kPol,
                            static_cast<int>(sizeof(kPol) / sizeof(kPol[0])), 2, 4);
  if (p) {
    p->kind = LootKind_Police;
    if (rng.next_float() < 0.5f) push_item(*p, 69, rng.next_int(8, 16)); // 9mm
    if (rng.next_float() < 0.35f) push_item(*p, 151, 1); // FNX bias
    if (rng.next_float() < 0.4f) push_item(*p, 264, 1); // police hat
  }
  return p;
}

LootPile* LootSystem::spawn_helicrash(Vec2 pos, Rng& rng) {
  // Crash sites: mix of military gear + meds + scrap
  static const ItemId kHeli[] = {
    11, 14, 15, 18, 19, 22, 28, 45, 53, 59, 69, 76, 98, 85, 151, 156, 158, 159,
    168, 204, 251, 265, 400, 404, 36, 39
  };
  LootPile* p = spawn_table(pos, rng, kHeli,
                            static_cast<int>(sizeof(kHeli) / sizeof(kHeli[0])), 3, 5);
  if (p) {
    p->kind = LootKind_Helicrash;
    push_item(*p, 18, 1);
    if (rng.next_float() < 0.45f) push_item(*p, 53, 1); // spilled fuel
    if (rng.next_float() < 0.3f) {
      static const ItemId kGun[] = { 156, 158, 168, 159 };
      push_item(*p, kGun[rng.next_int(0, 3)], 1);
    }
  }
  return p;
}

LootPile* LootSystem::spawn_secret(Vec2 pos, Rng& rng) {
  // Rare cache: NVG, SVD, armor, medical, ammo
  static const ItemId kSec[] = {
    265, 168, 156, 19, 28, 45, 13, 11, 400, 405, 85, 22, 456
  };
  LootPile* p = spawn_table(pos, rng, kSec,
                            static_cast<int>(sizeof(kSec) / sizeof(kSec[0])), 3, 5);
  if (p) p->kind = LootKind_Secret;
  if (p && rng.next_float() < 0.5f) push_item(*p, 168, 1); // SVD bias
  return p;
}

LootPile* LootSystem::spawn_firestation(Vec2 pos, Rng& rng) {
  static const ItemId kFire[] = {
    24, 50, 53, 59, 200, 201, 204, 205, 18, 26, 39, 82, 22, 353
  };
  LootPile* p = spawn_table(pos, rng, kFire,
                            static_cast<int>(sizeof(kFire) / sizeof(kFire[0])), 2, 4);
  if (p) {
    p->kind = LootKind_Fire;
    if (rng.next_float() < 0.6f) push_item(*p, 204, 1); // fire axe
    if (rng.next_float() < 0.35f) push_item(*p, 205, 1); // crowbar
  }
  return p;
}

LootPile* LootSystem::spawn_church(Vec2 pos, Rng& rng) {
  static const ItemId kCh[] = {
    1, 6, 18, 25, 26, 29, 33, 49, 54, 210, 300, 350, 358, 362, 45
  };
  LootPile* p = spawn_table(pos, rng, kCh,
                            static_cast<int>(sizeof(kCh) / sizeof(kCh[0])), 2, 3);
  if (p) {
    p->kind = LootKind_Church;
    if (rng.next_float() < 0.5f) push_item(*p, 18, 1);
    if (rng.next_float() < 0.4f) push_item(*p, 25, 1);
    if (rng.next_float() < 0.45f) push_item(*p, 49, 1);
    if (rng.next_float() < 0.3f) push_item(*p, 358, 1); // cloak
  }
  return p;
}

LootPile* LootSystem::spawn_bunker(Vec2 pos, Rng& rng) {
  // Underground dens: mil ammo/guns + meds + tools (not full airdrop tier)
  static const ItemId kBun[] = {
    11, 14, 15, 18, 19, 22, 28, 45, 46, 53, 69, 85, 151, 158, 159, 181, 265,
    400, 404, 36, 39, 59, 100, 101
  };
  LootPile* p = spawn_table(pos, rng, kBun,
                            static_cast<int>(sizeof(kBun) / sizeof(kBun[0])), 3, 5);
  if (p) {
    p->kind = LootKind_Military;
    push_item(*p, 18, rng.next_int(1, 2));
    if (rng.next_float() < 0.4f) push_item(*p, 19, 1);
    if (rng.next_float() < 0.3f) push_item(*p, 265, 1); // NVG cache
  }
  return p;
}

} // namespace mdaz
