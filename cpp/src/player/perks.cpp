#include "mdaz/perks.hpp"

namespace mdaz {

const char* perk_c2_var(int f) {
  static const char* const k[Perk_Count] = {
    "perk_header", "perk_spotter", "perk_runner", "perk_camel", "perk_hamster",
    "perk_snowborn", "perk_farmer", "perk_regen", "perk_meleedef", "perk_meleeatk",
    "perk_trunkster", "perk_hunter", "perk_sweethome", "perk_metabolism", "perk_eye",
    "perk_agronom"};
  return (f >= 0 && f < Perk_Count) ? k[f] : "";
}

void PerkState::reset() {
  // Restart_game: expierence_points = 16, perk_* = 0, perks_learnt_count = 0, perk_cost = 200.
  xp_raw = 16;
  learnt = 0;
  cost = 200;
  for (int i = 0; i < Perk_Count; ++i) owned[i] = false;
  perk2_once = false;
}

bool PerkState::all_owned() const {
  for (int i = 0; i < Perk_Count; ++i) if (!owned[i]) return false;
  return true;
}

bool PerkState::buy(int f) {
  if (!can_buy(f)) return false;
  owned[f] = true;
  learnt += 1;
  // perk_cost = 216 * (perks_learnt_count + 1) + 100 * perks_learnt_count - 16 * perks_learnt_count
  cost = 216 * (learnt + 1) + 100 * learnt - 16 * learnt;
  return true;
}

int PerkState::pack_bits() const {
  int b = 0;
  for (int i = 0; i < Perk_Count; ++i) if (owned[i]) b |= 1 << i;
  return b;
}

void PerkState::unpack_bits(int b) {
  for (int i = 0; i < Perk_Count; ++i) owned[i] = (b & (1 << i)) != 0;
}

const PerkSlotPos kPerkMenuSlots[Perk_Count] = {
  // y = HotspotY - 95
  {13, -333, -95}, {1, -238, -95}, {2, -143, -95}, {0, -48, -95},
  {8, 47, -95}, {9, 142, -95}, {10, 237, -95}, {15, 332, -95},
  // y = HotspotY
  {12, -333, 0}, {5, -238, 0}, {4, -143, 0}, {7, -48, 0},
  {6, 47, 0}, {3, 142, 0}, {11, 237, 0}, {14, 332, 0},
};

void perk_icon_src(int f, int& sx, int& sy) {
  // t544 frames: 5 per row, 92 px pitch, origin (1, 1).
  if (f < 0) f = 0;
  sx = 1 + 92 * (f % 5);
  sy = 1 + 92 * (f / 5);
}

} // namespace mdaz
