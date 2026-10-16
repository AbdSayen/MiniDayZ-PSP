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
  }
  return p;
}

LootPile* LootSystem::spawn_hospital(Vec2 pos, Rng& rng) {
  static const ItemId kMed[] = {
    18, 18, 19, 28, 28, 29, 45, 46, 46, 18, 19, 28, 34, 265, 250, 35
  };
  LootPile* p = spawn_table(pos, rng, kMed,
                            static_cast<int>(sizeof(kMed) / sizeof(kMed[0])), 3, 5);
  if (p) {
    push_item(*p, 18, rng.next_int(1, 2)); // guaranteed bandage
    if (rng.next_float() < 0.55f) push_item(*p, 46, 1); // tetracycline common in hospitals
    if (rng.next_float() < 0.15f) {
      push_item(*p, (rng.next_float() < 0.5f) ? 251 : 400, 1); // rare helmet/vest
    }
  }
  return p;
}

LootPile* LootSystem::spawn_gas_station(Vec2 pos, Rng& rng) {
  static const ItemId kGas[] = {
    53, 53, 53, 59, 59, 24, 50, 8, 30, 1, 200, 205, 353, 39, 22
  };
  LootPile* p = spawn_table(pos, rng, kGas,
                            static_cast<int>(sizeof(kGas) / sizeof(kGas[0])), 2, 4);
  // Guaranteed gasoline can at stations
  if (p && rng.next_float() < 0.7f) push_item(*p, 53, 1);
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
    11, 14, 15, 18, 19, 43, 56, 69, 70, 76, 77, 78, 79, 80, 81, 98, 99, 151, 158, 159, 160, 169, 177, 179,
    251, 263, 400, 402, 404, 405, 22, 265, 57
  };
  LootPile* p = spawn_table(pos, rng, kMil,
                            static_cast<int>(sizeof(kMil) / sizeof(kMil[0])), 2, 4);
  if (p && rng.next_float() < 0.35f) {
    static const ItemId kArmor[] = { 251, 263, 400, 404, 405 };
    push_item(*p, kArmor[rng.next_int(0, 4)], 1);
  }
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

} // namespace mdaz
