// In-game HUD, inventory panel and craft menu for the 480x272 logical screen.
// PSP-first (2026-10-19 rewrite): no touch chrome (virtual stick / tap buttons removed),
// every action is shown as a PSP button hint; the same layout is used on desktop.
// Sprites/frames are the original C2 ones (gui_panel, gui_*_bar, gui_time_bg, minimap_*,
// gui_item_backpack, gui_car_*, sub_menu*, item on_ground frames).
#include "mdaz/hud.hpp"
#include "mdaz/weapon_unlocks.hpp"
#include "mdaz/balance_globals.hpp"
#include "mdaz/craft.hpp"
#include "mdaz/game.hpp"
#include "mdaz/inventory.hpp"
#include "mdaz/inventory_ui.hpp"
#include "mdaz/item_sprites.hpp"
#include "mdaz/map_labels.hpp"
#include "mdaz/mapgen.hpp"
#include "mdaz/platform.hpp"
#include "mdaz/psp_ui.hpp"
#include "mdaz/quest.hpp"
#include "mdaz/types.hpp"
#include "mdaz/vehicle.hpp"
#include "mdaz/weapons.hpp"
#include "mdaz/zombie.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace mdaz {

namespace {

using namespace ui;

// C2 Dictionary "Deaths" — stats screen label (Deaths_impl.json), not combat logic.
const char kC2_UI_Deaths[] = "Deaths";
const char kC2_Bleeding_guide[] = "Bleeding_guide";
const char kC2_Inventory_guide[] = "Inventory_guide";
const char kC2_ach_move_text[] = "ach_move_text";
void touch_hud_c2_audit() {
  (void)kC2_UI_Deaths;
  (void)kC2_Bleeding_guide;
  (void)kC2_Inventory_guide;
  (void)kC2_ach_move_text;
}

bool spr(IPlatform& p, const char* name) { return p.has_sprite(name); }

// Layout constants (logical px).
// gui_panel native frame 60x20 (C2 GUI_layout draws it at 2x = 120x40; PSP uses 1x).
const float kVitalX = 38.f, kVitalY = kSafe, kVitalW = 60.f, kVitalH = 20.f, kVitalGap = 2.f;
const float kHotSlot = 22.f, kHotGap = 2.f;
const float kHotY = kHintBarY - kHotSlot - 3.f;
const float kMiniW = 56.f, kMiniH = 56.f;

// C2 GUI_layout: one gui_panel instance per vital, animation frame 0..4 =
// blood(heart) / water(drop) / food(fork+knife) / heat(thermometer) / radiation.
// Each frame already carries its icon + bar track; the 1x5 bar strip sits inside the track
// at native (22,7), 5 px tall, 35 px at 100% (track interior x22..56, y7..11).
struct PanelFrame { const char* sheet; int sx, sy; };
const PanelFrame kPanelFrames[5] = {
  { "gui_panel-sheet0", 1, 1 }, { "gui_panel-sheet0", 1, 23 },
  { "gui_panel-sheet1", 1, 1 }, { "gui_panel-sheet1", 1, 23 },
  { "gui_panel-sheet2", 0, 0 },
};
const float kBarX = 22.f, kBarY = 7.f, kBarW = 35.f, kBarH = 5.f;

void draw_vital_bar(IPlatform& p, float x, float y, int frame, float fill01, const char* bar_sprite,
                    float br, float bg, float bb) {
  if (fill01 < 0.f) fill01 = 0.f;
  if (fill01 > 1.f) fill01 = 1.f;
  const PanelFrame& f = kPanelFrames[frame];
  if (spr(p, f.sheet))
    p.draw_sprite_frame(f.sheet, x, y, kVitalW, kVitalH, f.sx, f.sy, 60, 20, 0.f, 1.f);
  else
    p.draw_rect(x + kBarX - 1.f, y + kBarY - 1.f, kBarW + 2.f, kBarH + 2.f, 0.2f, 0.28f, 0.36f, 0.9f);
  const float fw = std::floor(kBarW * fill01 + 0.5f);
  if (fw < 1.f) return;
  if (spr(p, bar_sprite)) p.draw_sprite(bar_sprite, x + kBarX, y + kBarY, fw, kBarH, 0.f, 1.f);
  else p.draw_rect(x + kBarX, y + kBarY, fw, kBarH, br, bg, bb, 1.f);
}

// C2 warning overlays (gui_starving / gui_thirsty / gui_heat / gui_sick: frame 0 blank,
// frame 1 red silhouette) placed over their panel at the GUI_layout offsets (native px).
void draw_warn_icon(IPlatform& p, float px, float py, const char* sheet, float ox, float oy,
                    int sx1, int sy1, int w, int h, bool lit) {
  if (!lit || !spr(p, sheet)) return;
  p.draw_sprite_frame(sheet, px + ox, py + oy, (float)w, (float)h, sx1, sy1, w, h, 0.f, 1.f);
}

// Item icon fitted into a box, keeping the aspect of the C2 frame. Uses the on_ground frame
// for clothing/weapons (the sheet0 30x30 frames are worn/held overlays).
void draw_item_fit(IPlatform& p, ItemId id, float x, float y, float w, float h, float alpha = 1.f) {
  const ItemIcon* ic = find_item_ground_icon(id);
  if (!ic || !spr(p, ic->sprite)) {
    p.draw_rect(x + w * 0.25f, y + h * 0.25f, w * 0.5f, h * 0.5f, 0.35f, 0.35f, 0.3f, alpha);
    return;
  }
  float s = w / static_cast<float>(ic->sw);
  const float sh = h / static_cast<float>(ic->sh);
  if (sh < s) s = sh;
  if (s > 2.f) s = 2.f;
  if (s >= 1.f) s = std::floor(s); // integer scale keeps pixel art crisp
  const float dw = ic->sw * s, dh = ic->sh * s;
  p.draw_sprite_frame(ic->sprite, std::floor(x + (w - dw) * 0.5f), std::floor(y + (h - dh) * 0.5f),
                      dw, dh, ic->sx, ic->sy, ic->sw, ic->sh, 0.f, alpha);
}

void draw_slot_bg(IPlatform& p, float x, float y, float w, float h, bool hi) {
  // Dark C2-style slot (gui_inventory palette): near-black fill, brown rim.
  p.draw_rect(x, y, w, h, hi ? 0.2f : 0.11f, hi ? 0.17f : 0.1f, hi ? 0.11f : 0.08f, 0.92f);
  outline(p, x, y, w, h, 0.42f, 0.35f, 0.22f, 1.f);
}

void draw_count(IPlatform& p, float x, float y, float w, float h, int count) {
  if (count <= 1) return;
  char cb[8];
  std::snprintf(cb, sizeof(cb), "%d", count);
  text_right(p, x + w - 1.f, y + h - kCharH - 1.f, cb, 0.98f, 0.92f, 0.6f);
}

const char* item_name(const Game& game, ItemId id) {
  const WeaponDef* w = find_weapon(id);
  if (w) return w->name;
  const ItemDef* d = game.items().find(id);
  return d ? d->name.c_str() : "";
}

void draw_vertical_bar(IPlatform& p, float x, float y, float w, float h, float fill01,
                       const char* chrome, float r, float g, float b) {
  if (fill01 < 0.f) fill01 = 0.f;
  if (fill01 > 1.f) fill01 = 1.f;
  if (spr(p, chrome)) p.draw_sprite(chrome, x, y, w, h, 0.f, 0.92f);
  else p.draw_rect(x, y, w, h, 0.12f, 0.12f, 0.12f, 0.85f);
  const float ih = (h - 4.f) * fill01;
  if (ih >= 1.f) p.draw_rect(x + 2.f, y + h - 2.f - ih, w - 4.f, ih, r, g, b, 0.95f);
}

// C2 GUI_draw_vehicle: gui_car_bg + condition / fuel bars. Touch drive buttons are not drawn
// (PSP: analog steers, the hint bar shows the exit button).
void draw_vehicle_hud(IPlatform& p, const Vehicle& v) {
  static const char kC2_GUI_draw_vehicle[] = "GUI_draw_vehicle";
  (void)kC2_GUI_draw_vehicle;
  const float bw = 44.f, bh = 60.f;
  const float bx = kSafe, by = kHintBarY - bh - 4.f;
  if (spr(p, "gui_car_bg-sheet0")) p.draw_sprite("gui_car_bg-sheet0", bx, by, bw, bh, 0.f, 0.9f);
  else panel(p, bx, by, bw, bh);
  const float hp01 = v.hp / vehicle_balance::kHpMax;
  const float fuel01 = v.fuel / vehicle_balance::kFuelMax;
  draw_vertical_bar(p, bx + 8.f, by + 14.f, 8.f, 40.f, hp01, "gui_car_condition-sheet0", 0.35f, 0.75f, 0.35f);
  draw_vertical_bar(p, bx + 28.f, by + 14.f, 8.f, 40.f, fuel01, "gui_car_fuel-sheet0", 0.85f, 0.75f, 0.25f);
  if (v.fuel < 15.f && spr(p, "gui_car_nofuel_icon-sheet0"))
    p.draw_sprite("gui_car_nofuel_icon-sheet0", bx + 26.f, by + 2.f, 12.f, 12.f, 0.f, 1.f);
  if (v.hp < 25.f && spr(p, "gui_car_disable_icon-sheet0"))
    p.draw_sprite("gui_car_disable_icon-sheet0", bx + 6.f, by + 2.f, 12.f, 12.f, 0.f, 1.f);
}

void draw_minimap(IPlatform& platform, const Game& game) {
  const Player& pl = game.player();
  const float mx = kScreenW - kSafe - kMiniW, my = kHintBarY - kMiniH - 4.f;
  const float mw = kMiniW, mh = kMiniH;
  // C2 draws minimap_tile on the transparent GUI_elements layer straight over the world, at
  // 20 px per cell. Here the 16x16 map is shrunk to 56x56, so a port-side light backdrop keeps
  // the (black/alpha) original tile icons and the marks readable; 1 px dark border.
  platform.draw_rect(mx - 1.f, my - 1.f, mw + 2.f, mh + 2.f, 0.08f, 0.07f, 0.05f, 0.9f);
  platform.draw_rect(mx, my, mw, mh, 0.80f, 0.78f, 0.66f, 0.80f);
  // C2 Map generation: one minimap_tile (20x20 frames) per map cell, frame chosen from the
  // cell value (507 IV0, filled from Array 150): 0 -> frame 0 (no event), 6 -> 6, 9 -> 4,
  // 11 -> 8, 12 -> 2, 13 -> 1, 14 -> 3, 15 -> 9. The C2 neighbour-road overrides (511
  // conditions -> frames 3/10-13) are not ported. There is no visited-fog in data.js: the
  // old port darkened every unvisited sector at 82% black, which made the map look black.
  static const short kMiniFrame[29][2] = {
    {1,1},{23,1},{45,1},{67,1},{89,1},{1,23},{23,23},{45,23},{67,23},{89,23},
    {1,45},{23,45},{45,45},{67,45},{1,1},{23,23},{89,45},{1,67},{23,67},{45,67},
    {67,67},{89,67},{1,89},{23,89},{45,89},{67,89},{89,89},{1,1},{23,1}};
  const float cell_w = mw / static_cast<float>(kMapSectorsX);
  const float cell_h = mh / static_cast<float>(kMapSectorsY);
  const bool has_tile = spr(platform, "minimap_tile-sheet0");
  for (int sy = 0; sy < kMapSectorsY; ++sy) {
    for (int sx = 0; sx < kMapSectorsX; ++sx) {
      const float cx = mx + sx * cell_w, cy = my + sy * cell_h;
      int f = 0;
      switch (game.mapgen().sector_cell(sx, sy)) {
      case 6: f = 6; break;
      case 9: f = 4; break;
      case 11: f = 8; break;
      case 12: f = 2; break;
      case 13: f = 1; break;
      case 14: f = 3; break;
      case 15: f = 9; break;
      default: f = 0; break;
      }
      if (has_tile)
        platform.draw_sprite_frame("minimap_tile-sheet0", cx, cy, cell_w + 0.05f, cell_h + 0.05f,
                                   kMiniFrame[f][0], kMiniFrame[f][1], 20, 20, 0.f, 1.f);
      else
        platform.draw_rect(cx, cy, cell_w, cell_h, 0.22f, 0.26f, 0.16f, 0.92f);
    }
  }
  auto to_mini = [&](float wx, float wy, float& ox, float& oy) {
    ox = mx + (wx / kMapLayoutW) * mw;
    oy = my + (wy / kMapLayoutH) * mh;
  };
  for (int i = 0; i < game.mapgen().location_count(); ++i) {
    const MapLocation& L = game.mapgen().location(i);
    float px = 0.f, py = 0.f;
    to_mini(L.pos.x, L.pos.y, px, py);
    if (px < mx || py < my || px > mx + mw - 2.f || py > my + mh - 2.f) continue;
    if (L.type == Loc_BunkerEntrance && spr(platform, "minimap_hint-sheet0"))
      platform.draw_sprite_frame("minimap_hint-sheet0", px - 3.f, py - 4.f, 6.f, 8.f, 1, 1, 13, 16, 0.f, 1.f);
    else if (spr(platform, "minimap_mark-sheet0"))
      platform.draw_sprite("minimap_mark-sheet0", px - 2.f, py - 2.f, 4.f, 4.f, 0.f, 0.85f);
    else
      platform.draw_rect(px, py, 2.f, 2.f, 0.8f, 0.75f, 0.45f, 1.f);
  }
  if (game.quest().map_pin && game.quest().q_task_id == 2) {
    float qx = 0.f, qy = 0.f;
    const Vec2 pin = game.quest().map_pin_pos();
    to_mini(pin.x, pin.y, qx, qy);
    if (qx >= mx && qy >= my && qx <= mx + mw && qy <= my + mh) {
      if (spr(platform, "walk_marker-sheet0"))
        platform.draw_sprite("walk_marker-sheet0", qx - 4.f, qy - 5.f, 8.f, 10.f, 0.f, 0.95f);
      else
        platform.draw_rect(qx - 2.f, qy - 2.f, 4.f, 4.f, 0.95f, 0.25f, 0.2f, 1.f);
    }
  }
  float ppx = 0.f, ppy = 0.f;
  to_mini(pl.pos.x, pl.pos.y, ppx, ppy);
  if (spr(platform, "minimap_player-sheet0"))
    platform.draw_sprite("minimap_player-sheet0", ppx - 4.f, ppy - 4.f, 8.f, 8.f, 0.f, 1.f);
  else
    platform.draw_rect(ppx - 2.f, ppy - 2.f, 4.f, 4.f, 0.3f, 0.95f, 0.4f, 1.f);
}

void draw_hotbar(IPlatform& platform, const Game& game) {
  // data.js: slot cells (gui_item_backpack, IV12 = slot index) exist only for the capacity the
  // worn containers give; missing containers are covered by gui_slot_hider_*. So draw exactly
  // capacity() cells (at most 8 in this one-row bar), never empty cells beyond capacity.
  int n = game.inventory().capacity();
  if (n > 8) n = 8;
  if (n < 1) n = 1;
  const float total = n * kHotSlot + (n - 1) * kHotGap;
  const float x0 = std::floor((kScreenW - total) * 0.5f);
  for (int i = 0; i < n; ++i) {
    const float sx = x0 + i * (kHotSlot + kHotGap);
    const bool eq = (i == game.equipped_slot());
    draw_slot_bg(platform, sx, kHotY, kHotSlot, kHotSlot, eq);
    const InvSlot& sl = game.inventory().slot(i);
    if (!sl.empty()) {
      draw_item_fit(platform, sl.id, sx + 2.f, kHotY + 2.f, kHotSlot - 4.f, kHotSlot - 4.f);
      draw_count(platform, sx, kHotY, kHotSlot, kHotSlot, sl.count);
    }
    if (eq) outline(platform, sx - 1.f, kHotY - 1.f, kHotSlot + 2.f, kHotSlot + 2.f, 0.95f, 0.8f, 0.3f, 1.f);
  }
  // Equipped name (left of hotbar) + magazine (right of hotbar).
  const InvSlot& eq = game.inventory().slot(game.equipped_slot());
  if (!eq.empty())
    text_fit(platform, x0, kHotY - kLineH, 120.f, item_name(game, eq.id), 0.9f, 0.9f, 0.75f);
  const int ammo = game.ammo_for_equipped();
  const int cap = game.mag_capacity_equipped();
  if (ammo >= 0 || cap > 0) {
    char abuf[32];
    if (cap > 0) std::snprintf(abuf, sizeof(abuf), "%d/%d +%d", game.mag_rounds(), cap, ammo > 0 ? ammo : 0);
    else std::snprintf(abuf, sizeof(abuf), "%d", ammo);
    text_right(platform, x0 + total, kHotY - kLineH, abuf, 0.95f, 0.85f, 0.5f);
  }
}

void draw_quest_panel(IPlatform& platform, const Game& game) {
  if (!game.quest().show_panel()) return;
  char qbuf[160];
  const char* qline = quest_hud_line(game.quest(), game.items());
  if (game.quest().q_task_id == 1) {
    int have = 0;
    for (int i = 0; i < game.inventory().slot_count(); ++i) {
      const InvSlot& s = game.inventory().slot(i);
      if (s.id == static_cast<ItemId>(game.quest().q_target_sid)) have += s.count;
    }
    std::snprintf(qbuf, sizeof(qbuf), "%s (%d/1)", qline ? qline : "", have > 1 ? 1 : have);
  } else if (qline && qline[0]) {
    std::snprintf(qbuf, sizeof(qbuf), "%s", qline);
  } else {
    std::snprintf(qbuf, sizeof(qbuf), "TASK %d: %s", game.quest().q_task_id,
                  game.quest().label(game.items()));
  }
  const float x = kSafe, y = 46.f, w = 170.f, h = 26.f;
  if (spr(platform, "gui_quest_panel-sheet0"))
    platform.draw_sprite("gui_quest_panel-sheet0", x, y, w, h, 0.f, 0.85f);
  else
    panel(platform, x, y, w, h, 0.8f);
  if (spr(platform, "quest_icon-sheet0"))
    platform.draw_sprite("quest_icon-sheet0", x + 3.f, y + 5.f, 16.f, 16.f, 0.f, 1.f);
  text_wrap(platform, x + 22.f, y + 4.f, w - 26.f, 2, qbuf, 0.95f, 0.88f, 0.55f);
}

void draw_compass(IPlatform& platform, const Game& game, float y) {
  if (game.in_bunker()) return;
  const Player& pl = game.player();
  float best = 1e12f, bdx = 0.f, bdy = 0.f;
  LocationType best_t = Loc_BunkerEntrance;
  bool found = false;
  for (int i = 0; i < game.mapgen().location_count(); ++i) {
    const MapLocation& L = game.mapgen().location(i);
    if (L.type != Loc_BunkerEntrance && L.type != Loc_Hospital && L.type != Loc_Military &&
        L.type != Loc_GasStation && L.type != Loc_Police && L.type != Loc_Firestation &&
        L.type != Loc_Church && L.type != Loc_Secret) continue;
    const float dx = L.pos.x - pl.pos.x, dy = L.pos.y - pl.pos.y;
    const float d2 = dx * dx + dy * dy;
    if (d2 < best) { best = d2; best_t = L.type; bdx = dx; bdy = dy; found = true; }
  }
  if (!found) return;
  const char* dir = "?";
  const float ax = std::fabs(bdx), ay = std::fabs(bdy);
  if (ax < 1.f && ay < 1.f) dir = "HERE";
  else if (ay >= ax * 2.f) dir = (bdy < 0.f) ? "N" : "S";
  else if (ax >= ay * 2.f) dir = (bdx > 0.f) ? "E" : "W";
  else if (bdy < 0.f) dir = (bdx > 0.f) ? "NE" : "NW";
  else dir = (bdx > 0.f) ? "SE" : "SW";
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%s %s", location_label(best_t), dir);
  text(platform, kSafe, y, buf, 0.7f, 0.75f, 0.55f);
}

void draw_gameplay_hints(IPlatform& platform, const Game& game) {
  Hint h[8];
  int n = 0;
  const char* ia = game.interact_hint();
  if (game.driving()) {
    h[n++] = Hint{Btn_None, Btn_Analog, "WASD", "Drive"};
    h[n++] = Hint{Btn_None, Btn_Circle, "E", "Exit car"};
  } else {
    h[n++] = Hint{Btn_None, Btn_Cross, "Z", "Attack"};
    h[n++] = Hint{Btn_None, Btn_Circle, "E", ia ? ia : "Interact"};
    h[n++] = Hint{Btn_None, Btn_Triangle, "C", "Use"};
    h[n++] = Hint{Btn_None, Btn_Square, "R", "Reload"};
  }
  h[n++] = Hint{Btn_None, Btn_Select, "TAB", "Bag"};
  h[n++] = Hint{Btn_R, Btn_Square, "B", "Craft"};
  h[n++] = Hint{Btn_None, Btn_Start, "ESC", "Menu"};
  hint_bar(platform, h, n, 0.6f);
}

// ---------------------------------------------------------------- inventory panel
const float kInvX = kSafe, kInvY = kSafe, kInvW = kScreenW - 2.f * kSafe;
const float kInvH = kHintBarY - kSafe - 4.f;
const float kClothSlot = 22.f, kBagSlot = 26.f;

void draw_inventory_panel(IPlatform& platform, const Game& game) {
  const Inventory& inv = game.inventory();
  const InventoryUiState& ui = game.inventory_ui_state();
  const float t = game.ui_time();
  panel(platform, kInvX, kInvY, kInvW, kInvH, 0.94f);
  int used = 0;
  const int cap = inv.capacity();
  for (int i = 0; i < cap; ++i) if (!inv.slot(i).empty()) ++used;
  char title[48];
  std::snprintf(title, sizeof(title), "INVENTORY  %d/%d", used, cap);
  panel_title(platform, kInvX, kInvY, kInvW, title);

  // Left column: portrait + worn clothing containers (C2 gui_inv_card_* rows).
  const float lx = kInvX + 8.f, ly = kInvY + 18.f;
  if (spr(platform, "gui_portrait_bg-sheet0"))
    platform.draw_sprite_frame("gui_portrait_bg-sheet0", lx, ly, 39.f, 53.f, 83, 1, 39, 53, 0.f, 1.f);
  if (spr(platform, "player_skin_def_1-sheet0"))
    platform.draw_sprite_frame("player_skin_def_1-sheet0", lx + 4.f, ly + 12.f, 30.f, 30.f, 1, 1, 30, 30, 0.f, 1.f);
  {
    float wx = lx + 44.f;
    for (int i = 0; i < inv.worn_count() && i < 6; ++i) {
      const ItemId wid = inv.worn_at(i);
      if (wid <= 0) continue;
      draw_slot_bg(platform, wx, ly, 18.f, 18.f, false);
      draw_item_fit(platform, wid, wx + 1.f, ly + 1.f, 16.f, 16.f);
      wx += 20.f;
      if (wx > lx + 150.f) break;
    }
  }
  static const ClothingRegion kRows[] = {ClothingRegion_Pants, ClothingRegion_Outerwear,
                                         ClothingRegion_Vest, ClothingRegion_Backpack};
  static const char* kRowName[] = {"PANTS", "JACKET", "VEST", "BACKPACK"};
  float ry = ly + 58.f;
  for (int r = 0; r < 4; ++r) {
    const ClothingRegion region = kRows[r];
    const bool avail = inv.clothing().region_available(region);
    text(platform, lx, ry, kRowName[r], avail ? 0.85f : 0.4f, avail ? 0.8f : 0.4f, avail ? 0.6f : 0.38f);
    for (int s = 0; s < kClothingContainerSlots; ++s) {
      const float sx = lx + 52.f + s * (kClothSlot + 2.f);
      const float sy = ry - 6.f;
      if (!avail) { platform.draw_rect(sx, sy, kClothSlot, kClothSlot, 0.1f, 0.1f, 0.09f, 0.6f); continue; }
      const bool focus = ui.focus_cloth_region == region && ui.focus_cloth_sub == s;
      draw_slot_bg(platform, sx, sy, kClothSlot, kClothSlot, focus);
      const InvSlot& cs = inv.clothing().slot(region, s);
      if (!cs.empty()) {
        draw_item_fit(platform, cs.id, sx + 2.f, sy + 2.f, kClothSlot - 4.f, kClothSlot - 4.f);
        draw_count(platform, sx, sy, kClothSlot, kClothSlot, cs.count);
      }
      if (focus) focus_frame(platform, sx - 1.f, sy - 1.f, kClothSlot + 2.f, kClothSlot + 2.f, t);
    }
    ry += kClothSlot + 6.f;
  }

  // Right: bag grid (8 columns, first row = hotbar / hands).
  const int cols = 8;
  const float gx0 = kInvX + kInvW - 8.f - cols * kBagSlot - (cols - 1) * 2.f;
  const float gy0 = kInvY + 22.f;
  text(platform, gx0, gy0 - 2.f - kCharH, "HOTBAR", 0.6f, 0.6f, 0.5f);
  int rows = (cap + cols - 1) / cols;
  for (int i = 0; i < cap; ++i) {
    const int col = i % cols, row = i / cols;
    const float sx = gx0 + col * (kBagSlot + 2.f);
    const float sy = gy0 + row * (kBagSlot + 2.f) + (row > 0 ? 6.f : 0.f);
    const bool eq = (i == game.equipped_slot());
    const bool focus = (i == ui.focus_bag_slot);
    draw_slot_bg(platform, sx, sy, kBagSlot, kBagSlot, eq || focus);
    const InvSlot& sl = inv.slot(i);
    if (!sl.empty()) {
      draw_item_fit(platform, sl.id, sx + 2.f, sy + 2.f, kBagSlot - 4.f, kBagSlot - 4.f);
      draw_count(platform, sx, sy, kBagSlot, kBagSlot, sl.count);
    }
    if (eq) outline(platform, sx, sy, kBagSlot, kBagSlot, 0.95f, 0.8f, 0.3f, 1.f);
    if (focus) focus_frame(platform, sx - 1.f, sy - 1.f, kBagSlot + 2.f, kBagSlot + 2.f, t);
  }

  // Details of the focused item.
  const float dy = gy0 + rows * (kBagSlot + 2.f) + 10.f;
  const float dx = gx0, dw = cols * (kBagSlot + 2.f) - 2.f;
  ItemId fid = 0;
  int fcount = 0;
  if (ui.focus_cloth_region != ClothingRegion_Count && ui.focus_cloth_sub >= 0) {
    const InvSlot& cs = inv.clothing().slot(ui.focus_cloth_region, ui.focus_cloth_sub);
    fid = cs.id; fcount = cs.count;
  } else if (ui.focus_bag_slot >= 0 && ui.focus_bag_slot < cap) {
    const InvSlot& sl = inv.slot(ui.focus_bag_slot);
    fid = sl.id; fcount = sl.count;
  }
  if (dy + 24.f <= kInvY + kInvH) {
    platform.draw_rect(dx, dy, dw, 24.f, 0.f, 0.f, 0.f, 0.35f);
    if (fid > 0) {
      draw_item_fit(platform, fid, dx + 2.f, dy + 2.f, 20.f, 20.f);
      char line[80];
      if (fcount > 1) std::snprintf(line, sizeof(line), "%s x%d", item_name(game, fid), fcount);
      else std::snprintf(line, sizeof(line), "%s", item_name(game, fid));
      text_fit(platform, dx + 26.f, dy + 3.f, dw - 30.f, line, 0.95f, 0.92f, 0.75f);
      const bool hand = ui.focus_bag_slot >= 0 && ui.focus_bag_slot < 8 &&
                        ui.focus_cloth_region == ClothingRegion_Count;
      text_fit(platform, dx + 26.f, dy + 13.f, dw - 30.f,
               hand ? "IN HOTBAR" : (ui.focus_cloth_region != ClothingRegion_Count ? "IN CLOTHING" : "IN BAG"),
               0.6f, 0.65f, 0.5f);
    } else {
      text(platform, dx + 6.f, dy + 8.f, "EMPTY SLOT", 0.5f, 0.5f, 0.45f);
    }
  }
  if (game.inv_full_flash() > 0.f)
    text_center(platform, kScreenW * 0.5f, kInvY + kInvH - 12.f, "NO SPACE", 1.f, 0.55f, 0.2f);

  const bool in_cloth = ui.focus_cloth_region != ClothingRegion_Count;
  Hint h[6];
  int n = 0;
  h[n++] = Hint{Btn_None, Btn_DPad, "ARROWS", "Move"};
  if (in_cloth) {
    h[n++] = Hint{Btn_None, Btn_Cross, "ENTER", "Take"};
  } else {
    h[n++] = Hint{Btn_None, Btn_Cross, "ENTER", "Use/Wear"};
    h[n++] = Hint{Btn_None, Btn_Triangle, "C", "To hand"};
    h[n++] = Hint{Btn_None, Btn_Square, "R", "Drop"};
  }
  h[n++] = Hint{Btn_None, Btn_Circle, "ESC", "Close"};
  hint_bar(platform, h, n);
}

// ---------------------------------------------------------------- craft menu
// C2 Call_sub_menu_craft + Check_craft: sub_menu chrome, recipe rows, GUI_craft_possible tint.
void draw_craft_menu(IPlatform& p, const Game& game) {
  static const char kC2_Call_sub_menu_craft[] = "Call_sub_menu_craft";
  static const char kC2_Check_craft[] = "Check_craft";
  (void)kC2_Call_sub_menu_craft;
  (void)kC2_Check_craft;
  const float pw = 300.f, ph = 206.f;
  const float px = std::floor((kScreenW - pw) * 0.5f), py = kSafe + 4.f;
  panel(p, px, py, pw, ph, 0.94f);
  panel_title(p, px, py, pw, game.near_campfire() ? "CRAFT  (AT FIRE)" : "CRAFT");
  const bool at_fire = game.near_campfire();
  const int n = craft_recipe_count();
  const int sel = game.craft_menu_index();
  const int max_rows = 11;
  const float row_h = 14.f;
  int start = 0;
  if (n > max_rows) {
    start = sel - max_rows / 2;
    if (start < 0) start = 0;
    if (start > n - max_rows) start = n - max_rows;
  }
  const float ly = py + 18.f;
  for (int i = start, row = 0; i < n && row < max_rows; ++i, ++row) {
    const CraftRecipe* r = craft_recipe_at(i);
    if (!r) continue;
    const float ry = ly + row * row_h;
    const bool focus = (i == sel);
    p.draw_rect(px + 6.f, ry, pw - 12.f, row_h - 1.f, focus ? 0.3f : 0.11f, focus ? 0.24f : 0.1f,
                focus ? 0.15f : 0.08f, 0.95f);
    const bool possible = craft_can(game.inventory(), *r) && (!r->requires_fire || at_fire);
    draw_item_fit(p, r->result_id, px + 8.f, ry, 12.f, 12.f);
    char line[72];
    std::snprintf(line, sizeof(line), "%s x%d", item_name(game, r->result_id), r->result_count);
    text_fit(p, px + 24.f, ry + 3.f, pw - 90.f, line,
             possible ? 0.55f : 0.62f, possible ? 0.95f : 0.6f, possible ? 0.45f : 0.55f);
    if (r->requires_fire) text_right(p, px + pw - 10.f, ry + 3.f, "FIRE", 0.95f, 0.6f, 0.25f);
    if (focus) focus_frame(p, px + 5.f, ry - 1.f, pw - 10.f, row_h + 1.f, game.ui_time());
  }
  if (n > max_rows) {
    char sb[16];
    std::snprintf(sb, sizeof(sb), "%d/%d", sel + 1, n);
    text_right(p, px + pw - 8.f, py + 4.f, sb, 0.7f, 0.7f, 0.6f);
  }
  // Ingredients of the focused recipe.
  const CraftRecipe* r = craft_recipe_at(sel);
  if (r) {
    const float iy = ly + max_rows * row_h + 4.f;
    char ing[120];
    int len = std::snprintf(ing, sizeof(ing), "NEEDS: %s x%d", item_name(game, r->ing_a), r->count_a);
    if (r->ing_b > 0 && len > 0 && len < static_cast<int>(sizeof(ing)))
      len += std::snprintf(ing + len, sizeof(ing) - len, " + %s x%d", item_name(game, r->ing_b), r->count_b);
    if (r->tool_any_knife && len > 0 && len < static_cast<int>(sizeof(ing)))
      std::snprintf(ing + len, sizeof(ing) - len, "  (KNIFE)");
    else if (r->tool_id > 0 && len > 0 && len < static_cast<int>(sizeof(ing)))
      std::snprintf(ing + len, sizeof(ing) - len, "  (%s)", item_name(game, r->tool_id));
    text_wrap(p, px + 8.f, iy, pw - 16.f, 2, ing, 0.85f, 0.82f, 0.65f);
  }
  Hint h[4] = {
    Hint{Btn_None, Btn_DPad, "ARROWS", "Select"},
    Hint{Btn_None, Btn_Cross, "ENTER", "Craft"},
    Hint{Btn_None, Btn_Circle, "ESC", "Close"},
  };
  hint_bar(p, h, 3);
}

void draw_weapon_unlock_toast_impl(IPlatform& platform, const Game& game) {
  static const char kC2_Show_unlock_wpn[] = "Show_unlock_wpn";
  (void)kC2_Show_unlock_wpn;
  if (game.weapon_unlock_toast_t() <= 0.f || game.weapon_unlock_toast_weapon() <= 0) return;
  const char* title = weapon_unlock_toast_title_c2();
  const char* body = weapon_unlock_toast_body_c2(game.weapon_unlock_toast_weapon());
  if (!title || !title[0]) return;
  const float bw = 200.f, bh = 30.f;
  const float bx = (kScreenW - bw) * 0.5f, by = 80.f;
  const float a = game.weapon_unlock_toast_t() > 0.25f ? 1.f : game.weapon_unlock_toast_t() / 0.25f;
  const char* banner = weapon_unlock_toast_banner_sprite();
  if (banner && banner[0] && spr(platform, banner))
    platform.draw_sprite(banner, bx, by, bw, bh, 0.f, a);
  else
    platform.draw_rect(bx, by, bw, bh, 0.08f, 0.1f, 0.12f, 0.88f * a);
  text_center(platform, kScreenW * 0.5f, by + 5.f, title, 0.95f * a, 0.88f * a, 0.35f * a);
  if (body && body[0]) text_center(platform, kScreenW * 0.5f, by + 17.f, body, 0.85f * a, 0.9f * a, 0.75f * a);
}

} // namespace

void draw_weapon_unlock_toast(IPlatform& platform, const Game& game) {
  draw_weapon_unlock_toast_impl(platform, game);
}

void draw_debug_hud(IPlatform& platform, const Game& game) {
  touch_hud_c2_audit();
  const Player& pl = game.player();
  char buf[96];

  // Full-screen panels replace the HUD (nothing bleeds through them).
  if (game.show_bag() || game.show_craft_menu()) {
    platform.draw_rect(0, 0, kScreenW, kScreenH, 0.f, 0.f, 0.f, 0.45f);
    if (game.show_bag()) draw_inventory_panel(platform, game);
    else draw_craft_menu(platform, game);
    if (const char* toast = game.toast_text()) {
      const float w = static_cast<float>(text_w(toast)) + 16.f;
      panel(platform, std::floor((kScreenW - w) * 0.5f), 120.f, w, 18.f, 0.9f);
      text_center(platform, kScreenW * 0.5f, 126.f, toast, 0.95f, 0.92f, 0.7f);
    }
    return;
  }

  // Portrait (gui_portrait_bg frame 39x53 @ 83,1) at 2/3 scale, top-left.
  if (spr(platform, "gui_portrait_bg-sheet0"))
    platform.draw_sprite_frame("gui_portrait_bg-sheet0", kSafe, kSafe, 26.f, 35.f, 83, 1, 39, 53, 0.f, 1.f);
  // C2: the portrait is gui_btn_perks (29x32, anim per skin, hotspot -4,-3) pinned at the same
  // origin as the card (gui_portrait_bg, hotspot +1,0), so it sits inside the card's dark window
  // at card-local (5,3). Card stays below (gui_portrait_bg a63 = move to bottom).
  {
    const float s = 26.f / 39.f;
    const float px = kSafe + 5.f * s, py = kSafe + 3.f * s;
    if (spr(platform, "gui_btn_perks-sheet0"))
      platform.draw_sprite_frame("gui_btn_perks-sheet0", px, py, 29.f * s, 32.f * s, 1, 1, 29, 32, 0.f, 1.f);
    else if (spr(platform, "player_skin_def_1-sheet0"))
      platform.draw_sprite_frame("player_skin_def_1-sheet0", px, py, 29.f * s, 29.f * s, 1, 1, 30, 30, 0.f, 1.f);
  }

  // Vitals: blood, water, food, heat (C2 GUI_layout order; colours = the C2 1x5 bar strips).
  const float vx = kVitalX;
  const float step = kVitalW + kVitalGap;
  draw_vital_bar(platform, vx, kVitalY, 0, pl.survival.blood / 100.f, "gui_blood_bar-sheet0", 0.85f, 0.2f, 0.21f);
  draw_vital_bar(platform, vx + step, kVitalY, 1, pl.survival.water / 100.f, "gui_water_bar-sheet0", 0.29f, 0.67f, 0.69f);
  draw_vital_bar(platform, vx + 2.f * step, kVitalY, 2, pl.survival.food / 100.f, "gui_food_bar-sheet0", 0.31f, 0.58f, 0.27f);
  draw_vital_bar(platform, vx + 3.f * step, kVitalY, 3, pl.survival.temperature / 100.f, "gui_heat_bar-sheet0", 0.6f, 0.54f, 0.02f);
  {
    // Blink of the red warning silhouettes (C2 toggles frame 0/1 from events; ~2 Hz here).
    const bool blink = (static_cast<int>(game.ui_time() * 4.f) & 1) != 0;
    // Offsets from GUI_layout (instance top-left minus panel top-left, /2).
    if (pl.survival.bleeding > 0.f && spr(platform, "gui_bleeding-sheet0"))
      platform.draw_sprite("gui_bleeding-sheet0", vx + 35.f, kVitalY + 4.f, 7.f, 11.f, 0.f, 1.f);
    draw_warn_icon(platform, vx, kVitalY, "gui_sick-sheet0", 43.f, 2.f, 16, 1, 13, 15,
                   pl.survival.infected && blink);
    draw_warn_icon(platform, vx + step, kVitalY, "gui_thirsty-sheet0", 8.f, 2.f, 14, 1, 11, 17,
                   pl.survival.water < 20.f && blink);
    draw_warn_icon(platform, vx + 2.f * step, kVitalY, "gui_starving-sheet0", 6.f, 3.f, 17, 1, 14, 18,
                   pl.survival.food < 20.f && blink);
    draw_warn_icon(platform, vx + 3.f * step, kVitalY, "gui_heat-sheet0", 8.f, 2.f, 14, 1, 11, 18,
                   pl.survival.temperature < 25.f && blink);
    float iy = kVitalY + kVitalH + 2.f;
    // Wetness / gas mask / torch battery mini-bars next to the icons.
    float bx = vx + 2.f * step;
    if (game.wetness() > 8.f) {
      platform.draw_rect(bx, iy, kVitalW, 3.f, 0.08f, 0.1f, 0.15f, 0.8f);
      platform.draw_rect(bx, iy, kVitalW * (game.wetness() / 100.f), 3.f, 0.25f, 0.45f, 0.85f, 0.95f);
      iy += 5.f;
    }
    if (game.wearing_gasmask()) {
      platform.draw_rect(bx, iy, kVitalW, 3.f, 0.08f, 0.1f, 0.08f, 0.8f);
      platform.draw_rect(bx, iy, kVitalW * (game.gasmask_filter() / 100.f), 3.f, 0.35f, 0.7f, 0.35f, 0.95f);
      iy += 5.f;
    }
    if (pl.torch_on || pl.nvg_on || game.torch_battery() < 99.f) {
      platform.draw_rect(bx, iy, kVitalW, 3.f, 0.1f, 0.1f, 0.1f, 0.85f);
      platform.draw_rect(bx, iy, kVitalW * (game.torch_battery() / 100.f), 3.f,
                         pl.nvg_on ? 0.3f : 0.9f, pl.nvg_on ? 0.85f : 0.75f, pl.nvg_on ? 0.35f : 0.2f, 1.f);
    }
    if (game.adrenaline_t() > 0.f) text(platform, vx + 3.f * step, kVitalY + kVitalH + 3.f, "ADRENALINE", 0.5f, 0.9f, 0.55f);
  }

  // Day badge (gui_time_bg) top-right, FPS under it.
  {
    const float tw = 64.f, th = 18.f;
    const float tx = kScreenW - kSafe - tw;
    if (spr(platform, "gui_time_bg-sheet0")) platform.draw_sprite("gui_time_bg-sheet0", tx, kSafe, tw, th, 0.f, 0.95f);
    else panel(platform, tx, kSafe, tw, th);
    std::snprintf(buf, sizeof(buf), "DAY %d", pl.days_survived);
    text_center(platform, tx + tw * 0.5f, kSafe + 6.f, buf, 0.95f, 0.9f, 0.75f);
    std::snprintf(buf, sizeof(buf), "FPS %d", static_cast<int>(game.fps() + 0.5f));
    text_right(platform, kScreenW - kSafe, kSafe + th + 3.f, buf, 0.55f, 0.95f, 0.55f);
  }

  draw_quest_panel(platform, game);
  // Location compass text ("POLICE E") removed: data.js has no location-name strings or
  // text objects; the original only shows locations on the minimap.
  (void)&draw_compass;

  if (game.airdrop_alert() > 0.f)
    text_center(platform, kScreenW * 0.5f, 40.f, "AIRDROP INBOUND", 0.95f, 0.75f, 0.25f);
  if (game.radio_t() > 0.f) {
    const float eta_s = game.airdrop_eta();
    const float hours = eta_s / static_cast<float>(balance::Day_Night_speed) *
                        static_cast<float>(balance::Airdrop_Timer_Hours);
    if (eta_s <= 0.f) std::snprintf(buf, sizeof(buf), "RADIO: AIRDROP SOON");
    else std::snprintf(buf, sizeof(buf), "RADIO: DROP ~%.1fh", hours > 0.05f ? hours : eta_s / 60.f);
    text(platform, kSafe, kHotY - 2.f * kLineH, buf, 0.55f, 0.85f, 0.95f);
  }

  // Centre indicators (reload spinner / jam).
  if (game.reload_cd() > 0.f) {
    const float ang = game.reload_cd() * 360.f;
    if (spr(platform, "reloading_spinner-sheet0"))
      platform.draw_sprite_frame("reloading_spinner-sheet0", 228.f, 100.f, 24.f, 24.f, 1, 1, 47, 47, ang, 0.95f);
    else
      text_center(platform, kScreenW * 0.5f, 104.f, "RELOADING", 0.85f, 0.85f, 0.55f);
  } else if (game.gun_jammed()) {
    text_center(platform, kScreenW * 0.5f, 104.f, "JAMMED", 1.f, 0.45f, 0.25f);
  } else if (game.mag_capacity_equipped() > 0 && game.mag_rounds() <= 0 && game.ammo_for_equipped() > 0) {
    if (spr(platform, "reloading_icon-sheet0"))
      platform.draw_sprite("reloading_icon-sheet0", 234.f, 98.f, 12.f, 18.f, 0.f, 0.8f);
    else
      text_center(platform, kScreenW * 0.5f, 104.f, "RELOAD", 0.9f, 0.8f, 0.35f);
  }
  if (game.inv_full_flash() > 0.f)
    text_center(platform, kScreenW * 0.5f, 150.f, "INVENTORY FULL", 1.f, 0.55f, 0.2f);

  if (game.driving()) {
    const Vehicle* v = game.driven_vehicle();
    if (v) draw_vehicle_hud(platform, *v);
  }

  draw_minimap(platform, game);
  draw_weapon_unlock_toast_impl(platform, game);

  draw_hotbar(platform, game);
  if (game.state() == AppState_Playing) draw_gameplay_hints(platform, game);

  // Toast (save/load/craft feedback).
  if (const char* toast = game.toast_text()) {
    const float w = static_cast<float>(text_w(toast)) + 16.f;
    const float x = std::floor((kScreenW - w) * 0.5f);
    panel(platform, x, 120.f, w, 18.f, 0.9f);
    text_center(platform, kScreenW * 0.5f, 126.f, toast, 0.95f, 0.92f, 0.7f);
  }
}

} // namespace mdaz
