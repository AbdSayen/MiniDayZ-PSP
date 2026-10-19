// C2 layout "Tutorial" (Mini DayZ 1.4.1 data.js, event sheet Tutorial_events + the tutorial
// branches of Game_events / Menu_Events). Everything here follows the decoded events; port
// substitutions for touch-only GUI buttons are listed in KNOWN_ISSUES.md ("Tutorial").
#include "mdaz/game.hpp"
#include "mdaz/tutorial_layout.hpp"
#include "mdaz/building_solid.hpp"
#include "mdaz/psp_ui.hpp"
#include "mdaz/hud.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace mdaz {

using ui::text_wrap;
using ui::text_center;

namespace {
const float kTutCamSpeed = 600.f;   // camera Bullet speed 6000 x timescale 0.1
const float kTutScale = 0.55f;      // = world_to_screen_scale() in game.cpp
float d2(Vec2 a, Vec2 b) { const float x = a.x - b.x, y = a.y - b.y; return x * x + y * y; }
// GUI_show_tutorial_tip: text id per tip id (l_eng_ui.xml).
int tip_text_for(int id, bool inventory_open) {
  switch (id) {
  case 0: return 461;
  case 1: return inventory_open ? 454 : 453;
  case 2: return 454;
  case 3: return 455;
  case 6: return 458;
  case 7: return 462;
  case 8: return 463;
  case 9: return 464;
  case 10: return 459;
  case 11: return 460;
  case 12: return 467;
  case 13: return 465;
  case 14: return 466;
  default: return 0;
  }
}
// camera_roll: text 536[469 + id - 1] -> ids 1..7 = 469..475, 8 = 476 (no string), 9 = 451,
// 10 = 452, 11 = 468 (t536 string table indices as remapped by the l_eng_ui loader).
int roll_text_for(int id) {
  if (id >= 1 && id <= 7) return 468 + id;
  if (id == 9) return 451;
  if (id == 10) return 452;
  if (id == 11) return 468;
  return 476;
}
bool in_rect(Vec2 p, Vec2 c, Vec2 sz, float pad) {
  return p.x >= c.x - sz.x * 0.5f - pad && p.x <= c.x + sz.x * 0.5f + pad &&
         p.y >= c.y - sz.y * 0.5f - pad && p.y <= c.y + sz.y * 0.5f + pad;
}
const TutMisc* tut_find_misc(int type, int iv0 = -1) {
  for (int i = 0; i < tut_misc_count(); ++i) {
    const TutMisc& m = tut_misc(i);
    if (m.type == type && (iv0 < 0 || m.iv0 == iv0)) return &m;
  }
  return 0;
}
// b_exit "wooden" frames: 0 = 82x39 (hotspot .5/.513, broken), 1 = 69x27 (.507/.519, repaired).
// Collision poly (frame 0) spans x -0.3..0.493 and y -0.513..0.487 of the frame.
bool boat_overlap(Vec2 p, Vec2 b) {
  return p.x >= b.x - 0.3f * 82.f - 8.f && p.x <= b.x + 0.4933f * 82.f + 8.f &&
         p.y >= b.y - 0.5128f * 39.f - 8.f && p.y <= b.y + 0.4872f * 39.f + 8.f;
}
} // namespace

bool Game::tut_los(Vec2 a) const { return !los_blocked_world(player_.pos, a); }

Vec2 Game::render_cam() const {
  if (tut_.active && tut_.roll_phase > 0) return tut_.cam;
  return player_.pos;
}

Game::TutorialView Game::tutorial_view() const {
  TutorialView v;
  v.tip_visible = tut_.tip_visible; v.tip_two = tut_.tip_two; v.tip_text_id = tut_.tip_text;
  v.tip_id = tut_.tip_id; v.camera_paused = tut_.roll_phase == 2;
  v.camera_rolling = tut_.roll_phase > 0;
  v.ui_step = tut_.ui_step; v.inv_step = tut_.inv_step; v.perk_step = tut_.perk_step;
  v.bleed_step = tut_.bleed_step; v.final_step = tut_.final_step; v.menu_visible = tut_.menu_visible;
  return v;
}

void Game::start_tutorial() {
  // Base session reset (Game_events start of layout: stats, inventory, timers).
  start_new_game(menu_difficulty_);
  std::memset(tree_hp_, 2, sizeof(tree_hp_));
  TutorialRun& t = tut_;
  t.active = true; t.t = 0.f; t.start_phase = 0;
  t.moveblock = 0; t.blocked_gui = 0; t.onscreen = 1;   // Tutor_hint_onscreen = 1
  t.ui_step = 0; t.inv_step = -1; t.perk_step = 0; t.bleed_step = -1; t.death = 0;
  for (int i = 0; i < 12; ++i) t.found[i] = false;
  t.firearm_tip_done = false; t.encounter_on = false; t.encounter_t = 0.f; t.bleed_t = 0.f;
  t.experience_points = 0; t.die_count = 0;
  t.tip_visible = false; t.tip_two = false; t.tip_top = false; t.tip_text = 0; t.tip_id = -1;
  t.dark = 0.f; t.highlight = 0;
  t.roll_id = 0; t.roll_phase = 0; t.cam = Vec2(); t.roll_target = Vec2(); t.roll_kind = 0; t.roll_index = -1;
  t.timescale = 1.f; t.zeds_blind = false;
  for (int i = 0; i < 4; ++i) { t.zone_alive[i] = false; t.zone_pos[i] = Vec2(); t.zone_size[i] = Vec2(); }
  t.menu_visible = false; t.menu_kind = 0; t.boat_frame = 0;
  t.final_step = 0; t.final_t = 0.f; t.say_id = 0; t.say_t = 0.f; t.fade = 0.f; t.shots = 0;
  t.zed_hit_prev = false;
  for (int i = 0; i < 2; ++i) { t.bot[i] = Vec2(); t.bot_anim[i] = 0; t.bot_face[i] = 0.f; }

  // Tutorial start of layout: mapgen destroyed (Reload_locations does nothing).
  mapgen_.clear();
  zombies_.clear(); animals_.clear(); bandits_.clear(); vehicles_.clear(); loot_.clear();
  campfires_.clear(); bullets_.clear();
  cell_tiles_.clear();
  quest_.reset();
  for (int i = 0; i < C2Map::kMaxZr; ++i) { zr_iv0_[i] = 0; zr_iv1_[i] = false; }
  // Buildings (exterior + interior shape by ext sprite).
  for (int i = 0; i < tut_building_count(); ++i) {
    const TutBuilding& b = tut_building(i);
    int shape = -1;
    for (int s = 0; s < building_shape_count(); ++s) {
      const BuildingShape* bs = building_shape(s);
      if (bs && bs->ext_sprite && std::strcmp(bs->ext_sprite, b.ext_sprite) == 0) { shape = s; break; }
    }
    mapgen_.add_placed(Loc_Village, b.x, b.y, 0, shape);
  }
  // hammer_crash instances (frame 0, big_obstacle_base 0).
  for (int i = 0; i < tut_misc_count(); ++i) {
    const TutMisc& m = tut_misc(i);
    if (m.type == kTutT_HammerCrash) mapgen_.add_placed(Loc_Hummer, m.x, m.y, -2, -1);
  }
  world_.reset();
  mapgen_.apply_collision(world_);
  // Trees (obst_base 10 / big_obstacle_base per tree_block).
  tree_count_ = 0;
  for (int i = 0; i < tut_tree_count() && tree_count_ < kMaxTrees; ++i) {
    const TutTree& tr = tut_tree(i);
    trees_[tree_count_] = Vec2(tr.x, tr.y);
    tree_sub_[tree_count_] = static_cast<unsigned char>(tr.sub);
    tree_hp_[tree_count_] = 2;
    ++tree_count_;
  }
  {
    static MapProp props[64];
    int n = 0;
    for (int i = 0; i < tut_prop_count() && n < 64; ++i) {
      const TutProp& p = tut_prop(i);
      props[n].x = p.x; props[n].y = p.y;
      props[n].def = static_cast<short>(p.def); props[n].frame = static_cast<short>(p.frame);
      ++n;
    }
    c2_props_.build_list(props, n);
  }
  // items_on_ground instances + Spawn_drop 402 (Assault vest) and 450 (Taloon backpack).
  for (int i = 0; i < tut_item_count(); ++i) {
    const TutItem& it = tut_item(i);
    loot_.spawn_item(Vec2(it.x, it.y), static_cast<ItemId>(it.id), it.count > 0 ? it.count : 1);
  }
  loot_.spawn_item(Vec2(3830.f, 889.f), 402, 1);
  loot_.spawn_item(Vec2(4467.f, 795.f), 450, 1);
  // zed_resp_point frame 0: one zed_normal at the point.
  for (int i = 0; i < tut_zed_point_count(); ++i) {
    const TutZedPoint& z = tut_zed_point(i);
    zombies_.spawn(Zed_Normal, Vec2(z.x, z.y));
  }
  for (int i = 0; i < tut_misc_count(); ++i) {
    const TutMisc& m = tut_misc(i);
    if (m.type == kTutT_Crow) animals_.spawn(Animal_Crow, Vec2(m.x, m.y));
    else if (m.type == kTutT_BotTutor1) t.bot[0] = Vec2(m.x, m.y);
    else if (m.type == kTutT_BotTutor2) t.bot[1] = Vec2(m.x, m.y);
    else if (m.type == kTutT_Zone && m.iv0 >= 1 && m.iv0 <= 3) {
      t.zone_alive[m.iv0] = true;
      t.zone_pos[m.iv0] = Vec2(m.x, m.y);
      t.zone_size[m.iv0] = Vec2(m.w, m.h);
    } else if (m.type == kTutT_PlayerStart) player_.pos = Vec2(m.x, m.y);
  }
  // Player start: player_collision_base (577,1001), IV13..16 = 75, 75, 100, 75.
  player_.survival.food = 75.f; player_.survival.water = 75.f;
  player_.survival.blood = 100.f; player_.survival.temperature = 75.f;
  player_.survival.bleeding = 0.f;
  t.cam = player_.pos;
  // Tilemaps of the Tutorial layout.
  if (platform_) {
    char path[512];
    std::snprintf(path, sizeof(path), "%s/tutorial_tilemaps.txt", data_dir_);
    if (!world_tiles_.load(*platform_, path))
      std::fprintf(stderr, "[mdaz] tutorial tilemaps missing: %s\n", path);
    platform_->ground_cache_invalidate();
  }
  for (int i = 0; i < kMaxZombies; ++i) tut_saved_aggro_[i] = 0.f;
  state_ = AppState_Playing;
  audio_layout_start();
}

void Game::tut_leave_to_menu() {
  tut_.active = false;
  tutorial_finished_ = true;
  save_settings();
  if (platform_) {
    char path[512];
    std::snprintf(path, sizeof(path), "%s/world_tilemaps.txt", data_dir_);
    world_tiles_.load(*platform_, path);
    platform_->ground_cache_invalidate();
  }
  mapgen_.clear();   // CONTINUE / NEW GAME regenerate the real map
  state_ = AppState_Menu;
}

void Game::tut_show_tip(int p0, int id, int p2) {
  TutorialRun& t = tut_;
  t.tip_visible = true;
  t.tip_two = (p0 == 1);
  t.tip_top = false;
  t.tip_id = id;
  t.tip_text = tip_text_for(id, inventory_ui_.opened);
  t.dark = 0.5f;
  t.highlight = 0;
  if (p2 == 0) { t.onscreen = 0; t.blocked_gui = 1; }
  else if (p2 == 1) t.onscreen = 1;
  switch (id) {
  case 0: t.highlight = 1; t.inv_step = 0; break;
  case 1: if (!inventory_ui_.opened) { t.highlight = 2; t.inv_step = 1; } else t.inv_step = 2; break;
  case 2: t.inv_step = 2; break;
  case 12: t.inv_step = 3; break;
  case 3: t.ui_step = 3; break;
  case 6: t.ui_step = 4; t.dark = 0.75f; break;
  case 7: t.highlight = 3; t.inv_step = 4; tut_set_zeds_blind(true); break;
  case 8: t.highlight = 4; t.inv_step = 5; tut_set_zeds_blind(true); break;
  case 9: t.highlight = 5; t.inv_step = 6; tut_set_zeds_blind(true); break;
  case 10:
    t.experience_points = 316; t.highlight = 6; t.perk_step = 1;
    sfx2d("MD_Perk_1", 0.f);   // GUI_show_tutorial_tip
    break;
  case 11: t.tip_top = true; t.perk_step = 2; break;
  case 13: t.highlight = 7; t.bleed_step = 1; t.dark = 0.65f; break;
  case 14: t.timescale = 0.f; t.death = 5; break;
  default: break;
  }
}

void Game::tut_close_tip() {
  tut_.tip_visible = false;
  tut_.dark = 0.f;
  tut_.highlight = 0;
  tut_.tip_id = -1;
  tut_.onscreen = 0;
}

void Game::tut_set_zeds_blind(bool blind) {
  if (blind == tut_.zeds_blind) return;
  int i = 0;
  for (Zombie* z = zombies_.begin(); z != zombies_.end() && i < kMaxZombies; ++z, ++i) {
    if (blind) { tut_saved_aggro_[i] = z->aggro_radius; z->aggro_radius = 0.f; }
    else if (tut_saved_aggro_[i] > 0.f) z->aggro_radius = tut_saved_aggro_[i];
  }
  tut_.zeds_blind = blind;
}

void Game::tut_camera_roll(int id, Vec2 target, int kind, int index) {
  TutorialRun& t = tut_;
  t.moveblock = 1;
  t.roll_id = id; t.roll_target = target; t.roll_kind = kind; t.roll_index = index;
  t.roll_phase = 1;
  t.cam = player_.pos;
  t.timescale = 0.1f;
  if (inventory_ui_.opened) close_inventory();
}

void Game::tut_camera_roll_back() {
  tut_.blocked_gui = 0;
  tut_.tip_visible = false;
  tut_.dark = 0.f;
  tut_.roll_phase = 3;
  tut_.timescale = 0.1f;
}

void Game::tut_start_final() {
  TutorialRun& t = tut_;
  player_.survival.food = player_.survival.water = player_.survival.blood =
      player_.survival.temperature = 100.f;
  player_.survival.bleeding = 0.f;
  player_.survival.infected = false;   // IV34
  if (inventory_ui_.opened) close_inventory();
  show_craft_menu_ = false;
  t.onscreen = 1; t.blocked_gui = 1;
  t.tip_visible = false; t.menu_visible = false;
  t.final_step = 1; t.final_t = 0.f; t.say_id = 0; t.fade = 0.f; t.shots = 0;
}

void Game::tut_update_final(float dt) {
  TutorialRun& t = tut_;
  const float prev = t.final_t;
  t.final_t += dt;
  const float ft = t.final_t;
  auto crossed = [&](float at) { return prev < at && ft >= at; };
  if (crossed(1.f)) {
    player_.facing_deg = 180.f;
    for (int i = 0; i < 2; ++i) {
      float a = std::atan2(player_.pos.y - t.bot[i].y, player_.pos.x - t.bot[i].x) * 57.2957795f;
      if (i == 0) a += 10.f;
      t.bot_face[i] = a;
      t.bot_anim[i] = 2;   // run
    }
  }
  if (ft >= 1.f && ft < 3.f) {
    for (int i = 0; i < 2; ++i) {
      const float r = t.bot_face[i] * 0.01745329252f;
      t.bot[i].x += std::cos(r) * 100.f * dt;
      t.bot[i].y += std::sin(r) * 100.f * dt;
    }
  }
  if (crossed(3.f)) { t.bot_anim[0] = t.bot_anim[1] = 0; }
  if (crossed(3.3f)) { t.say_id = 125; t.say_t = 0.f; }
  if (crossed(6.3f)) { t.say_id = 126; t.say_t = 0.f; }
  if (crossed(9.3f)) t.bot_anim[0] = 1;   // aim
  if (crossed(10.3f)) { t.say_id = 127; t.say_t = 0.f; }
  if (crossed(14.3f)) { t.say_id = 0; t.fade = 1.f; }
  for (int s = 0; s < 3; ++s)
    if (crossed(14.3f + 0.1f * s)) sfx_at("Ak74_1", 0.f, Vec2(t.bot[0].x, t.bot[0].y), "m4");   // Tutorial_script_final
  if (crossed(14.5f)) sfx2d("tutor_death", 0.f, "tutor");
  if (crossed(14.5f)) { tutorial_finished_ = true; save_settings(); }
  t.say_t += dt;
  if (ft >= 15.5f) tut_leave_to_menu();
}

void Game::update_tutorial(float dt, const InputState& in) {
  TutorialRun& t = tut_;
  t.t += dt;
  if (t.final_step > 0) { tut_update_final(dt); return; }

  InputState pin = in;
  bool tip_touch = in.confirm_pressed;
  bool consumed = false;

  // Start of layout: wait 0.5 + 2 -> tip(1,3,1); wait 2 -> enable Tutorial_encounter.
  if (t.start_phase == 0 && t.t >= 2.5f) { tut_show_tip(1, 3, 1); t.start_phase = 1; }
  if (t.start_phase == 1 && t.t >= 4.5f) { t.encounter_on = true; t.start_phase = 2; }

  // --- camera_roll ---
  if (t.roll_phase == 1 || t.roll_phase == 3) {
    const Vec2 goal = t.roll_phase == 1 ? t.roll_target : player_.pos;
    const float dx = goal.x - t.cam.x, dy = goal.y - t.cam.y;
    const float len = std::sqrt(dx * dx + dy * dy), step = kTutCamSpeed * dt;
    if (len <= step) {
      t.cam = goal;
      if (t.roll_phase == 1) {
        t.roll_phase = 2;
        t.timescale = 0.f;
        t.blocked_gui = 1;
        t.tip_visible = true; t.tip_two = true; t.tip_top = false; t.tip_id = -1;
        t.tip_text = roll_text_for(t.roll_id);
        t.dark = 0.5f;
      } else {
        t.roll_phase = 0;
        t.moveblock = 0;
        t.timescale = 1.f;
      }
    } else {
      t.cam.x += dx / len * step;
      t.cam.y += dy / len * step;
    }
  } else if (t.roll_phase == 2) {
    if (tip_touch && t.blocked_gui == 1 && t.moveblock == 1) { tut_camera_roll_back(); consumed = true; }
  }

  // --- b_exit Level_menu (CurrentLevel 0) ---
  if (t.menu_visible) {
    if (t.menu_kind == 1) {
      if (in.confirm_pressed || in.back_pressed) t.menu_visible = false;   // "Ok"
    } else if (in.confirm_pressed) {   // Repair (44)
      for (int i = 0; i < inventory_.slot_count(); ++i)
        if (inventory_.slot(i).id == 50 && inventory_.slot(i).count > 0) { inventory_.remove_at(i, 1); break; }
      t.boat_frame = 1;
      t.menu_visible = false;
      if (const TutMisc* b = tut_find_misc(kTutT_BExit)) player_.pos = Vec2(b->x - 100.f, player_.pos.y);
      tut_start_final();   // pl12 IV0=2 moved onto the player -> Tutorial_script_final
      return;
    } else if (in.back_pressed) t.menu_visible = false;   // Leave (45)
    return;   // menu is modal
  }

  // --- tip touches (guides) ---
  if (!consumed && tip_touch && t.tip_visible && t.roll_phase == 0) {
    if (t.ui_step == 3 && t.tip_id == 3) { tut_show_tip(0, 6, 1); consumed = true; }
    else if (t.ui_step == 4 && t.tip_id == 6) {
      tut_close_tip(); t.ui_step = 5; t.onscreen = 0; consumed = true;   // tutorMoveChoosen = 1
    } else if (t.inv_step == 2 && t.onscreen == 1 && t.ui_step >= 4) {
      tut_show_tip(1, 12, 1); consumed = true;
    } else if (t.inv_step == 3 && t.onscreen == 1 && t.moveblock == 0) {
      tut_close_tip(); t.inv_step = 4; consumed = true;
    } else if (t.perk_step == 1 && t.tip_id == 10) { tut_show_tip(0, 11, 1); consumed = true; }
    else if (t.perk_step == 2 && t.tip_id == 11) { tut_close_tip(); t.perk_step = 3; consumed = true; }
    else if (t.bleed_step == 1 && t.tip_id == 13) { tut_close_tip(); t.bleed_step = 2; consumed = true; }
  }
  // --- button guides ---
  if (in.interact_pressed && t.inv_step == 0 && t.ui_step >= 4) { tut_close_tip(); t.inv_step = 1; }
  if (in.bag_pressed) {
    if (t.inv_step == 1 && t.ui_step >= 4 && t.moveblock == 0 && !inventory_ui_.opened) tut_show_tip(1, 2, 1);
    else if (t.inv_step == 2 && t.onscreen == 1 && t.moveblock == 0) { tut_close_tip(); t.inv_step = 4; }
    else if (t.inv_step == 3 && t.onscreen == 1 && t.moveblock == 0) { tut_close_tip(); t.inv_step = 4; }
  }
  if (in.attack_pressed && !consumed && t.inv_step == 4 && t.tip_id == 7) {
    tut_show_tip(0, 8, 1); t.moveblock = 1; consumed = true;
  }
  if ((in.reload_pressed || in.mag_reload_pressed) && t.inv_step == 5) { tut_show_tip(0, 9, 1); t.moveblock = 0; }
  if ((in.equip_delta != 0 || in.bag_pressed) && t.inv_step > 4 && t.inv_step != 7 &&
      t.perk_step == 0 && t.moveblock == 0 && t.tip_id == 9) {
    tut_close_tip(); t.inv_step = 7; tut_set_zeds_blind(false);
  }

  // --- movement block (AltMove disabled while a hint is on screen) ---
  const bool block = t.onscreen == 1 || t.roll_phase > 0;
  if (block && !inventory_ui_.opened) { pin.move_x = 0.f; pin.move_y = 0.f; pin.run = false; }
  if (consumed) { pin.attack = pin.attack_pressed = false; pin.confirm = pin.confirm_pressed = false; }
  if (t.roll_phase > 0) { pin.attack = pin.attack_pressed = false; pin.interact_pressed = false; }

  // --- simulation (timescale) ---
  const int piles_before = [&]() { int n = 0; for (int i = 0; i < loot_.count(); ++i) if (loot_.at(i)->active) ++n; return n; }();
  if (t.timescale > 0.f) {
    Vec2 zprev[kMaxZombies];
    int zi = 0;
    for (Zombie* z = zombies_.begin(); z != zombies_.end() && zi < kMaxZombies; ++z, ++zi) zprev[zi] = z->pos;
    update_playing(dt * t.timescale, pin);
    if (state_ != AppState_Playing && state_ != AppState_Paused) return;
    // pl12 zones are obstacles for zeds (bounce back).
    zi = 0;
    for (Zombie* z = zombies_.begin(); z != zombies_.end() && zi < kMaxZombies; ++z, ++zi)
      for (int k = 1; k <= 3; ++k)
        if (t.zone_alive[k] && in_rect(z->pos, t.zone_pos[k], t.zone_size[k], 0.f)) z->pos = zprev[zi];
  }
  int piles_after = 0;
  for (int i = 0; i < loot_.count(); ++i) if (loot_.at(i)->active) ++piles_after;

  // --- Inventory_guide: interact hint near loot; loot picked up ---
  if (t.inv_step == -1) {
    for (int i = 0; i < loot_.count(); ++i) {
      const LootPile* p = loot_.at(i);
      if (p->active && d2(p->pos, player_.pos) < 100.f * 100.f && tut_los(p->pos)) {
        tut_show_tip(0, 0, 2); t.inv_step = 0; break;
      }
    }
  }
  if (t.inv_step == 1 && piles_after < piles_before && !t.tip_visible) tut_show_tip(0, 1, 1);

  // --- Firearm_guide: M4 picked up (trigger once) ---
  if (!t.firearm_tip_done && t.inv_step != 7) {
    for (int i = 0; i < inventory_.slot_count(); ++i)
      if (inventory_.slot(i).id == 158) {
        t.firearm_tip_done = true;
        tut_show_tip(0, 7, 1); t.moveblock = 1;
        break;
      }
  }

  // --- Bleeding_guide ---
  bool zed_touch = false;
  for (const Zombie* z = zombies_.begin(); z != zombies_.end(); ++z)
    if (z->alive && d2(z->pos, player_.pos) < 20.f * 20.f) { zed_touch = true; break; }
  if (zed_touch && !t.zed_hit_prev && t.bleed_step == -1) {
    player_.survival.bleeding = 1.f; t.bleed_step = 0;
  }
  t.zed_hit_prev = zed_touch;
  t.bleed_t += dt;
  if (t.bleed_t >= 1.f) {
    t.bleed_t -= 1.f;
    if (t.bleed_step == 0 && player_.survival.bleeding > 0.f && !t.tip_visible) {
      for (const Zombie* z = zombies_.begin(); z != zombies_.end(); ++z)
        if (z->alive && d2(z->pos, player_.pos) < 400.f * 400.f && tut_los(z->pos)) {
          tut_show_tip(1, 13, 1); break;
        }
    }
  }

  // --- pl12 zone IV0=1 (perks) ---
  if (t.zone_alive[1] && t.onscreen == 0 && in_rect(player_.pos, t.zone_pos[1], t.zone_size[1], 10.f)) {
    t.zone_alive[1] = false;
    tut_show_tip(0, 10, 1);
    player_.survival.bleeding = 0.f;
    animals_.spawn(Animal_Crow, Vec2(7525.f, 838.f));
    animals_.spawn(Animal_Crow, Vec2(7500.f, 850.f));
  }

  // --- b_exit collision -> Level_menu ---
  if (const TutMisc* b = tut_find_misc(kTutT_BExit)) {
    static bool was_on = false;
    const bool on = boat_overlap(player_.pos, Vec2(b->x, b->y));
    if (on && !was_on && t.boat_frame == 0) {
      bool wood = false;
      for (int i = 0; i < inventory_.slot_count(); ++i)
        if (inventory_.slot(i).id == 50 && inventory_.slot(i).count > 0) wood = true;
      t.menu_visible = true;
      t.menu_kind = wood ? 3 : 1;
    }
    was_on = on;
  }

  // --- Tutorial_encounter (every 1 s) ---
  if (t.encounter_on) {
    t.encounter_t += dt;
    if (t.encounter_t >= 1.f) {
      t.encounter_t -= 1.f;
      if (t.moveblock == 0 && !show_craft_menu_ && player_.survival.blood > 0.f && t.roll_phase == 0) {
        const float r450 = 450.f * 450.f;
        bool fired = false;
        auto try_fire = [&](int id, Vec2 p, float r2) {
          if (fired || t.found[id]) return;
          if (d2(p, player_.pos) > r2 || !tut_los(p)) return;
          t.found[id] = true; fired = true;
          tut_camera_roll(id, p, id, 0);
        };
        for (const Zombie* z = zombies_.begin(); z != zombies_.end() && !fired; ++z)
          if (z->alive) try_fire(1, z->pos, r450);
        for (int i = 0; i < animals_.count() && !fired; ++i) {
          const Animal* a = animals_.at(i);
          if (!a->alive) continue;
          if (a->type == Animal_Wolf) try_fire(2, a->pos, r450);
        }
        for (int i = 0; i < tut_prop_count() && !fired; ++i)
          if (tut_prop(i).def == 3) try_fire(3, Vec2(tut_prop(i).x, tut_prop(i).y), r450);
        for (int i = 0; i < tut_car_loot_count() && !fired; ++i)
          try_fire(4, Vec2(tut_car_loot(i).x, tut_car_loot(i).y), r450);
        for (int i = 0; i < animals_.count() && !fired; ++i) {
          const Animal* a = animals_.at(i);
          if (a->alive && a->type == Animal_Deer) try_fire(5, a->pos, r450);
        }
        for (int i = 0; i < animals_.count() && !fired; ++i) {
          const Animal* a = animals_.at(i);
          if (a->alive && a->type == Animal_Rabbit) try_fire(6, a->pos, r450);
        }
        for (int i = 0; i < tut_prop_count() && !fired; ++i)
          if (tut_prop(i).def == 0 && tut_prop(i).frame == 1)
            try_fire(7, Vec2(tut_prop(i).x, tut_prop(i).y), r450);
        if (!fired)
          if (const TutMisc* b = tut_find_misc(kTutT_BExit)) try_fire(9, Vec2(b->x, b->y), r450);
        for (int i = 0; i < loot_.count() && !fired; ++i)
          if (loot_.at(i)->active) try_fire(10, loot_.at(i)->pos, r450);
        for (int i = 0; i < loot_.count() && !fired; ++i) {
          const LootPile* p = loot_.at(i);
          if (!p->active) continue;
          for (int j = 0; j < p->item_count; ++j)
            if (p->items[j] == 158) { try_fire(11, p->pos, 400.f * 400.f); break; }
        }
      }
    }
  }
}

void Game::render_tutorial_world_top(float cx, float cy, float sc) const {
  if (!platform_) return;
  const TutorialRun& t = tut_;
  const Vec2 cam = render_cam();
  // b_exit boat (wooden frame 0 / 1).
  if (const TutMisc* b = tut_find_misc(kTutT_BExit)) {
    const float sx = cx + (b->x - cam.x) * sc, sy = cy + (b->y - cam.y) * sc;
    if (platform_->has_sprite("b_exit-sheet0")) {
      if (t.boat_frame == 0)
        platform_->draw_sprite_frame("b_exit-sheet0", sx - 41.f * sc, sy - 0.5128f * 39.f * sc,
                                     82.f * sc, 39.f * sc, 144, 1, 82, 39, 0.f, 1.f);
      else
        platform_->draw_sprite_frame("b_exit-sheet0", sx - 0.5072f * 69.f * sc, sy - 0.5185f * 27.f * sc,
                                     69.f * sc, 27.f * sc, 73, 53, 69, 27, 0.f, 1.f);
    }
  }
  // bot_tutor_1 / bot_tutor_2 (stay / aim / run 4 frames at 10 fps).
  static const char* kSheets[2][2] = {{"bot_tutor_1-sheet0", "bot_tutor_1-sheet1"},
                                      {"bot_tutor_2-sheet0", "bot_tutor_2-sheet1"}};
  for (int i = 0; i < 2; ++i) {
    const float sx = cx + (t.bot[i].x - cam.x) * sc, sy = cy + (t.bot[i].y - cam.y) * sc;
    int fx = 1, fy = 1, sheet = 0;
    if (t.bot_anim[i] == 1) fx = 33;
    else if (t.bot_anim[i] == 2) {
      const int f = static_cast<int>(t.final_t * 10.f) & 3;
      static const int kRun[4][3] = {{0, 1, 33}, {0, 33, 33}, {1, 1, 1}, {1, 33, 1}};
      sheet = kRun[f][0]; fx = kRun[f][1]; fy = kRun[f][2];
    }
    if (platform_->has_sprite(kSheets[i][sheet]))
      // Characters are drawn at native size (30x30), like the player and zeds.
      platform_->draw_sprite_frame(kSheets[i][sheet], sx - 15.f, sy - 15.f, 30.f, 30.f,
                                   fx, fy, 30, 30, t.bot_face[i], 1.f);
  }
}

void Game::render_tutorial_overlay() const {
  if (!platform_) return;
  const TutorialRun& t = tut_;
  const float W = static_cast<float>(platform_->screen_w()), H = static_cast<float>(platform_->screen_h());
  const float cx = W * 0.5f, cy = H * 0.5f, sc = kTutScale;
  const Vec2 cam = render_cam();
  // fade_sprite (dark); camera_roll target is moved above it (highlight hole).
  if (t.dark > 0.f) {
    if (t.roll_phase == 2) {
      const float tx = cx + (t.roll_target.x - cam.x) * sc, ty = cy + (t.roll_target.y - cam.y) * sc;
      const float r = 22.f;
      platform_->draw_rect(0, 0, W, ty - r, 0, 0, 0, t.dark);
      platform_->draw_rect(0, ty + r, W, H - ty - r, 0, 0, 0, t.dark);
      platform_->draw_rect(0, ty - r, tx - r, 2.f * r, 0, 0, 0, t.dark);
      platform_->draw_rect(tx + r, ty - r, W - tx - r, 2.f * r, 0, 0, 0, t.dark);
    } else {
      platform_->draw_rect(0, 0, W, H, 0, 0, 0, t.dark);
    }
  }
  if (t.tip_visible) {
    const float bw = t.tip_two ? 310.f : 247.f, bh = t.tip_two ? 63.f : 43.f;
    const float hy = t.tip_two ? 0.476f : 0.5116f;
    const float y0 = t.tip_top ? 75.f : H - 75.f;
    const float x = cx - bw * 0.5f, y = y0 - bh * hy;
    if (platform_->has_sprite("gui_tutor_tip_bg-sheet0")) {
      if (t.tip_two) platform_->draw_sprite_frame("gui_tutor_tip_bg-sheet0", x, y, bw, bh, 1, 1, 310, 63, 0.f, 1.f);
      else platform_->draw_sprite_frame("gui_tutor_tip_bg-sheet0", x, y, bw, bh, 1, 66, 247, 43, 0.f, 1.f);
    } else {
      platform_->draw_rect(x, y, bw, bh, 0.1f, 0.1f, 0.1f, 0.85f);
    }
    const char* s = tut_ui_text(t.tip_text);
    if (s && *s) {
      // "two" carries the confirm (check mark) button on its right ~75 px.
      const int lines = t.tip_two ? 5 : 4;
      text_wrap(*platform_, x + 10.f, y + (t.tip_two ? 6.f : 3.f), bw - (t.tip_two ? 90.f : 20.f), lines, s, 1.f, 1.f, 1.f);
    }
  }
  if (t.menu_visible) {
    const float bw = 300.f, bh = 90.f, x = cx - bw * 0.5f, y = cy - bh * 0.5f;
    platform_->draw_rect(0, 0, W, H, 0, 0, 0, 0.5f);
    platform_->draw_rect(x, y, bw, bh, 0.12f, 0.12f, 0.1f, 0.95f);
    text_wrap(*platform_, x + 10.f, y + 8.f, bw - 20.f, 5, tut_ui_text(t.menu_kind == 1 ? 42 : 46), 1.f, 1.f, 1.f);
    if (t.menu_kind == 1) {
      text_center(*platform_, cx, y + bh - 16.f, tut_ui_text(41), 1.f, 0.9f, 0.5f);
    } else {
      text_center(*platform_, cx - 60.f, y + bh - 16.f, tut_ui_text(44), 1.f, 0.9f, 0.5f);
      text_center(*platform_, cx + 60.f, y + bh - 16.f, tut_ui_text(45), 1.f, 0.9f, 0.5f);
    }
  }
  if (t.final_step > 0) {
    if (t.say_id > 0) {
      // White Text pinned 25 px above the speaker; shown 3 s (127: 4 s), then fades.
      const float hold = t.say_id == 127 ? 4.f : 3.f;
      const float a = t.say_t < hold ? 1.f : (t.say_t < hold + 1.f ? hold + 1.f - t.say_t : 0.f);
      if (a > 0.f) {
        const Vec2 who = t.say_id == 125 ? player_.pos : t.bot[0];
        const float sx = cx + (who.x - cam.x) * sc, sy = cy + (who.y - cam.y) * sc - 25.f;
        text_center(*platform_, sx, sy, tut_log_text(t.say_id), a, a, a);
      }
    }
    if (t.fade > 0.f) platform_->draw_rect(0, 0, W, H, 0, 0, 0, t.fade);
  }
}

} // namespace mdaz
