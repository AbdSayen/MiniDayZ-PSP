// Item 10: C2 XP (expierence_points) + perkmenu (data.js Game_events / perkmenu groups).
// PSP adaptation (KNOWN_ISSUES 10): C2 opens the perk menu by tapping gui_btn_perks (the HUD
// portrait); on PSP it is the "PERKS" entry of the pause menu. D-pad moves the selection over
// the 2x8 icon grid (= tapping an icon), Cross = tapping gui_perk_icon_buy, Circle/Start close.
#include "mdaz/game.hpp"
#include "mdaz/balance_globals.hpp"
#include "mdaz/platform.hpp"
#include "mdaz/psp_ui.hpp"
#include "mdaz/ui_strings.hpp"

#include <cstdio>
#include <cstring>

namespace mdaz {

using namespace ui;

namespace {

// Bullet t910 IV4 name class of a player weapon (bullet type spawned per player IV1 / IV43,
// default IV4 of that bullet type in the layout). 1: name listed in both the zed (+8) and the
// tank (+20) kill events; 2: zed only (madsen / bizon / vss); 3: "shotgun" (zed IV5 = 1 ->
// +8 in the Zeds_common ForEach, the tank shotgun event is empty); 0: not listed (groza).
int c2_bullet_xp_class(ItemId w) {
  switch (w) {
  case 156: case 158: case 160: case 159: case 161: case 157: case 162: case 163: case 164:
  case 166: case 167: case 168: case 169: case 173: case 174: case 175: case 177: case 186:
  case 151: case 152: case 153: case 170: case 165: case 172: case 176: case 179: case 180:
  case 181: case 182:
    return 1;
  case 183: case 185: case 187:
    return 2;
  case 154: case 155: case 178: case 171:
    return 3;
  default:
    return 0;
  }
}

} // namespace

const PerkState& Game::perks() const { return player_.perks; }

void Game::xp_add(int n) {
  // Every XP event except item_activate 62 / Function "Expierence" requires LayoutName = "Map".
  if (tut_.active) return;
  player_.perks.xp_raw += n;
}

void Game::xp_on_zed_killed(const Zombie& z) {
  if (xp_src_ == 0) return;   // traps, fire, explosions, car: no t910 player bullet
  if (z.type == Zed_Screamer) return;   // obj710: per-hit +6 only
  if (xp_src_ == 2) { xp_add(z.type == Zed_Tank ? 20 : 8); return; }   // t910 IV5 (melee)
  const int c = c2_bullet_xp_class(xp_src_wpn_);
  if (z.type == Zed_Tank) { if (c == 1) xp_add(20); return; }
  if (c != 0) xp_add(8);
}

void Game::tick_xp(float dt) {
  // GROUP EXpierence: Every 10 s, player IV15 > 0, LayoutName = "Map":
  // CurrentLevel 0/1/2/3 -> +1/+2/+3/+3.
  if (!tut_.active && !player_.survival.is_dead()) {
    xp_every_t_ += dt;
    while (xp_every_t_ >= 10.f) {
      xp_every_t_ -= 10.f;
      const int lvl = static_cast<int>(balance::CurrentLevel);
      player_.perks.xp_raw += lvl <= 0 ? 1 : lvl == 1 ? 2 : 3;
    }
  }
  // MD_Perk_2: expierence_points >= perk_cost, >= 100, Map, TriggerOnce.
  PerkState& ps = player_.perks;
  const bool c = ps.xp_raw >= ps.cost && ps.xp_raw >= 100 && !tut_.active;
  if (c && !ps.perk2_once) sfx2d("MD_Perk_2", 0.f);
  ps.perk2_once = c;
}

void Game::open_perk_menu() {
  // perkmenu open: icons spawned, select marker on frame 13, buy frame 1 when XP >= 100 and
  // frame 13 not bought (sid808068840237150), description 413 + "Score to unlock".
  show_perks_ = true;
  perk_sel_ = 13;
  perk_buy_lit_ = player_.perks.xp_raw >= 100 && !player_.perks.has(13);
}

void Game::perk_select(int f) {
  // OnTapGestureObject gui_perk_icon: marker + IV0; buy frame only changes when XP >= cost.
  perk_sel_ = f;
  if (player_.perks.xp_raw >= player_.perks.cost) perk_buy_lit_ = !player_.perks.has(f);
}

void Game::update_perk_menu(const InputState& in) {
  if (in.back_pressed || in.pause_pressed) { show_perks_ = false; return; }   // close_perks_menu
  int idx = 0;
  for (int i = 0; i < Perk_Count; ++i) if (kPerkMenuSlots[i].frame == perk_sel_) idx = i;
  int row = idx / 8, col = idx % 8;
  const int r0 = row, c0 = col;
  if (in.nav_left) col = (col + 7) % 8;
  if (in.nav_right) col = (col + 1) % 8;
  if (in.nav_up || in.nav_down) row = 1 - row;
  if (row != r0 || col != c0) perk_select(kPerkMenuSlots[row * 8 + col].frame);
  if (in.confirm_pressed || in.attack_pressed) {
    if (player_.perks.can_buy(perk_sel_)) {
      sfx2d("MD_Perk_1", 0.f);
      perk_buy_lit_ = false;
      player_.perks.buy(perk_sel_);
    }
  }
}

void Game::render_perk_menu() const {
  IPlatform& p = *platform_;
  const PerkState& ps = player_.perks;
  p.draw_rect(0, 0, kScreenW, kScreenH, 0.f, 0.f, 0.f, 0.6f);
  const float px = kSafe, py = kSafe, pw = kScreenW - 2.f * kSafe, ph = kHintBarY - kSafe - 4.f;
  panel(p, px, py, pw, ph, 0.95f);
  panel_title(p, px, py, pw, "PERKS");
  // C2 HotspotX/HotspotY grid scaled 0.5 (C2 GUI px -> 480x272), hotspot at screen centre.
  const float sc = 0.5f, hx = kScreenW * 0.5f, hy = 132.f;
  const float isz = 90.f * sc;
  for (int i = 0; i < Perk_Count; ++i) {
    const PerkSlotPos& s = kPerkMenuSlots[i];
    // CreateObject at HotspotX + dx (the -48 is folded into dx), icon hotspot 0.5.
    const float cx = hx + s.dx * sc, cy = hy + s.dy * sc;
    int sx, sy;
    perk_icon_src(s.frame, sx, sy);
    // Not bought: opacity 50 + AdjustHSL saturation 0; bought: opacity 100, saturation 100.
    const bool own = ps.has(s.frame);
    if (p.has_sprite("gui_perk_icon-sheet0"))
      p.draw_sprite_frame("gui_perk_icon-sheet0", cx - isz * 0.5f, cy - isz * 0.5f, isz, isz,
                          sx, sy, 90, 90, 0.f, own ? 1.f : 0.5f);
    else
      p.draw_rect(cx - isz * 0.5f, cy - isz * 0.5f, isz, isz, 0.4f, 0.4f, 0.4f, own ? 1.f : 0.5f);
    if (s.frame == perk_sel_) {
      const float m = 95.f * sc;   // gui_perk_icon_select 95x95, hotspot 0.495
      if (p.has_sprite("gui_perk_icon_select-sheet0"))
        p.draw_sprite("gui_perk_icon_select-sheet0", cx - m * 0.495f, cy - m * 0.495f, m, m, 0.f, 1.f);
      else
        outline(p, cx - m * 0.5f, cy - m * 0.5f, m, m, 1.f, 1.f, 1.f, 1.f);
    }
  }
  // gui_perk_icon_buy (95x119, hotspot 0.505 / 0.403) at HotspotX + 330, HotspotY + 110.
  {
    const float bw = 95.f * sc, bh = 119.f * sc;
    const float bx = hx + 330.f * sc - bw * 0.505f, by = hy + 110.f * sc - bh * 0.403f;
    const char* sheet = perk_buy_lit_ ? "gui_perk_icon_buy-sheet1" : "gui_perk_icon_buy-sheet0";
    if (p.has_sprite(sheet)) p.draw_sprite(sheet, bx, by, bw, bh, 0.f, 1.f);
    else p.draw_rect(bx, by, bw, bh, perk_buy_lit_ ? 0.3f : 0.2f, perk_buy_lit_ ? 0.6f : 0.2f, 0.2f, 1.f);
  }
  // t597: description (l_eng_ui 400 + frame) + newline + "Score to unlock: " + (perk_cost - 16).
  {
    const char* d = ui_string(400 + perk_sel_);
    char name[64] = {0};
    const char* nl = d ? std::strchr(d, '\n') : 0;
    if (d) {
      const std::size_t n = nl ? static_cast<std::size_t>(nl - d) : std::strlen(d);
      std::memcpy(name, d, n < sizeof(name) - 1 ? n : sizeof(name) - 1);
    }
    const float tx = px + 12.f, ty = hy + 36.f;
    text(p, tx, ty, name, 1.f, 0.92f, 0.6f);
    if (nl) text_wrap(p, tx, ty + 12.f, 330.f, 3, nl + 1, 0.9f, 0.9f, 0.9f);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s %d", ui_string(536) ? ui_string(536) : "", ps.shown_cost());
    text(p, tx, ty + 50.f, buf, 0.9f, 0.8f, 0.5f);
    // t543 at panel imagepoint "exp": ui 337 "XP " + (expierence_points - 16), white.
    std::snprintf(buf, sizeof(buf), "%s%d", ui_string(337) ? ui_string(337) : "", ps.shown_xp());
    text_right(p, px + pw - 10.f, py + 4.f, buf, 1.f, 1.f, 1.f);
  }
  Hint h[3] = {
    Hint{Btn_None, Btn_DPad, "ARROWS", "Select"},
    Hint{Btn_None, Btn_Cross, "ENTER", "Learn"},
    Hint{Btn_None, Btn_Circle, "ESC", "Back"},
  };
  // cover the gameplay hint strip drawn by the HUD underneath
  p.draw_rect(0.f, kHintBarY - 2.f, kScreenW, kScreenH - kHintBarY + 2.f, 0.f, 0.f, 0.f, 1.f);
  hint_bar(p, h, 3);
}

void Game::test_hurt_zombie_by_player(Zombie& z, float dmg, ItemId weapon, bool melee) {
  xp_src_ = melee ? 2 : 1;
  xp_src_wpn_ = weapon;
  if (melee) hurt_zombie(z, dmg, weapon);
  else hurt_zombie_bullet(z, dmg, false, weapon);
  xp_src_ = 0;
}

void Game::harness_perks(int step) {
  if (step == -1) {         // XP 600 raw (shown 584) -> notify visible
    player_.perks.xp_raw = 600;
  } else if (step == 0) {   // pause menu "PERKS"
    state_ = AppState_Playing;
    open_perk_menu();
  } else if (step == 1) {   // select Camel (frame 3) and learn it
    perk_select(3);
    if (player_.perks.can_buy(3)) { sfx2d("MD_Perk_1", 0.f); perk_buy_lit_ = false; player_.perks.buy(3); }
  }
  char line[160];
  std::snprintf(line, sizeof(line), "harness_perks step=%d xp_shown=%d cost_shown=%d learnt=%d camel=%d sel=%d lit=%d",
                step, player_.perks.shown_xp(), player_.perks.shown_cost(), player_.perks.learnt,
                (int)player_.perks.has(3), perk_sel_, (int)perk_buy_lit_);
  if (platform_) platform_->log_line(line);
}

} // namespace mdaz
