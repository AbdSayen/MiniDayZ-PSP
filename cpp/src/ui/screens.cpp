// Full-screen UI states for 480x272: main menu, pause menu, death screen, controls sheet.
// D-pad focus + PSP buttons only (2026-10-19 PSP UI rewrite). Same layout on desktop.
#include "mdaz/game.hpp"
#include "mdaz/hud.hpp"
#include "mdaz/platform.hpp"
#include "mdaz/psp_ui.hpp"

#include <cstdio>

namespace mdaz {

using namespace ui;

namespace {
void draw_toast(IPlatform& p, const char* toast) {
  if (!toast) return;
  const float w = static_cast<float>(text_w(toast)) + 16.f;
  const float x = (kScreenW - w) * 0.5f;
  panel(p, x, 120.f, w, 18.f, 0.9f);
  text_center(p, kScreenW * 0.5f, 126.f, toast, 0.95f, 0.92f, 0.7f);
}
const char* difficulty_name(Difficulty d) {
  if (d == Difficulty_Regular) return "REGULAR";
  if (d == Difficulty_Veteran) return "VETERAN";
  return "NOVICE";
}
} // namespace

void Game::render_main_menu() const {
  IPlatform& p = *platform_;
  char diff[40];
  std::snprintf(diff, sizeof(diff), "< %s >", difficulty_name(menu_difficulty_));
  const char* labels[5] = { "NEW GAME", "CONTINUE", diff, "CONTROLS", "EXIT" };
  bool disabled[5] = { false, !has_save(), false, false, false };
  ListStyle st;
  st.w = 150.f; st.row_h = 30.f; st.x = (kScreenW - st.w) * 0.5f; st.y = 84.f; st.wood = true;
  list_menu(p, st, labels, 5, menu_focus_, ui_t_, disabled);
  text(p, st.x + st.w + 8.f, st.y + 2.f * st.row_h + 10.f, "DIFFICULTY", 0.7f, 0.7f, 0.6f);

  p.draw_rect(0.f, kHintBarY - 2.f - 12.f, 150.f, 12.f, 0.f, 0.f, 0.f, 0.55f);
  if (p.has_sprite("player_skin_def_1-sheet0"))
    text(p, kSafe, kHintBarY - 12.f, "1.4.1 C++ PORT  SPRITES OK", 0.6f, 0.85f, 0.55f);
  else
    text(p, kSafe, kHintBarY - 12.f, "NO SPRITES.PAK - COPY DEPLOY", 1.f, 0.35f, 0.3f);

  if (show_controls_) {
    draw_controls_help();
  } else {
    Hint h[3] = {
      Hint{Btn_None, Btn_DPad, "ARROWS", "Select"},
      Hint{Btn_None, Btn_Cross, "ENTER", "Confirm"},
      Hint{Btn_None, Btn_DPad, "<- ->", "Difficulty"},
    };
    hint_bar(p, h, 3);
  }
  draw_toast(p, toast_text());
}

void Game::render_pause_menu() const {
  IPlatform& p = *platform_;
  p.draw_rect(0, 0, kScreenW, kScreenH, 0.f, 0.f, 0.f, 0.5f);
  if (show_controls_) {
    draw_controls_help();
    draw_toast(p, toast_text());
    return;
  }
  const float pw = 190.f, ph = 214.f;
  const float px = (kScreenW - pw) * 0.5f, py = 22.f;
  panel(p, px, py, pw, ph);
  panel_title(p, px, py, pw, "PAUSED");
  // PSP adds "HALF RES" (world at 240x136, HUD full res; saved in settings.cfg, default OFF).
  const bool hr = p.supports_half_res();
  const int n = hr ? 8 : 6;
  const char* labels[8] = { "RESUME", "SAVE GAME", "LOAD GAME", "CANCEL TASK", "CONTROLS",
                            "MAIN MENU", "MAIN MENU", "MAIN MENU" };
  if (hr) {
    labels[5] = half_res() ? "HALF RES: ON" : "HALF RES: OFF";
    labels[6] = perf_overlay_on() ? "MS OVERLAY: ON" : "MS OVERLAY: OFF";
  }
  bool disabled[8] = { false, false, !has_save(), !quest_.show_panel(), false, false, false, false };
  ListStyle st;
  st.w = 160.f; st.row_h = hr ? 23.f : 31.f; st.x = px + (pw - st.w) * 0.5f; st.y = py + 20.f; st.wood = true;
  list_menu(p, st, labels, n, pause_focus_, ui_t_, disabled);
  Hint h[4] = {
    Hint{Btn_None, Btn_DPad, "ARROWS", "Select"},
    Hint{Btn_None, Btn_Cross, "ENTER", "Confirm"},
    Hint{Btn_None, Btn_Circle, "ESC", "Resume"},
    Hint{Btn_None, Btn_Start, "ESC", "Resume"},
  };
  hint_bar(p, h, 3);
  draw_toast(p, toast_text());
}

void Game::render_dead_screen() const {
  IPlatform& p = *platform_;
  p.clear(0.04f, 0.01f, 0.01f);
  if (p.has_sprite("dead_screen-sheet0"))
    p.draw_sprite("dead_screen-sheet0", (kScreenW - 231.f) * 0.5f, 36.f, 231.f, 53.f, 0.f, 1.f);
  else
    text_center(p, kScreenW * 0.5f, 56.f, "YOU DIED", 1.f, 0.3f, 0.25f);
  if (p.has_sprite("player_skin_dead-sheet0"))
    p.draw_sprite_frame("player_skin_dead-sheet0", (kScreenW - 60.f) * 0.5f, 100.f, 60.f, 60.f,
                        1, 1, 30, 30, 0.f, 1.f);
  if (player_.survival.death_reason[0]) {
    char cause[96];
    std::snprintf(cause, sizeof(cause), "KILLED BY: %s", player_.survival.death_reason);
    text_center(p, kScreenW * 0.5f, 172.f, cause, 0.85f, 0.5f, 0.45f);
  }
  char buf[96];
  std::snprintf(buf, sizeof(buf), "DAYS %d   ZEDS %d   BANDITS %d   ANIMALS %d",
                player_.days_survived, player_.zeds_killed, player_.bandits_killed,
                player_.animals_killed);
  text_center(p, kScreenW * 0.5f, 190.f, buf, 0.8f, 0.6f, 0.55f);
  if (state_timer_ > 1.5f) {
    Hint h[1] = { Hint{Btn_None, Btn_Cross, "ENTER", "Main menu"} };
    hint_bar(p, h, 1);
  }
}

void Game::draw_controls_help() const {
  IPlatform& p = *platform_;
  const float pw = 420.f, ph = 222.f;
  const float px = (kScreenW - pw) * 0.5f, py = kSafe + 4.f;
  panel(p, px, py, pw, ph, 0.95f);
  panel_title(p, px, py, pw, "CONTROLS");
  // PSP mapping (platform_psp.cpp). Desktop keys shown when --keys is set.
  static const Hint kLeft[] = {
    {Btn_None, Btn_Analog, "WASD", "Move"},
    {Btn_None, Btn_Cross, "Z", "Attack / shoot"},
    {Btn_None, Btn_Circle, "E", "Interact / pick up"},
    {Btn_None, Btn_Triangle, "C", "Use equipped item"},
    {Btn_None, Btn_Square, "R", "Reload"},
    {Btn_None, Btn_R, "SHIFT", "Hold: run"},
    {Btn_None, Btn_L, "CTRL", "Hold: crouch"},
    {Btn_None, Btn_Select, "TAB", "Inventory"},
    {Btn_None, Btn_Start, "ESC", "Pause menu"},
  };
  static const Hint kRight[] = {
    {Btn_L, Btn_DPad, "Q / X", "Hotbar slot"},
    {Btn_R, Btn_Cross, "F", "Throw"},
    {Btn_R, Btn_Circle, "G", "Drop item"},
    {Btn_R, Btn_Triangle, "T", "Torch / campfire"},
    {Btn_R, Btn_Square, "B", "Craft menu"},
    {Btn_L, Btn_Start, "F5", "Quick save"},
    {Btn_L, Btn_Select, "F9", "Quick load"},
    {Btn_None, Btn_DPad, "ARROWS", "Menus: move focus"},
    {Btn_None, Btn_Circle, "ESC", "Menus: back"},
  };
  const int n = static_cast<int>(sizeof(kLeft) / sizeof(kLeft[0]));
  for (int i = 0; i < n; ++i) {
    hint(p, px + 10.f, py + 22.f + i * 20.f, kLeft[i]);
    hint(p, px + pw * 0.5f + 4.f, py + 22.f + i * 20.f, kRight[i]);
  }
  Hint h[1] = { Hint{Btn_None, Btn_Circle, "ESC", "Back"} };
  hint_bar(p, h, 1);
}

} // namespace mdaz
