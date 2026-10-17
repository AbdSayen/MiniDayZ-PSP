#include "mdaz/loot.hpp"

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

int ammo_qty(ItemId id, Rng& rng) {
  if (id >= 11 && id <= 17) return rng.next_int(4, 16);
  if (id == 69 || id == 12 || id == 58 || id == 100 || id == 101) return rng.next_int(4, 14);
  return 1;
}

} // namespace

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
    push_item(p, id, ammo_qty(id, rng));
  }
  return &p;
}

LootPile* LootSystem::spawn_random(Vec2 pos, Rng& rng) {
  if (count_ >= kMaxLootPiles) return 0;
  LootPile& p = piles_[count_++];
  p.pos = pos;
  p.item_count = 0;
  p.active = true;
  p.is_airdrop = false;
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
    11, 12, 13, 14, 15, 18, 19, 28, 85, 151, 156, 157, 158, 159, 160, 168,
    169, 204, 251, 265, 400, 404, 451, 22, 45, 36
  };
  LootPile* p = spawn_table(pos, rng, kAir,
                            static_cast<int>(sizeof(kAir) / sizeof(kAir[0])), 4, 6);
  if (p) {
    p->is_airdrop = true;
    // Guaranteed medical + chance of armor / high-tier gun
    push_item(*p, 18, rng.next_int(1, 3));
    if (rng.next_float() < 0.55f) push_item(*p, 19, 1);
    if (rng.next_float() < 0.4f) {
      static const ItemId kArmor[] = { 251, 263, 400, 404, 405 };
      push_item(*p, kArmor[rng.next_int(0, 4)], 1);
    }
    if (rng.next_float() < 0.25f) {
      static const ItemId kGun[] = { 158, 159, 168, 156 };
      push_item(*p, kGun[rng.next_int(0, 3)], 1);
    }
    // Attachment / case chance (Magpull / Choke / Protector Case)
    if (rng.next_float() < 0.35f) {
      static const ItemId kAtt[] = { 88, 89, 90, 76, 80 };
      push_item(*p, kAtt[rng.next_int(0, 4)], 1);
    }
    if (rng.next_float() < 0.4f) push_item(*p, 34, 1); // heatpack
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
  // Guaranteed gasoline can at stations
  if (p && rng.next_float() < 0.7f) push_item(*p, 53, 1);
  if (p && rng.next_float() < 0.35f) push_item(*p, 59, 1); // toolbox bias
  return p;
}

LootPile* LootSystem::spawn_civilian(Vec2 pos, Rng& rng) {
  static const ItemId kCiv[] = {
    1, 2, 5, 8, 10, 18, 26, 30, 42, 54, 203, 300, 350, 24, 50, 353, 354, 40
  };
  return spawn_table(pos, rng, kCiv,
                     static_cast<int>(sizeof(kCiv) / sizeof(kCiv[0])), 1, 3);
}

LootPile* LootSystem::spawn_military(Vec2 pos, Rng& rng) {
  static const ItemId kMil[] = {
    11, 14, 15, 18, 19, 43, 56, 69, 70, 76, 77, 78, 79, 80, 81, 88, 89, 90, 98, 99, 151, 158, 159, 160, 169, 177, 179,
    251, 263, 303, 355, 400, 402, 404, 405, 22, 265, 57
  };
  LootPile* p = spawn_table(pos, rng, kMil,
                            static_cast<int>(sizeof(kMil) / sizeof(kMil[0])), 2, 4);
  if (p && rng.next_float() < 0.35f) {
    static const ItemId kArmor[] = { 251, 263, 400, 404, 405 };
    push_item(*p, kArmor[rng.next_int(0, 4)], 1);
  }
  if (p && rng.next_float() < 0.2f) push_item(*p, 355, 1); // Gorka jacket bias
  return p;
}

LootPile* LootSystem::spawn_bandit(Vec2 pos, Rng& rng) {
  static const ItemId kBot[] = {
    12, 69, 18, 1, 8, 152, 170, 42, 53, 26, 45, 151, 251
  };
  return spawn_table(pos, rng, kBot,
                     static_cast<int>(sizeof(kBot) / sizeof(kBot[0])), 1, 3);
}

LootPile* LootSystem::spawn_police(Vec2 pos, Rng& rng) {
  // Station lockers: handguns, ammo, batons, hats, meds
  static const ItemId kPol[] = {
    12, 69, 18, 26, 42, 151, 152, 170, 179, 180, 203, 251, 264, 36, 45, 22, 39
  };
  LootPile* p = spawn_table(pos, rng, kPol,
                            static_cast<int>(sizeof(kPol) / sizeof(kPol[0])), 2, 4);
  if (p) {
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
    push_item(*p, 18, rng.next_int(1, 2));
    if (rng.next_float() < 0.4f) push_item(*p, 19, 1);
    if (rng.next_float() < 0.3f) push_item(*p, 265, 1); // NVG cache
  }
  return p;
}

} // namespace mdaz
