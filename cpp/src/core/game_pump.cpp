// C2 waterpump (t435) and the avalible_interaction priority (Merged pick and interact /
// loot_checker2, data.js sid1430881814617229). See KNOWN_ISSUES "Колонка".
#include "mdaz/game.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/ui_strings.hpp"

#include <cmath>

namespace mdaz {

namespace {
float d2v(float ax, float ay, Vec2 b) { const float x = ax - b.x, y = ay - b.y; return x * x + y * y; }
// l_eng_log.xml client_log lines used by the pump events (verbatim).
const char* kLog12 = "There's no water in this pump.";
const char* kLog13 = "I don't want to drink.";
const char* kLog77 = "I've drunk from the water pump.";
// player_base LOS behaviours interactor (Solid) and doorchecker (custom): range 70, cone 360
// (Map instance props). HasLOSToObject fails beyond the range before any distance compare.
const float kLosRange = 70.f;
} // namespace

// create_map_element cat 3 sub 2 (sid8279941300293499): Spawn waterpump; map_element IV3 == 0 ->
// IV0 = choose(100, 200) (sid3903561981802149). Tutorial layout instance: IV0 = 250.
void Game::init_pump_water(Rng& rng, bool tutorial) {
  for (int i = 0; i < 6000; ++i) pump_iv0_[i] = 0;
  for (int i = 0; i < c2_props_.count() && i < 6000; ++i) {
    if (c2_props_.at(i).def != kPumpDef) continue;
    pump_iv0_[i] = tutorial ? 250 : (rng.next_int(0, 1) ? 200 : 100);
  }
  avail_interaction_ = 0; avail_t_ = 0.f; avail_pump_ = -1; pump_walk_ = -1;
  pump_lock_t_ = 0.f; pump_add_water_ = 0.f;
  // Map layout only: OnLayoutStart -> Wait 0.1 -> Global_respawn (layoutname == "Map").
  pump_respawn_start_t_ = tutorial ? -1.f : 0.1f;
}

void Game::tick_pump_respawn_start(float dt) {
  if (pump_respawn_start_t_ < 0.f) return;
  pump_respawn_start_t_ -= dt;
  if (pump_respawn_start_t_ <= 0.f) { pump_respawn_start_t_ = -1.f; global_respawn_pumps(); }
}

// Global_respawn (sid388188595587361, layoutname == "Map"), sub-event sid703496838047803:
// pick waterpumps with distance(pump, player_collision_base) >= 500, ForEach:
// IV0 = choose(0, 25, 0, 25, 50, 75, 100, 150, 200) (choose = uniform over the 9 arguments).
void Game::global_respawn_pumps() {
  static const short kChoose[9] = {0, 25, 0, 25, 50, 75, 100, 150, 200};
  for (int i = 0; i < c2_props_.count() && i < 6000; ++i) {
    const MapProp& mp = c2_props_.at(i);
    if (mp.def != kPumpDef) continue;
    const float dx = mp.x - player_.pos.x, dy = mp.y - player_.pos.y;
    if (dx * dx + dy * dy < 500.f * 500.f) continue;
    pump_iv0_[i] = kChoose[rng_.next_int(0, 8)];
  }
}

int Game::pump_count_for_test() const {
  int n = 0;
  for (int i = 0; i < c2_props_.count(); ++i) if (c2_props_.at(i).def == kPumpDef) ++n;
  return n;
}
int Game::pump_index_for_test(int n) const {
  for (int i = 0; i < c2_props_.count(); ++i) if (c2_props_.at(i).def == kPumpDef && n-- == 0) return i;
  return -1;
}

int Game::nearest_pump(Vec2 p, float& d2) const {   // PickDistance nearest
  int best = -1; d2 = 0.f;
  for (int i = 0; i < c2_props_.count(); ++i) {
    const MapProp& mp = c2_props_.at(i);
    if (mp.def != kPumpDef) continue;
    const float d = d2v(mp.x, mp.y, p);
    if (best < 0 || d < d2) { best = i; d2 = d; }
  }
  return best;
}

int Game::first_pump_for_test() const {
  for (int i = 0; i < c2_props_.count(); ++i) if (c2_props_.at(i).def == kPumpDef) return i;
  return -1;
}

// waterpump frame 20x28, hotspot (0.35, 0.75); player_collision_base is a small box at the player.
bool Game::pump_overlaps_player(int k) const {
  if (k < 0 || k >= c2_props_.count()) return false;
  const MapProp& mp = c2_props_.at(k);
  const float x0 = mp.x - 0.35f * 20.f, y0 = mp.y - 0.75f * 28.f;
  return player_.pos.x >= x0 && player_.pos.x < x0 + 20.f &&
         player_.pos.y >= y0 && player_.pos.y < y0 + 28.f;
}

// sid2626558775459672 (pump IV0 > 0) / sid6215585229440874 (IV0 <= 0). IV14 = water.
void Game::pump_drink(int k) {
  if (k < 0 || k >= 6000) return;
  short& w = pump_iv0_[k];
  if (w <= 0) { show_toast(kLog12); return; }
  const float iv14 = player_.survival.water;
  if (iv14 >= 100.f) { show_toast(kLog13); return; }   // sid7751218635932882
  if (w > 25) {                                         // sid401776724287473
    const float need = 100.f - iv14;
    w = static_cast<short>(w - static_cast<int>(std::floor((need > 25.f ? 25.f : need) + 0.5f)));
    pump_add_water_ = 25.f;
  } else {                                              // sid5981791044814398
    pump_add_water_ = static_cast<float>(w);
    w = 0;
  }
  pump_lock_t_ = 2.f;   // IV21 = 1, Wait 2, IV21 = 0, AddInstanceVar IV14, Update_Stats, log 77
  use_anim_t_ = 2.f;
}

void Game::tick_pump_for_test(float dt) { update_avalible_interaction(dt); }

void Game::update_avalible_interaction(float dt) {
  if (pump_lock_t_ > 0.f) {
    pump_lock_t_ -= dt;
    if (pump_lock_t_ <= 0.f) {
      pump_lock_t_ = 0.f;
      player_.survival.water += pump_add_water_;   // Update_Stats clamps on its tick
      pump_add_water_ = 0.f;
      show_toast(kLog77);
    }
  }
  avail_t_ += dt;
  if (avail_t_ < 0.5f) return;   // System Every 0.5 s
  avail_t_ = 0.f;
  int ai = 0;
  avail_pump_ = -1;
  if (in_bunker_ || vehicles_.player_driving()) { avail_interaction_ = 0; return; }
  // 5: nearest item on the ground (t912) <= 100 px, interactor LOS or overlapping.
  {
    int best = -1; float bd = 0.f;
    for (int i = 0; i < loot_.count(); ++i) {
      const LootPile* p = loot_.at(i);
      if (!p->active || p->item_count <= 0) continue;
      const float d = d2v(p->pos.x, p->pos.y, player_.pos);
      if (best < 0 || d < bd) { best = i; bd = d; }
    }
    if (best >= 0 && bd <= 100.f * 100.f &&
        ((bd <= kLosRange * kLosRange && !los_blocked_world(player_.pos, loot_.at(best)->pos)) || bd < 15.f * 15.f)) ai = 5;
  }
  // 6: t909 (port has no separate t909 object; KNOWN_ISSUES).
  if (ai != 5 && ai != 6) {
    // 1: plants t940 (IV1 ready), interactor LOS, <= 100.
    {
      int best = -1; float bd = 0.f;
      for (int i = 0; i < crop_count_; ++i) {
        if (!crops_[i].active) continue;
        const float d = d2v(crops_[i].pos.x, crops_[i].pos.y, player_.pos);
        if (best < 0 || d < bd) { best = i; bd = d; }
      }
      if (best >= 0 && crops_[best].ready && bd <= kLosRange * kLosRange &&
          !los_blocked_world(player_.pos, crops_[best].pos)) ai = 1;
    }
    // 2: berry bush (nearest), interactor LOS, <= 100, frame 1.
    {
      int best = -1; float bd = 0.f;
      for (int i = 0; i < c2_props_.count(); ++i) {
        const MapProp& mp = c2_props_.at(i);
        if (MapProps::berry_item(mp.def) == 0) continue;
        const float d = d2v(mp.x, mp.y, player_.pos);
        if (best < 0 || d < bd) { best = i; bd = d; }
      }
      if (best >= 0) {
        const MapProp& mp = c2_props_.at(best);
        if (bd <= kLosRange * kLosRange && mp.frame == 1 && !los_blocked_world(player_.pos, Vec2(mp.x, mp.y))) ai = 2;
      }
    }
    // 3: car_loot_point < 50 with doorchecker LOS (trunk_avail_, computed every tick).
    if (trunk_avail_ >= 0) ai = 3;
    // 4: waterpump (nearest) doorchecker LOS, <= 100.
    {
      float pd = 0.f;
      const int pk = nearest_pump(player_.pos, pd);
      if (pk >= 0 && pd <= kLosRange * kLosRange) {   // LOS range 70 < the 100 px compare
        const MapProp& mp = c2_props_.at(pk);
        if (!doorchecker_blocked(player_.pos, Vec2(mp.x, mp.y))) { ai = 4; avail_pump_ = pk; }
      }
    }
    // 8: panel_button - no port object (KNOWN_ISSUES).
    // 7 (sid5546315147952857): player IV3 == 0, IV4 == 1 or 5 (equipped 202 / 204); doorchecker
    // HasLOSToObject obst_base (range 70: picks the visible ones), frame 10, distance <= 100.
    {
      const ItemId eq = inventory_.slot(equipped_slot_).id;
      if (eq == 202 || eq == 204) {
        for (int ti = 0; ti < tree_count_; ++ti) {
          if (tree_sub_[ti] < 1 || tree_sub_[ti] > 6 || tree_frame_[ti] != 0) continue;
          const float d = d2v(trees_[ti].x, trees_[ti].y, player_.pos);
          if (d > kLosRange * kLosRange) continue;
          if (doorchecker_blocked(player_.pos, trees_[ti])) continue;
          ai = 7;
          break;
        }
      }
    }
  }
  avail_interaction_ = ai;
}

} // namespace mdaz

namespace mdaz {
bool Game::harness_place_at_pump() {
  float d2 = 0.f;
  const int k = nearest_pump(player_.pos, d2);
  if (k < 0) return false;
  const MapProp& mp = c2_props_.at(k);
  player_.pos = Vec2(mp.x + 60.f, mp.y);
  player_.survival.water = 40.f;
  pump_iv0_[k] = 150;   // test-only: a non-empty pump for the screenshot
  return true;
}
} // namespace mdaz
