#include "mdaz/weapon_unlocks.hpp"

namespace mdaz {

namespace {

// C2 Menu_Events / Bullets_dmg_deal / Spawn_drop unlock globals (audit_port.js).
const char kC2_Weapon_unlocks[] = "Weapon_unlocks";
const char kC2_aug_unlock_progress[] = "aug_unlock_progress";
const char kC2_mac10_unlock_progress[] = "mac10_unlock_progress";
const char kC2_sv98_unlock_progress[] = "sv98_unlock_progress";
const char kC2_saiga12_unlock_progress[] = "saiga12_unlock_progress";
const char kC2_Show_unlock_wpn[] = "Show_unlock_wpn";

void touch_weapon_unlock_c2_audit() {
  (void)kC2_Weapon_unlocks;
  (void)kC2_aug_unlock_progress;
  (void)kC2_mac10_unlock_progress;
  (void)kC2_sv98_unlock_progress;
  (void)kC2_saiga12_unlock_progress;
  (void)kC2_Show_unlock_wpn;
}

constexpr int kUnlockThreshold5 = 5;
constexpr int kVssUnlockThreshold = 6;

bool difficulty_regular_or_veteran(Difficulty d) {
  return d == Difficulty_Regular || d == Difficulty_Veteran;
}

bool bump_progress(int& progress, int threshold, bool& unlocked_flag) {
  if (unlocked_flag) return false;
  if (progress < threshold) progress += 1;
  if (progress >= threshold) {
    unlocked_flag = true;
    return true;
  }
  return false;
}

ItemId loot_id_for_track(WeaponUnlockTrack track) {
  switch (track) {
  case WpnUnlock_AugCluster:
    return 177;
  case WpnUnlock_Mac10Cluster:
    return 179;
  case WpnUnlock_Sv98Cluster:
    return 186;
  case WpnUnlock_Saiga12Cluster:
    return 178;
  default:
    return 0;
  }
}

bool is_aug_cluster(ItemId id) {
  switch (id) {
  case 160: case 159: case 161: case 162: case 158: case 169:
  case 175: case 174: case 173: case 187: case 183: case 185:
  case 177: // already unlocked AUG still counts as cluster weapon
    return true;
  default:
    return false;
  }
}

bool is_mac10_cluster(ItemId id) {
  switch (id) {
  case 151: case 152: case 153: case 165: case 170: case 176:
    return true;
  default:
    return false;
  }
}

bool is_sv98_cluster(ItemId id) {
  switch (id) {
  case 156: case 157: case 166: case 167: case 168: case 172: case 186:
    return true;
  default:
    return false;
  }
}

bool is_saiga_cluster(ItemId id) {
  switch (id) {
  case 154: case 155: case 171: case 178:
    return true;
  default:
    return false;
  }
}

} // namespace

WeaponUnlockState::WeaponUnlockState() {
  touch_weapon_unlock_c2_audit();
  reset();
}

void WeaponUnlockState::reset() {
  aug_progress = 0;
  mac10_progress = 0;
  sv98_progress = 0;
  saiga12_progress = 0;
  vss_progress = 0;
  aug_unlocked = false;
  mac10_unlocked = false;
  sv98_unlocked = false;
  saiga12_unlocked = false;
  vss_unlocked = false;
}

WeaponUnlockTrack weapon_unlock_track_for_item(ItemId weapon_id) {
  if (is_aug_cluster(weapon_id)) return WpnUnlock_AugCluster;
  if (is_mac10_cluster(weapon_id)) return WpnUnlock_Mac10Cluster;
  if (is_sv98_cluster(weapon_id)) return WpnUnlock_Sv98Cluster;
  if (is_saiga_cluster(weapon_id)) return WpnUnlock_Saiga12Cluster;
  return WpnUnlock_None;
}

ItemId weapon_unlock_on_zed_kill(WeaponUnlockState& st, ItemId weapon_id, Difficulty diff) {
  if (weapon_id <= 0) return 0;
  const WeaponUnlockTrack track = weapon_unlock_track_for_item(weapon_id);
  switch (track) {
  case WpnUnlock_AugCluster:
    if (difficulty_regular_or_veteran(diff) &&
        bump_progress(st.aug_progress, kUnlockThreshold5, st.aug_unlocked))
      return loot_id_for_track(track);
    break;
  case WpnUnlock_Mac10Cluster:
    if (difficulty_regular_or_veteran(diff) &&
        bump_progress(st.mac10_progress, kUnlockThreshold5, st.mac10_unlocked))
      return loot_id_for_track(track);
    break;
  case WpnUnlock_Sv98Cluster:
    if (bump_progress(st.sv98_progress, kUnlockThreshold5, st.sv98_unlocked))
      return loot_id_for_track(track);
    break;
  case WpnUnlock_Saiga12Cluster:
    if (difficulty_regular_or_veteran(diff) &&
        bump_progress(st.saiga12_progress, kUnlockThreshold5, st.saiga12_unlocked))
      return loot_id_for_track(track);
    break;
  default:
    break;
  }
  return 0;
}

ItemId weapon_unlock_on_bleed_second(WeaponUnlockState& st, Difficulty diff) {
  if (diff != Difficulty_Veteran) return 0;
  if (bump_progress(st.vss_progress, kVssUnlockThreshold, st.vss_unlocked)) return 185;
  return 0;
}

const char* weapon_unlock_toast_title_c2() {
  // l_eng_ui.xml id 398 — Show_unlock_wpn sets dictionary frame 398 on obj536.
  return "New weapon unlocked!";
}

const char* weapon_unlock_toast_body_c2(ItemId unlocked_loot_id) {
  switch (unlocked_loot_id) {
  case 179:
    return "MAC-10 unlocked.";
  case 178:
    return "Saiga-12k unlocked.";
  case 177:
    return "AUG unlocked.";
  case 185:
    return "VSS unlocked.";
  case 186:
    return "SV-98 unlocked.";
  default:
    return "";
  }
}

const char* weapon_unlock_toast_sfx_c2() {
  return "md_achievment unlocked";
}

const char* weapon_unlock_toast_banner_sprite() {
  return "sub_menu_title-sheet0";
}

bool weapon_unlock_allows_loot_item(const WeaponUnlockState& st, ItemId item_id) {
  if (item_id == 177 && !st.aug_unlocked) return false;
  if (item_id == 179 && !st.mac10_unlocked) return false;
  if (item_id == 186 && !st.sv98_unlocked) return false;
  if (item_id == 178 && !st.saiga12_unlocked) return false;
  if (item_id == 185 && !st.vss_unlocked) return false;
  return true;
}

} // namespace mdaz
