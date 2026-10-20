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
} // namespace

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

} // namespace mdaz
