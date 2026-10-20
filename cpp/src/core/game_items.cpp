// C2 item state kept in the array count (IV1): bottle 54 / canteen 55 water level 0..100.
// item_activate (sub_menu "Use", ui 319 "Drink") and item_activate2 ("Use2", ui 322
// "Fill with water") verbatim from data.js; caller init_item_activation(2): return > 0 ->
// count = return, <= 0 -> id/count = 0, "zero" -> count = 0 (item stays), "lol" -> unchanged.
#include "mdaz/game.hpp"

#include "mdaz/c2_sfx.hpp"

#include <cstdio>

namespace mdaz {

namespace {
// l_eng_log.xml (verbatim)
const char* kLog12 = "There's no water in this pump.";
const char* kLog15 = "I need a water pump for that.";
const char* kLog16 = "I've filled it with a waterpump.";
const char* kLog49 = "Bottle empty.";
const char* kLog50 = "I've drunk from the water bottle.";
const char* kLog51 = "Canteen empty.";
const char* kLog52 = "I've drunk from the canteen.";
const char* kLog76 = "It's already full.";
const char* kLog90 = "I can't do it from inside of the car.";
const char* kLog10 = "Mag empty.";
const char* kLog11 = "Weapon ruined!";
const char* kLog44 = "Weapon ruined.";
const char* kLog45 = "Weapon cleaned.";

// Player IV1 (rifle slot IV2) / IV43 (pistol slot IV42) weapon classes, ids via t909 IV15.
bool c2_rifle_class(ItemId id) {
  switch (id) {
  case 154: case 155: case 156: case 157: case 158: case 159: case 160: case 161: case 162:
  case 163: case 164: case 166: case 167: case 168: case 169: case 173: case 174: case 175:
  case 177: case 178: case 183: case 184: case 185: case 186: case 187:
    return true;
  default: return false;
  }
}
bool c2_pistol_class(ItemId id) {
  switch (id) {
  case 151: case 152: case 153: case 165: case 170: case 171: case 172: case 176: case 179:
  case 180: case 181: case 182:
    return true;
  default: return false;
  }
}
} // namespace

const char* c2_log_weapon_ruined() { return kLog11; }
const char* c2_log_mag_empty() { return kLog10; }

// Group "Firearm shooting": t909 SubInstanceVar IV6 in each shot event, keyed by player IV1
// (rifles) / IV43 (pistols); class -> id through the t909 IV15 layout defaults.
float c2_fire_wear(ItemId id) {
  switch (id) {
  case 156: return 0.8f;  case 158: return 0.4f;  case 154: return 1.f;   case 160: return 0.3f;
  case 159: return 0.3f;  case 161: return 0.3f;  case 157: return 0.5f;  case 162: return 0.4f;
  case 155: return 0.5f;  case 163: return 1.f;   case 164: return 0.8f;  case 166: return 0.4f;
  case 167: return 0.4f;  case 168: return 0.8f;  case 169: return 0.3f;  case 173: return 0.3f;
  case 174: return 0.3f;  case 175: return 0.3f;  case 178: return 0.5f;  case 177: return 0.4f;
  case 183: return 0.3f;  case 184: return 0.4f;  case 185: return 0.4f;  case 186: return 0.6f;
  case 187: return 0.5f;
  case 151: case 152: case 153: case 170: case 165: case 171: case 172: case 176: case 180:
  case 182: return 1.f;
  case 179: return 0.4f;  case 181: return 0.5f;
  default: return 0.f;
  }
}

// t910 OnCollision zed 941 / tank 847 / wolf 469 / puncher 540 (IV5 melee): t909 IV15
// 0 (knife_any, flare) / 203 -> 1; 200, 204, 205, 210 -> 0.2; 201, 202, 206..209 -> 0.5.
float c2_melee_npc_wear(ItemId id) {
  switch (id) {
  case 42: case 43: case 44: case 203: return 1.f;
  case 200: case 204: case 205: case 210: return 0.2f;
  case 201: case 202: case 206: case 207: case 208: case 209: return 0.5f;
  default: return 0.f;
  }
}

float Game::held_condition(const InvSlot& s) const {
  if (s.id == 42 || s.id == 43 || s.id == 44) return static_cast<float>(s.count);
  if (c2_item_has_iv6(s.id)) return s.iv6;
  return 100.f;
}

void Game::wear_held_melee(ItemId weapon, float amount) {
  if (amount <= 0.f || weapon == 0) return;
  InvSlot& s = inventory_.slot(equipped_slot_);
  if (s.id != weapon) return;
  if (s.id == 42 || s.id == 43 || s.id == 44) {
    // knife_any IV6 = the array Condition (count); wear is whole units (IV15 = 0 -> 1).
    s.count -= static_cast<int>(amount + 0.5f);
    if (s.count < 0) s.count = 0;
    return;
  }
  if (!c2_item_has_iv6(s.id)) return;
  s.iv6 -= amount;
  if (s.iv6 < 0.f) s.iv6 = 0.f;
}

// item_activate CompareParam 36: two events, IV2 then IV42 (last SetReturnValue wins; no t909
// picked -> Function default return 0). Port: the first bag cell of each class stands in for
// the IV2 / IV42 weapon slot (KNOWN_ISSUES: weapons live in bag cells).
int Game::c2_cleaning_kit() {
  int ret = 0;
  bool cleaned = false;
  for (int pass = 0; pass < 2; ++pass) {
    int k = -1;
    for (int i = 0; i < inventory_.capacity() && k < 0; ++i) {
      const ItemId id = inventory_.slot(i).id;
      if (pass == 0 ? c2_rifle_class(id) : c2_pistol_class(id)) k = i;
    }
    if (k < 0) continue;
    InvSlot& w = inventory_.slot(k);
    if (w.iv6 <= 0.f) { show_toast(kLog44); ret = -2; }          // "lol" (log 44)
    if (w.iv6 < 100.f && w.iv6 > 0.f) {
      w.iv6 += 25.f;
      ret = -1;
      cleaned = true;
    } else {
      ret = -2;                                                   // Else -> "lol"
    }
    if (w.iv6 > 100.f) w.iv6 = 100.f;
  }
  if (cleaned) {
    use_anim_t_ = 5.f;                                            // IV21 = 1, Wait 5
    drink_t_ = 5.f;
    drink_add_ = 0.f;
    drink_log_ = kLog45;                                          // after the Wait
  }
  return ret;   // -2 = "lol" (unchanged), <= 0 otherwise -> slot cleared
}

// Items_update (sid7848660748444233 ...): worn backpack / outerwear / pants / vest with
// IV6 <= 0 (TriggerOnce) -> IV6 = 0, Drop_slot 1..N of that array, Cut_arrays (array size 0,
// tear_fabric if it was not empty). Port "once": the region still has cells.
void Game::tick_ruined_containers() {
  static const ClothingRegion kRegs[] = { ClothingRegion_Backpack, ClothingRegion_Outerwear,
                                          ClothingRegion_Pants, ClothingRegion_Vest };
  for (int w = 0; w < inventory_.worn_count(); ++w) {
    const ItemId id = inventory_.worn_at(w);
    if (inventory_.worn_iv6(w) > 0.f) continue;
    for (unsigned r = 0; r < sizeof(kRegs) / sizeof(kRegs[0]); ++r) {
      if (Inventory::chain_region_of(id) != kRegs[r]) continue;
      const int n = inventory_.clothing().region_size(kRegs[r]);
      if (n <= 0) continue;
      for (int c = 0; c < n; ++c) {
        const int fi = inventory_.flat_index(kRegs[r], c);
        if (fi >= 0 && !inventory_.slot(fi).empty()) try_drop_slot(fi);
      }
      sfx_at("tear_fabric", 0.f, player_.pos, "");   // PlayAtObject player_base, 0 dB
      inventory_.cut_region(kRegs[r]);
    }
  }
}


bool c2_item_has_use2_fill(ItemId id) { return id == 54 || id == 55; }

// item_activate CompareParam 54 / 55.
bool Game::c2_drink_container(int slot) {
  InvSlot& s = inventory_.slot(slot);
  if (s.id != 54 && s.id != 55) return false;
  const bool bottle = s.id == 54;
  const int c = s.count;
  if (c <= 0) {                                   // "zero" + log 49 / 51
    s.count = 0;
    show_toast(bottle ? kLog49 : kLog51);
    return true;
  }
  float add;
  if (c < 25) {                                   // "zero"; IV14 += c/2 (bottle) | c (canteen)
    s.count = 0;
    add = bottle ? c / 2.f : static_cast<float>(c);
  } else {                                        // (c-25 == 0 ? "zero" : c-25); +12.5 | +25
    s.count = c - 25;
    add = bottle ? 12.5f : 25.f;
  }
  sfx_play(c2_item_activate_sfx(s.id), player_.pos);   // "Whiskey" -10 dB at player
  use_anim_t_ = 2.f;                                    // IV21 = 1, Wait 2, IV21 = 0
  drink_t_ = 2.f;
  drink_add_ = add;
  drink_log_ = bottle ? kLog50 : kLog52;
  return true;
}

// item_activate2 CompareParam 54 / 55.
bool Game::c2_fill_container(int slot) {
  InvSlot& s = inventory_.slot(slot);
  if (s.id != 54 && s.id != 55) return false;
  const bool bottle = s.id == 54;
  const int c = s.count;
  if (vehicles_.player_driving()) { show_toast(kLog90); return true; }   // IV44 -> "lol"
  if (c == 100) { show_toast(kLog76); return true; }
  float d2 = 0.f;
  const int k = nearest_pump(player_.pos, d2);
  if (k < 0 || !pump_overlaps_player(k)) { show_toast(kLog15); return true; }
  short& w = pump_iv0_[k];
  if (w <= 0) { show_toast(kLog12); return true; }
  const int need = bottle ? (100 - c) / 2 : 100 - c;   // bottle: (100 - c) / 2
  sfx_play(c2_item_activate2_sfx(s.id), player_.pos);  // md_canteen_fill 0 dB
  if (w > need) {
    w = static_cast<short>(w - need);
    s.count = 100;
    use_anim_t_ = 2.f;                                  // IV21 = 1, Wait 2
    drink_t_ = 2.f;
    drink_add_ = 0.f;
    drink_log_ = kLog16;
  } else {
    const int loc = w;
    w = 0;
    s.count = bottle ? c + loc * 2 : c + loc;
    show_toast(kLog16);
  }
  return true;
}

void Game::tick_drink(float dt) {
  if (drink_t_ <= 0.f) return;
  drink_t_ -= dt;
  if (drink_t_ > 0.f) return;
  drink_t_ = 0.f;
  player_.survival.water += drink_add_;   // AddInstanceVar IV14; Update_Stats clamps
  drink_add_ = 0.f;
  if (drink_log_) show_toast(drink_log_);
  drink_log_ = 0;
}

// Harness (item 11b): bottle 54 at 30 in the first free cell; 0 = fill, 1 = drink, 2 = log.
void Game::harness_bottle(int step) {
  int k = -1;
  for (int i = 0; i < inventory_.capacity(); ++i) if (inventory_.slot(i).id == 54) { k = i; break; }
  if (step == 0 && k < 0) {
    for (int i = 0; i < inventory_.capacity(); ++i)
      if (inventory_.slot(i).empty()) { k = i; break; }
    if (k < 0) k = 0;
    inventory_.slot(k).id = 54;
    inventory_.slot(k).count = 30;
    player_.survival.water = 50.f;
  }
  if (k >= 0 && step == 0) c2_fill_container(k);
  if (k >= 0 && step == 1) c2_drink_container(k);
  char line[160];
  std::snprintf(line, sizeof(line), "harness_bottle step=%d slot=%d count=%d water=%.1f toast=%s", step, k,
                k >= 0 ? inventory_.slot(k).count : -1, player_.survival.water,
                toast_text() ? toast_text() : "-");
  std::printf("%s\n", line);
  if (platform_) platform_->log_line(line);
}

// Test-only (item 11c shots): rifle 156 at IV6 1.5 in hands, pants at IV6 37.5, then shots.
void Game::harness_durability(int step) {
  if (step == 0) {
    inventory_.slot(0) = InvSlot();
    inventory_.slot(0).id = 156; inventory_.slot(0).count = 1; inventory_.slot(0).iv6 = 1.5f;
    equipped_slot_ = 0;
    sync_mag_for_equipped();
    mag_rounds_ = 5;
    bool has_pants = false;
    for (int i = 0; i < inventory_.worn_count(); ++i)
      if (Inventory::chain_region_of(inventory_.worn_at(i)) == ClothingRegion_Pants) {
        inventory_.set_worn_iv6(i, 37.5f); has_pants = true;
      }
    if (!has_pants) {
      const ItemId ids[1] = {303};
      const float v[1] = {37.5f};
      inventory_.restore_worn_iv6(ids, v, 1);
    }
  } else {
    reload_cd_ = 0.f; attack_cd_ = 0.f;
    try_ranged_attack();
  }
  char line[160];
  std::snprintf(line, sizeof(line), "harness_durability step=%d id=%d iv6=%.2f mag=%d toast=%s", step,
                inventory_.slot(0).id, inventory_.slot(0).iv6, mag_rounds_, toast_text() ? toast_text() : "-");
  std::printf("%s\n", line);
  if (platform_) platform_->log_line(line);
}

} // namespace mdaz
