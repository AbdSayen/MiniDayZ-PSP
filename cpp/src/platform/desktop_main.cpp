#include "mdaz/audio.hpp"
#include "mdaz/game.hpp"
#include "mdaz/platform.hpp"
#include "mdaz/platform_null.hpp"
#include "mdaz/psp_ui.hpp"

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

// --shots <dir>: plays a fixed input script and dumps BMP frames, then quits.
class ScriptedPlatform : public mdaz::IPlatform {
public:
  ScriptedPlatform(mdaz::IPlatform* inner, const char* dir) : inner_(inner), dir_(dir), frame_(0) {}
  ~ScriptedPlatform() { delete inner_; }
  bool init(const char* t, int w, int h) { return inner_->init(t, w, h); }
  void shutdown() { inner_->shutdown(); }
  double time_seconds() { return frame_ / 60.0; }
  void begin_frame() { inner_->begin_frame(); }
  void clear(float r, float g, float b) { inner_->clear(r, g, b); }
  void draw_rect(float x, float y, float w, float h, float r, float g, float b, float a) {
    inner_->draw_rect(x, y, w, h, r, g, b, a);
  }
  void draw_text(float x, float y, const char* s, float r, float g, float b) {
    inner_->draw_text(x, y, s, r, g, b);
  }
  bool load_sprites(const char* d) { return inner_->load_sprites(d); }
  bool has_sprite(const char* n) const { return inner_->has_sprite(n); }
  void draw_sprite(const char* n, float x, float y, float w, float h, float a, float al) {
    inner_->draw_sprite(n, x, y, w, h, a, al);
  }
  void draw_sprite_frame(const char* n, float dx, float dy, float dw, float dh,
                         int sx, int sy, int sw, int sh, float a, float al) {
    inner_->draw_sprite_frame(n, dx, dy, dw, dh, sx, sy, sw, sh, a, al);
  }
  void draw_sprite_frame_mirrored(const char* n, float dx, float dy, float dw, float dh,
                                  int sx, int sy, int sw, int sh, float al) {
    inner_->draw_sprite_frame_mirrored(n, dx, dy, dw, dh, sx, sy, sw, sh, al);
  }
  bool read_file(const char* p, char** d, std::size_t* s) { return inner_->read_file(p, d, s); }
  bool write_file(const char* p, const void* d, std::size_t s) { return inner_->write_file(p, d, s); }
  int screen_w() const { return inner_->screen_w(); }
  int screen_h() const { return inner_->screen_h(); }

  void set_game(mdaz::Game* g, bool building, bool door = false, bool intro = false, bool wreck = false,
                bool tutorial = false) {
    game_ = g; building_ = building; door_ = door; intro_ = intro; wreck_ = wreck; tutorial_ = tutorial;
    if (!intro && g) g->skip_intro(); // keep the existing shot scripts' frame numbers
  }

  bool poll(mdaz::InputState& in) {
    if (!inner_->poll(in)) return false;
    in = mdaz::InputState();
    ++frame_;
    if (intro_) return frame_ < 240;
    if (door_) return poll_door(in);
    if (wreck_) return poll_wreck(in);
    if (tutorial_) return poll_tutorial(in);
    if (quest_) {
      if (frame_ == 60) { in.confirm = in.confirm_pressed = true; }
      if (game_ && frame_ == 200) game_->harness_set_quest(1, 1, 60);
      if (game_ && frame_ == 210) game_->harness_set_quest(2, 1, 0);
      if (game_ && frame_ == 220) game_->harness_set_quest(3, 1, 88);
      return frame_ < 230;
    }
    if (carhit_) {
      // 6 car models: place 20 px south of the polygon, walk north into it for 30 frames.
      static const int kSubs[] = { 1, 2, 8, 13, 14, 15 };
      if (frame_ == 60) { in.confirm = in.confirm_pressed = true; }
      if (game_ && frame_ == 190) game_->debug_hitbox_ = true;
      for (int j = 0; j < 6; ++j) {
        const int f = 200 + j * 60;
        if (game_ && frame_ == f) { carhit_y1_ = game_->harness_place_at_car(kSubs[j]); std::printf("[carhit] sub %d polygon bottom y %.1f player y %.1f\n", kSubs[j], carhit_y1_, game_->harness_player_y()); }
        if (frame_ > f + 2 && frame_ < f + 40) in.move_y = -1.f;
        if (game_ && frame_ == f + 45) std::printf("[carhit] sub %d after walking north: player y %.1f (polygon bottom %.1f, gap %.1f)\n", kSubs[j], game_->harness_player_y(), carhit_y1_, game_->harness_player_y() - carhit_y1_);
      }
      return frame_ < 200 + 6 * 60;
    }
    if (trunk_) {
      if (frame_ == 60) { in.confirm = in.confirm_pressed = true; }
      if (game_ && frame_ == 200) { game_->harness_place_at_trunk(false); float d = 0; game_->harness_trunk_near(d); std::printf("[trunk] placed %.0f px from point, alive=%d\n", d, game_->harness_trunks_alive()); }
      if (frame_ == 210) in.interact = in.interact_pressed = true;   // Circle: walk_marker
      if (game_ && (frame_ == 205 || (frame_ > 210 && frame_ <= 400 && frame_ % 10 == 0))) {
        static int k0 = -1; float d = 0; const int kn = game_->harness_trunk_near(d); if (frame_ == 205) k0 = kn; const int k = k0;
        std::printf("[trunk] f%d stage=%d d=%.1f car_frame=%d opened=%d ach_gta=%d\n", frame_, game_->harness_trunk_walk_stage(), d,
                    k >= 0 ? game_->harness_prop_frame(k) : -1, game_->trunk_opened_, game_->ach_gta());
      }
      if (game_ && frame_ >= 400 && frame_ < 1000 && frame_ % 10 == 0) game_->harness_place_at_trunk(true);
      if (frame_ >= 400 && frame_ < 1000 && frame_ % 10 == 2) in.interact = in.interact_pressed = true;
      if (game_ && frame_ == 1001) std::printf("[trunk] opened=%d found=%d alive=%d\n", game_->trunk_opened_, game_->trunk_found_, game_->harness_trunks_alive());
      return frame_ < 1010;
    }
    if (chop_) {
      if (frame_ == 60) { in.confirm = in.confirm_pressed = true; }
      if (game_ && frame_ == 200) { chop_tree_ = game_->harness_place_at_tree(); chop_log("placed"); }
      // Circle at frame 7: walk_marker -> swing; second Circle after Reloading_time_melee (1.6 s).
      if (frame_ == 240 || frame_ == 360) in.interact = in.interact_pressed = true;
      if (game_ && (frame_ == 300 || frame_ == 420)) chop_log(frame_ == 300 ? "hit1" : "hit2");
      if (frame_ == 470 || frame_ == 500 || frame_ == 530) in.interact = in.interact_pressed = true;  // pick up
      if (game_ && frame_ == 560) chop_log("pickup");
      if (frame_ == 570) { in.bag = in.bag_pressed = true; }
      return frame_ < 600;
    }
    if (death_) {
      if (frame_ == 60) { in.confirm = in.confirm_pressed = true; }
      if (frame_ == 200 && game_) game_->harness_death_demo();
      return frame_ < 330;
    }
    if (building_) return poll_building(in);
    // Menu -> NEW GAME (focus 0).
    if (frame_ == 60) { in.confirm = in.confirm_pressed = true; }
    if (frame_ > 200 && frame_ < 420) in.move_x = 1.f;
    // Inventory: SELECT opens, D-pad moves focus, SELECT closes.
    if (frame_ == 480) { in.bag = in.bag_pressed = true; }
    if (frame_ == 490 || frame_ == 494) in.move_x = 1.f;
    if (frame_ == 498) in.move_y = 1.f;
    if (frame_ == 515) { in.bag = in.bag_pressed = true; }
    // Craft menu: R+Square opens, D-pad down, Circle closes.
    if (frame_ == 525) { in.craft_pressed = true; }
    if (frame_ == 530 || frame_ == 534) in.move_y = 1.f;
    if (frame_ == 548) { in.back_pressed = true; }
    if (frame_ > 560 && frame_ < 1400) { in.move_x = -0.7f; in.move_y = 0.7f; in.run = true; }
    // Pause menu: START, D-pad down once (SAVE focus) for the shot, then CONTROLS.
    if (frame_ == 1500) { in.pause = in.pause_pressed = true; }
    if (frame_ == 1515) in.move_y = 1.f;
    if (frame_ == 1535 || frame_ == 1540 || frame_ == 1545) in.move_y = 1.f;
    if (frame_ == 1555) { in.confirm = in.confirm_pressed = true; }
    return frame_ < 1580;
  }

  // --shots-building: test-only placement next to a Solid building, then walk into its walls.
  bool poll_building(mdaz::InputState& in) {
    if (frame_ == 60) { in.confirm = in.confirm_pressed = true; }
    if (frame_ == 200 && game_) game_->harness_place_at_building(std::getenv("MDAZ_HARNESS_LOOT") ? 3 : 1); // south of the building
    if (frame_ > 260 && frame_ < 420) in.move_y = -1.f;              // walk north into the wall
    if (frame_ == 480 && game_) game_->harness_place_at_building(0); // inside (warmzone)
    if (frame_ > 540 && frame_ < 700) in.move_x = -1.f;              // walk west into a wall
    return frame_ < 760;
  }

  // --shots-wreck: test-only placements at a Humvee wreck, a heli wreck and a zed_resp_point.
  bool poll_wreck(mdaz::InputState& in) {
    if (frame_ == 60) { in.confirm = in.confirm_pressed = true; }
    if (std::getenv("MDAZ_HARNESS_PUMP")) {   // waterpump: prompt, Circle, drink
      if (frame_ == 200 && game_) game_->harness_place_at_pump();
      if (frame_ == 270) in.interact = in.interact_pressed = true;
      return frame_ < 470;
    }
    if (frame_ == 200 && game_) game_->harness_place_at_wreck(0);
    if (frame_ == 300 && game_) game_->harness_place_at_wreck(1);
    if (frame_ == 400 && game_) game_->harness_place_at_wreck(2);
    return frame_ < 470;
  }

  // --shots-tutorial: start the tutorial and walk the guides, driven by the tutorial state.
  // Placements and the Wood piles grant are test-only shortcuts (the route is ~7000 px).
  // Every camera_roll stop is captured as tut_roll_<text id>.bmp and dismissed.
  void shot(const char* name) {
    char path[512];
    std::snprintf(path, sizeof(path), "%s/%s.bmp", dir_, name);
    pending_shot_ = true;
    std::snprintf(pending_path_, sizeof(pending_path_), "%s", path);
  }
  bool poll_tutorial(mdaz::InputState& in) {
    if (!game_) return false;
    if (frame_ == 1) { game_->start_tutorial(); step_ = 1; wait_ = 0; return true; }
    const mdaz::Game::TutorialView v = game_->tutorial_view();
    auto ok = [&]() { in.confirm = in.confirm_pressed = true; };
    if (game_->tutorial_active() && v.camera_paused) {
      ++roll_wait_;
      if (roll_wait_ == 15) { char n[64]; std::snprintf(n, sizeof(n), "tut_roll_%d", v.tip_text_id); shot(n); }
      if (roll_wait_ == 25) ok();
      return frame_ < 5200;
    }
    roll_wait_ = 0;
    if (v.camera_rolling) return frame_ < 5200;
    // Bleeding_guide tip 13 (465) can appear whenever a zed reached the player: capture + close.
    if (v.tip_visible && v.tip_text_id == 465) {
      ++bleed_wait_;
      if (bleed_wait_ == 15) shot("tut_bleeding_tip13_465");
      if (bleed_wait_ == 25) ok();
      return frame_ < 5200;
    }
    bleed_wait_ = 0;
    ++wait_;
    auto tip_is = [&](int id) { return v.tip_visible && v.tip_text_id == id; };
    auto next = [&]() { ++step_; wait_ = 0; };
    switch (step_) {
    case 1: if (tip_is(455) && wait_ > 20) { shot("tut_01_tip3_455"); next(); } break;
    case 2: if (wait_ == 5) { ok(); next(); } break;
    case 3: if (tip_is(458) && wait_ > 20) { shot("tut_02_tip6_458"); next(); } break;
    case 4: if (wait_ == 5) { ok(); next(); } break;
    case 5: if (!v.tip_visible && wait_ > 20) { game_->harness_set_pos(1376.f, 724.f); next(); } break;
    case 6: if (tip_is(461) && wait_ > 10) { shot("tut_03_tip0_interact_461"); next(); } break;
    case 7: if (wait_ == 10) { in.interact = in.interact_pressed = true; next(); } break;
    case 8: if (tip_is(453) && wait_ > 15) { shot("tut_04_tip1_inventory_453"); next(); }
            else if (wait_ > 120) { shot("tut_04_FAILED"); next(); } break;
    case 9: if (wait_ == 5) { in.bag = in.bag_pressed = true; next(); } break;
    case 10: if (tip_is(454) && wait_ > 15) { shot("tut_05_tip2_hands_454"); next(); } break;
    case 11: if (wait_ == 5) { ok(); next(); } break;
    case 12: if (tip_is(467) && wait_ > 15) { shot("tut_06_tip12_drag_467"); next(); } break;
    case 13: if (wait_ == 5) { ok(); next(); } break;
    case 14: if (wait_ == 15) { in.bag = in.bag_pressed = true; next(); } break;   // close inventory
    case 15: if (wait_ == 15) { game_->harness_set_pos(3895.f, 905.f); next(); } break;
    case 16: if (tip_is(462)) { next(); }
             else if (wait_ % 30 == 0 && wait_ >= 90) in.interact = in.interact_pressed = true;
             break;
    case 17: if (wait_ == 15) { shot("tut_07_tip7_attack_462"); next(); } break;
    case 18: if (wait_ == 5) { in.attack = in.attack_pressed = true; next(); } break;
    case 19: if (tip_is(463) && wait_ > 15) { shot("tut_08_tip8_reload_463"); next(); } break;
    case 20: if (wait_ == 5) { in.reload = in.reload_pressed = true; next(); } break;
    case 21: if (tip_is(464) && wait_ > 15) { shot("tut_09_tip9_switch_464"); next(); } break;
    case 22: if (wait_ == 5) { in.equip_delta = 1; next(); } break;
    case 23: if (!v.tip_visible && wait_ > 15) { game_->harness_set_pos(4950.f, 900.f); next(); } break;
    case 24: if (wait_ == 150) { shot("tut_10_zeds"); next(); } break;
    case 25: if (wait_ == 10) { game_->harness_set_pos(6480.f, 930.f); next(); } break;
    case 26: if (tip_is(459)) next(); else if (wait_ > 60) in.move_x = 1.f; break;
    case 27: if (wait_ == 15) { shot("tut_11_tip10_perks_459"); next(); } break;
    case 28: if (wait_ == 5) { ok(); next(); } break;
    case 29: if (tip_is(460) && wait_ > 15) { shot("tut_12_tip11_460"); next(); } break;
    case 30: if (wait_ == 5) { ok(); next(); } break;
    case 31: if (wait_ == 15) { game_->harness_set_pos(7480.f, 834.f); next(); } break;
    case 32: if (v.menu_visible) next(); else if (wait_ > 90) in.move_x = 1.f; break;
    case 33: if (wait_ == 15) { shot("tut_13_boat_no_wood_42"); next(); } break;
    case 34: if (wait_ == 5) { ok(); game_->harness_give_item(50, 1); next(); } break;
    case 35: if (wait_ < 30) in.move_x = -1.f; else next(); break;
    case 36: if (v.menu_visible) next(); else in.move_x = 1.f; break;
    case 37: if (wait_ == 15) { shot("tut_14_boat_repair_46"); next(); } break;
    case 38: if (wait_ == 5) { ok(); next(); } break;
    case 39:
      if (wait_ == 240) shot("tut_15_final_125");
      if (wait_ == 420) shot("tut_16_final_126");
      if (wait_ == 660) shot("tut_17_final_127");
      if (wait_ == 870) shot("tut_18_final_black");
      if (!game_->tutorial_active() && wait_ > 900) next();
      break;
    case 40: if (wait_ == 30) { shot("tut_19_menu_after"); next(); } break;
    default: return wait_ < 10;
    }
    return frame_ < 5200;
  }

  // --shots-door: start south of the police station (one test placement), push north into the
  // wall east of the door, side-step west to the door gap (x -31..-1), then walk in. Input only.
  bool poll_door(mdaz::InputState& in) {
    if (frame_ == 60) { in.confirm = in.confirm_pressed = true; }
    if (frame_ == 200 && game_) game_->harness_place_at_building(2);
    if (frame_ > 220 && frame_ <= 310) in.move_y = -1.f;               // ~1.5 s into the wall
    else if (frame_ > 310 && game_ && door_phase_ == 0) {
      if (game_->harness_player_rel().x > -16.f) in.move_x = -1.f; else door_phase_ = 1;
    }
    if (door_phase_ == 1 && frame_ > 310) {                             // through the door,
      if (game_->harness_player_rel().y > 60.f) in.move_y = -1.f;      // stop once inside
      else door_phase_ = 2;
    }
    return frame_ < 620;
  }

  void end_frame() {
    if (intro_) {
      static const int kF[] = { 1, 60, 120, 200, 215, 230 };
      for (unsigned i = 0; i < sizeof(kF) / sizeof(kF[0]); ++i) {
        if (kF[i] != frame_ || shot_done_[i]) continue;
        shot_done_[i] = true;
        char path[512];
        std::snprintf(path, sizeof(path), "%s/intro_f%03d.bmp", dir_, frame_);
        std::printf("%s %s\n", inner_->save_screenshot(path) ? "saved" : "FAILED", path);
      }
      inner_->end_frame();
      return;
    }
    if (door_) {
      if (frame_ >= 210 && frame_ <= 600 && (frame_ - 210) % 35 == 0) {
        char path[512];
        std::snprintf(path, sizeof(path), "%s/door_%02d_f%d.bmp", dir_, (frame_ - 210) / 35, frame_);
        std::printf("%s %s\n", inner_->save_screenshot(path) ? "saved" : "FAILED", path);
      }
      inner_->end_frame();
      return;
    }
    if (wreck_) {
      static const struct { int frame; const char* name; } kW[] = {
        { 250, "wreck_hummer" }, { 350, "wreck_heli" }, { 402, "zr_spawn_t0" }, { 460, "zr_spawn_t1" },
      };
      static const struct { int frame; const char* name; } kP[] = {
        { 260, "pump_1_prompt" }, { 300, "pump_2_walk" }, { 330, "pump_3_drinking" }, { 450, "pump_4_drunk" },
      };
      if (std::getenv("MDAZ_HARNESS_PUMP")) {
        for (unsigned i = 0; i < sizeof(kP) / sizeof(kP[0]); ++i) {
          if (kP[i].frame != frame_) continue;
          char path[512];
          std::snprintf(path, sizeof(path), "%s/%s.bmp", dir_, kP[i].name);
          std::printf("%s %s\n", inner_->save_screenshot(path) ? "saved" : "FAILED", path);
        }
        inner_->end_frame();
        return;
      }
      for (unsigned i = 0; i < sizeof(kW) / sizeof(kW[0]); ++i) {
        if (kW[i].frame != frame_) continue;
        char path[512];
        std::snprintf(path, sizeof(path), "%s/%s.bmp", dir_, kW[i].name);
        std::printf("%s %s\n", inner_->save_screenshot(path) ? "saved" : "FAILED", path);
      }
      inner_->end_frame();
      return;
    }
    if (quest_) {
      static const struct { int frame; const char* name; } kQ[] = {
        { 205, "quest_1_item" }, { 215, "quest_2_bandits" }, { 225, "quest_3_case" },
      };
      for (unsigned i = 0; i < sizeof(kQ) / sizeof(kQ[0]); ++i) {
        if (kQ[i].frame != frame_) continue;
        char path[512];
        std::snprintf(path, sizeof(path), "%s/%s.bmp", dir_, kQ[i].name);
        std::printf("%s %s\n", inner_->save_screenshot(path) ? "saved" : "FAILED", path);
      }
      inner_->end_frame();
      return;
    }
    if (carhit_) {
      static const int kSubs[] = { 1, 2, 8, 13, 14, 15 };
      for (int j = 0; j < 6; ++j) {
        const int f = 200 + j * 60;
        const char* tag = frame_ == f + 1 ? "a_placed" : (frame_ == f + 45 ? "b_stopped" : 0);
        if (!tag) continue;
        char path[512];
        std::snprintf(path, sizeof(path), "%s/carhit_sub%02d_%s.bmp", dir_, kSubs[j], tag);
        std::printf("%s %s\n", inner_->save_screenshot(path) ? "saved" : "FAILED", path);
      }
      inner_->end_frame();
      return;
    }
    if (trunk_) {
      static const struct { int frame; const char* name; } kT[] = {
        { 205, "trunk_0_near" }, { 230, "trunk_1_walking" }, { 320, "trunk_2_opened" }, { 398, "trunk_3_lid_up" },
      };
      for (unsigned i = 0; i < sizeof(kT) / sizeof(kT[0]); ++i) {
        if (kT[i].frame != frame_) continue;
        char path[512];
        std::snprintf(path, sizeof(path), "%s/%s.bmp", dir_, kT[i].name);
        std::printf("%s %s\n", inner_->save_screenshot(path) ? "saved" : "FAILED", path);
      }
      inner_->end_frame();
      return;
    }
    if (chop_) {
      static const struct { int frame; const char* name; } kC[] = {
        { 235, "chop_0_before_prompt" }, { 300, "chop_1_hit1" }, { 440, "chop_2_hit2_stump_drop" }, { 595, "chop_3_inventory" },
      };
      for (unsigned i = 0; i < sizeof(kC) / sizeof(kC[0]); ++i) {
        if (kC[i].frame != frame_) continue;
        char path[512];
        std::snprintf(path, sizeof(path), "%s/%s.bmp", dir_, kC[i].name);
        std::printf("%s %s\n", inner_->save_screenshot(path) ? "saved" : "FAILED", path);
      }
      inner_->end_frame();
      return;
    }
    if (death_) {
      static const struct { int frame; const char* name; } kD[] = {
        { 199, "death_0_before" }, { 203, "death_1_f0" }, { 216, "death_2_f2" }, { 232, "death_3_f4" }, { 320, "death_4_rest" },
      };
      for (unsigned i = 0; i < sizeof(kD) / sizeof(kD[0]); ++i) {
        if (kD[i].frame != frame_) continue;
        char path[512];
        std::snprintf(path, sizeof(path), "%s/%s.bmp", dir_, kD[i].name);
        std::printf("%s %s\n", inner_->save_screenshot(path) ? "saved" : "FAILED", path);
      }
      inner_->end_frame();
      return;
    }
    if (tutorial_) {
      if (pending_shot_) {
        pending_shot_ = false;
        std::printf("%s %s\n", inner_->save_screenshot(pending_path_) ? "saved" : "FAILED", pending_path_);
      }
      inner_->end_frame();
      return;
    }
    if (building_) {
      static const struct { int frame; const char* name; } kB[] = {
        { 250, "bld_outside" }, { 430, "bld_north_wall" }, { 530, "bld_inside" }, { 710, "bld_west_wall" },
      };
      for (unsigned i = 0; i < sizeof(kB) / sizeof(kB[0]); ++i) {
        if (kB[i].frame != frame_) continue;
        char path[512];
        std::snprintf(path, sizeof(path), "%s/%s.bmp", dir_, kB[i].name);
        std::printf("%s %s\n", inner_->save_screenshot(path) ? "saved" : "FAILED", path);
      }
      inner_->end_frame();
      return;
    }
    static const struct { int frame; const char* name; } kShots[] = {
      { 50, "menu" }, { 180, "spawn" },
      { 220, "run_a" }, { 226, "run_b" }, { 232, "run_c" }, { 238, "run_d" },
      { 430, "walk" }, { 510, "bag" }, { 545, "craft" },
      { 900, "explore1" }, { 1390, "explore2" }, { 1530, "pause" }, { 1570, "controls" },
    };
    for (unsigned i = 0; i < sizeof(kShots) / sizeof(kShots[0]); ++i) {
      if (kShots[i].frame != frame_) continue;
      char path[512];
      std::snprintf(path, sizeof(path), "%s/%s.bmp", dir_, kShots[i].name);
      std::printf("%s %s\n", inner_->save_screenshot(path) ? "saved" : "FAILED", path);
    }
    inner_->end_frame();
  }

private:
  mdaz::IPlatform* inner_;
  const char* dir_;
  int frame_;
  mdaz::Game* game_ = 0;
  bool building_ = false;
  bool wreck_ = false;
  bool tutorial_ = false;
  bool death_ = false;
public:
  void set_death(bool d) { death_ = d; }
  bool chop_ = false;
  bool trunk_ = false;
  bool carhit_ = false;
  float carhit_y1_ = 0.f;
  int chop_tree_ = -1;
  void chop_log(const char* tag) {
    std::printf("[chop] %s tree=%d hp=%d trees=%d ground: sticks(38)=%d firewood(50)=%d bag: sticks=%d firewood=%d\n", tag, chop_tree_,
                game_->harness_tree_hp(chop_tree_), game_->harness_tree_count(), game_->harness_loot_count(38),
                game_->harness_loot_count(50), game_->harness_inv_count(38), game_->harness_inv_count(50));
  }
  bool quest_ = false;
  void set_quest(bool q) { quest_ = q; }
private:
  int step_ = 0, wait_ = 0, roll_wait_ = 0, bleed_wait_ = 0;
  bool pending_shot_ = false;
  char pending_path_[512] = {0};
  bool door_ = false;
  bool intro_ = false;
  bool shot_done_[8] = {};
  int door_phase_ = 0;
};

} // namespace

int main(int argc, char** argv) {
  const char* data_dir = "data";
  const char* shots_dir = 0;
  bool shots_building = false;
  bool shots_door = false;
  bool shots_intro = false;
  bool shots_wreck = false;
  bool shots_tutorial = false;
  bool shots_death = false;
  bool shots_quest = false;
  bool shots_chop = false;
  bool shots_trunk = false;
  bool shots_carhit = false;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--shots") == 0 && i + 1 < argc) shots_dir = argv[++i];
    else if (std::strcmp(argv[i], "--shots-building") == 0 && i + 1 < argc) {
      shots_dir = argv[++i];
      shots_building = true;
    }
    else if (std::strcmp(argv[i], "--shots-intro") == 0 && i + 1 < argc) {
      shots_dir = argv[++i];
      shots_intro = true;
    }
    else if (std::strcmp(argv[i], "--shots-wreck") == 0 && i + 1 < argc) {
      shots_dir = argv[++i];
      shots_wreck = true;
    }
    else if (std::strcmp(argv[i], "--shots-death") == 0 && i + 1 < argc) {
      shots_dir = argv[++i];
      shots_death = true;
    }
    else if (std::strcmp(argv[i], "--shots-carhit") == 0 && i + 1 < argc) {
      shots_dir = argv[++i];
      shots_carhit = true;
    }
    else if (std::strcmp(argv[i], "--shots-trunk") == 0 && i + 1 < argc) {
      shots_dir = argv[++i];
      shots_trunk = true;
    }
    else if (std::strcmp(argv[i], "--shots-chop") == 0 && i + 1 < argc) {
      shots_dir = argv[++i];
      shots_chop = true;
    }
    else if (std::strcmp(argv[i], "--shots-quest") == 0 && i + 1 < argc) {
      shots_dir = argv[++i];
      shots_quest = true;
    }
    else if (std::strcmp(argv[i], "--shots-tutorial") == 0 && i + 1 < argc) {
      shots_dir = argv[++i];
      shots_tutorial = true;
    }
    else if (std::strcmp(argv[i], "--shots-door") == 0 && i + 1 < argc) {
      shots_dir = argv[++i];
      shots_door = true;
    }
    else if (std::strcmp(argv[i], "--keys") == 0) mdaz::ui::set_keyboard_hints(true); // keyboard caps in hints
    else data_dir = argv[i];
  }

  mdaz::IPlatform* platform = mdaz::create_platform();
  if (!platform) {
    std::fprintf(stderr, "Failed to create platform\n");
    return 1;
  }
  ScriptedPlatform* scripted = 0;
  if (shots_dir) platform = scripted = new ScriptedPlatform(platform, shots_dir);

  // 2x PSP resolution window with logical 480x272 render
  if (!platform->init("Mini DAYZ C++ (1.4.1)", mdaz::kPspWidth * 2, mdaz::kPspHeight * 2)) {
    std::fprintf(stderr, "Platform init failed — falling back to null\n");
    platform->shutdown();
    delete platform;
    scripted = 0;
    platform = mdaz::create_null_platform();
    platform->init("Mini DAYZ C++", mdaz::kPspWidth, mdaz::kPspHeight);
  }

  mdaz::IAudio* audio = mdaz::create_sdl_audio();
  // Original 1.4.1 Construct media pack
  audio->set_media_dir("C:/Users/abdur/Desktop/MiniDayZ/original_docs/media");

  mdaz::Game game;
  if (!game.init(platform, data_dir)) {
    std::fprintf(stderr, "Game init failed\n");
    delete audio;
    platform->shutdown();
    delete platform;
    return 1;
  }
  game.set_audio(audio);
  if (scripted) scripted->set_game(&game, shots_building, shots_door, shots_intro, shots_wreck, shots_tutorial);
  if (scripted) scripted->set_death(shots_death);
  if (scripted) scripted->set_quest(shots_quest);
  if (scripted) scripted->chop_ = shots_chop;
  if (scripted) scripted->trunk_ = shots_trunk;
  if (scripted) scripted->carhit_ = shots_carhit;

  double prev = platform->time_seconds();
  while (!game.wants_quit()) {
    double now = platform->time_seconds();
    float dt = static_cast<float>(now - prev);
    prev = now;
    if (dt < 0.f) dt = 0.f;
    if (dt > 0.1f) dt = 0.1f;
    game.update(dt);
    game.render();
  }

  game.shutdown();
  delete audio;
  platform->shutdown();
  delete platform;
  return 0;
}
