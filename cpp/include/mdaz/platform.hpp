#pragma once

#include "mdaz/types.hpp"
#include <cstdio>

namespace mdaz {

struct InputState {
  float move_x;
  float move_y;
  bool attack;          // held
  bool attack_pressed;  // rising edge
  bool interact;
  bool interact_pressed;
  bool inventory;
  bool inventory_pressed;
  bool reload;
  bool reload_pressed;
  bool run;
  bool crouch; // Ctrl — quieter / slower
  bool pause;
  bool pause_pressed;
  bool confirm;
  bool confirm_pressed;
  bool cancel;
  bool save_pressed;
  bool load_pressed;
  bool torch_pressed;
  bool drop_pressed;       // G — drop one from equipped slot
  bool mag_reload_pressed; // V — mag/chamber reload (same as Tap_reload / R)
  bool craft_pressed;      // B — toggle craft recipe list (Call_sub_menu_craft)
  bool throw_pressed;      // F — throw stick / improvised projectile
  bool bag; // hold Tab to show inventory list
  bool bag_pressed;
  int slot_digit; // 1..9 pressed this frame, else 0
  int equip_delta; // -1 / +1 from Q/X or mouse wheel
  // Menu "back" (PSP Circle, desktop Esc/Backspace/E) — rising edge.
  bool back_pressed;
  // D-pad / analog navigation edges with auto-repeat — filled by Game, not platforms.
  bool nav_up;
  bool nav_down;
  bool nav_left;
  bool nav_right;
  float mouse_x;
  float mouse_y;
  bool mouse_left;
  bool mouse_left_pressed;

  InputState()
    : move_x(0), move_y(0),
      attack(false), attack_pressed(false),
      interact(false), interact_pressed(false),
      inventory(false), inventory_pressed(false),
      reload(false), reload_pressed(false), run(false), crouch(false),
      pause(false), pause_pressed(false),
      confirm(false), confirm_pressed(false),
      cancel(false), save_pressed(false), load_pressed(false),
      torch_pressed(false), drop_pressed(false), mag_reload_pressed(false),
      craft_pressed(false), throw_pressed(false), bag(false), bag_pressed(false),
      slot_digit(0),
      equip_delta(0),
      back_pressed(false), nav_up(false), nav_down(false), nav_left(false), nav_right(false),
      mouse_x(0.f), mouse_y(0.f), mouse_left(false), mouse_left_pressed(false) {}
};

class IPlatform {
public:
  virtual ~IPlatform() {}
  virtual bool init(const char* title, int width, int height) = 0;
  virtual void shutdown() = 0;
  virtual bool poll(InputState& out) = 0;
  virtual double time_seconds() = 0;
  virtual void begin_frame() = 0;
  virtual void clear(float r, float g, float b) = 0;
  virtual void draw_rect(float x, float y, float w, float h, float r, float g, float b, float a) = 0;
  virtual void draw_text(float x, float y, const char* text, float r, float g, float b) = 0;
  // Original C2 PNG sprites (name without path, e.g. "player_skin_def_1-sheet0").
  virtual bool load_sprites(const char* images_dir) { (void)images_dir; return false; }
  virtual bool has_sprite(const char* /*name*/) const { return false; }
  // Draw whole texture (buildings / single-frame art).
  virtual void draw_sprite(const char* /*name*/, float /*x*/, float /*y*/, float /*w*/, float /*h*/,
                           float /*angle_deg*/ = 0.f, float /*alpha*/ = 1.f) {}
  // Draw a cropped frame from a C2 spritesheet (src in pixels).
  virtual void draw_sprite_frame(const char* /*name*/,
                                 float /*dx*/, float /*dy*/, float /*dw*/, float /*dh*/,
                                 int /*sx*/, int /*sy*/, int /*sw*/, int /*sh*/,
                                 float /*angle_deg*/ = 0.f, float /*alpha*/ = 1.f) {}
  virtual void end_frame() = 0;
  // Saves the current back buffer as BMP; call before end_frame().
  virtual bool save_screenshot(const char* /*path*/) { return false; }
  virtual bool read_file(const char* path, char** out_data, std::size_t* out_size) = 0;
  virtual bool write_file(const char* path, const void* data, std::size_t size) = 0;
  virtual int screen_w() const { return kPspWidth; }
  virtual int screen_h() const { return kPspHeight; }

  // ---- Optional PSP performance hooks (2026-10-19). Defaults are no-ops, so the desktop
  // ---- (SDL) build renders exactly as before.
  // Frame profiling: marks the START of a stage; time until the next mark is billed to it.
  enum ProfileStage { Prof_Logic = 0, Prof_Ground, Prof_World, Prof_Hud, Prof_Present, Prof_Count };
  virtual void profile_mark(int /*stage*/) {}
  // Ground-layer cache. ground_snap_px() > 0 means the platform caches the ground layer and
  // the game must snap the ground camera to multiples of that many screen px.
  // ground_cache_begin returns true when the game must issue the ground draws this frame
  // (the platform clips them to the newly exposed strips); always pair with ground_cache_end.
  virtual int ground_snap_px() const { return 0; }
  virtual bool ground_cache_begin(float /*cam_px_x*/, float /*cam_px_y*/, unsigned /*key*/) { return true; }
  virtual void ground_cache_end() {}
  virtual void ground_cache_invalidate() {}
  // World pass (ground + objects, before the HUD). With half-res enabled the platform renders
  // the world at 240x136 and upscales; the HUD after world_pass_end stays at full res.
  virtual void world_pass_begin() {}
  virtual void world_pass_end() {}
  virtual bool supports_half_res() const { return false; }
  virtual void set_half_res(bool /*on*/) {}
  virtual bool half_res() const { return false; }
  // On-screen frame-time overlay (per-stage ms), PSP only.
  virtual bool supports_perf_overlay() const { return false; }
  virtual void set_perf_overlay(bool /*on*/) {}
  virtual bool perf_overlay() const { return false; }
  // Diagnostic log line (desktop: stdout; PSP: ms0:/PSP/GAME/MINIDAYZ/log.txt).
  // Dev benchmark (PSP autoplay.txt without "tutorial"): skip the first-launch tutorial.
  virtual bool dev_skip_tutorial() const { return false; }
  virtual void log_line(const char* s) { std::printf("[mdaz] %s\n", s); std::fflush(stdout); }
};

IPlatform* create_platform();

} // namespace mdaz
