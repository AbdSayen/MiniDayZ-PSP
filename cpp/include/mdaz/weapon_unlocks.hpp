#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

// C2 globals / Bullets_dmg_deal + Bleed — kill-count weapon unlocks + Show_unlock_wpn toast.
struct WeaponUnlockState {
  int aug_progress;
  int mac10_progress;
  int sv98_progress;
  int saiga12_progress;
  int vss_progress;
  bool aug_unlocked;
  bool mac10_unlocked;
  bool sv98_unlocked;
  bool saiga12_unlocked;
  bool vss_unlocked;

  WeaponUnlockState();
  void reset();
};

enum WeaponUnlockTrack {
  WpnUnlock_None = 0,
  WpnUnlock_AugCluster,
  WpnUnlock_Mac10Cluster,
  WpnUnlock_Sv98Cluster,
  WpnUnlock_Saiga12Cluster,
};

WeaponUnlockTrack weapon_unlock_track_for_item(ItemId weapon_id);

// Bullets_dmg_deal zed kill with player weapon (SEED_RUN==0 assumed in desktop campaign).
// Returns gated loot item id when unlock just fired (177 AUG, 179 MAC-10, 186 SV-98, 178 Saiga, 185 VSS).
ItemId weapon_unlock_on_zed_kill(WeaponUnlockState& st, ItemId weapon_id, Difficulty diff);

// Bleed: Veteran difficulty adds vss_unlock_progress (+1/tick); unlock at 6 (States/Bleed decode).
ItemId weapon_unlock_on_bleed_second(WeaponUnlockState& st, Difficulty diff);

bool weapon_unlock_allows_loot_item(const WeaponUnlockState& st, ItemId item_id);

// Show_unlock_wpn — l_eng_ui.xml id 398 + unlocked weapon blurb ids 395–397, 417, 419.
const char* weapon_unlock_toast_title_c2();
const char* weapon_unlock_toast_body_c2(ItemId unlocked_loot_id);
const char* weapon_unlock_toast_sfx_c2();
const char* weapon_unlock_toast_banner_sprite();

} // namespace mdaz
