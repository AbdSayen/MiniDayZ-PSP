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
#include "mdaz/quest.hpp"
#include "mdaz/types.hpp"
#include "mdaz/vehicle.hpp"
#include "mdaz/weapons.hpp"
#include "mdaz/zombie.hpp"

#include <cmath>
#include <cstdio>

namespace mdaz {

namespace {

// C2 Dictionary "Deaths" — stats screen label (Deaths_impl.json), not combat logic.
const char kC2_UI_Deaths[] = "Deaths";
// Tutorial overlay groups (Bleeding_guide / Inventory_guide) — desktop skips full tutor UI.
const char kC2_Bleeding_guide[] = "Bleeding_guide";
const char kC2_Inventory_guide[] = "Inventory_guide";
// Achievements menu: movement tutorial complete copy (Call ach_move_text param 13).
const char kC2_ach_move_text[] = "ach_move_text";
void touch_hud_c2_audit() {
  (void)kC2_UI_Deaths;
  (void)kC2_Bleeding_guide;
  (void)kC2_Inventory_guide;
  (void)kC2_ach_move_text;
}

bool spr(IPlatform& p, const char* name) { return p.has_sprite(name); }

// Scale GUI_layout (1024×768) → PSP logical (480×272).
float gx(float x) { return x * (480.f / 1024.f); }
float gy(float y) { return y * (272.f / 768.f); }
float gw(float w) { return w * (480.f / 1024.f); }
float gh(float h) { return h * (272.f / 768.f); }

// Original: gui_panel frame 60×20 @ (1,1) in 64×64 sheet; bars are 1×5 strips.
void draw_vital_bar(IPlatform& p, float panel_x, float panel_y,
                    float fill01, const char* bar_sprite,
                    float br, float bg, float bb) {
  if (fill01 < 0.f) fill01 = 0.f;
  if (fill01 > 1.f) fill01 = 1.f;
  const float pw = gw(120.f), ph = gh(40.f);
  if (spr(p, "gui_panel-sheet0"))
    p.draw_sprite_frame("gui_panel-sheet0", panel_x, panel_y, pw, ph,
                        1, 1, 60, 20, 0.f, 0.92f);
  else
    p.draw_rect(panel_x, panel_y, pw, ph, 0.08f, 0.08f, 0.08f, 0.85f);
  const float bx = panel_x + gw(8.f);
  const float by = panel_y + gh(12.f);
  const float bw = pw - gw(16.f);
  const float bh = gh(14.f);
  p.draw_rect(bx, by, bw, bh, 0.1f, 0.1f, 0.1f, 0.9f);
  const float fw = bw * fill01;
  if (fw < 0.5f) return;
  if (spr(p, bar_sprite))
    p.draw_sprite(bar_sprite, bx, by, fw, bh, 0.f, 1.f);
  else
    p.draw_rect(bx, by, fw, bh, br, bg, bb, 1.f);
}

void draw_status_icon(IPlatform& p, float& x, float y, const char* name,
                      float nw, float nh) {
  if (!spr(p, name)) return;
  const float dw = gw(nw * 1.6f), dh = gh(nh * 1.6f);
  p.draw_sprite(name, x, y, dw, dh, 0.f, 1.f);
  x += dw + 2.f;
}

static void draw_inv_item_icon(IPlatform& p, ItemId id, float x, float y, float w, float h) {
  const ItemIcon* ic = find_item_icon(id);
  if (ic && spr(p, ic->sprite))
    p.draw_sprite_frame(ic->sprite, x, y, w, h, ic->sx, ic->sy, ic->sw, ic->sh, 0.f, 1.f);
  else
    p.draw_rect(x + w * 0.2f, y + h * 0.2f, w * 0.6f, h * 0.6f, 0.35f, 0.35f, 0.3f, 1.f);
}

static void draw_inventory_bag_panel(IPlatform& platform, const Game& game) {
  const Inventory& inv = game.inventory();
  const InventoryPanelLayout L = inventory_panel_layout();
  const InventoryUiState& ui = game.inventory_ui_state();

  if (spr(platform, "gui_inventory-sheet0"))
    platform.draw_sprite_frame("gui_inventory-sheet0", L.panel_x, L.panel_y, L.panel_w, L.panel_h,
                               0, 0, 362, 283, 0.f, 0.96f);
  else {
    platform.draw_rect(L.panel_x, L.panel_y, L.panel_w, L.panel_h, 0.55f, 0.5f, 0.38f, 0.92f);
    platform.draw_rect(L.panel_x + 1.f, L.panel_y + 1.f, L.panel_w - 2.f, L.panel_h - 2.f,
                       0.06f, 0.06f, 0.05f, 0.95f);
  }

  if (spr(platform, "gui_portrait_bg-sheet0"))
    platform.draw_sprite("gui_portrait_bg-sheet0", L.portrait_x, L.portrait_y, L.portrait_w,
                         L.portrait_h, 0.f, 0.95f);

  // Worn clothing icons on portrait (C2 gui_portrait_helmet / item icons — not bag list).
  {
    float wy = L.portrait_y + gh(8.f);
    for (int i = 0; i < inv.worn_count(); ++i) {
      const ItemId wid = inv.worn_at(i);
      if (wid <= 0) continue;
      const float iw = gw(22.f), ih = gh(22.f);
      draw_inv_item_icon(platform, wid, L.portrait_x + gw(6.f), wy, iw, ih);
      wy += ih + 1.f;
    }
  }

  static const ClothingRegion kRows[] = {ClothingRegion_Pants, ClothingRegion_Outerwear,
                                         ClothingRegion_Vest, ClothingRegion_Backpack};
  static const char* kCardSpr[] = {"gui_inv_card_vest-sheet0", "gui_inv_card_vest-sheet0",
                                   "gui_inv_card_vest-sheet0", "gui_inv_card_backpack-sheet0"};
  for (int r = 0; r < 4; ++r) {
    const ClothingRegion region = kRows[r];
    if (!inv.clothing().region_available(region)) continue;
    const float row_y = L.cloth_origin_y + r * L.cloth_row_step;
    if (spr(platform, kCardSpr[r]))
      platform.draw_sprite_frame(kCardSpr[r], L.cloth_origin_x - gw(4.f), row_y - gh(4.f),
                                 L.cloth_slot_w * 4.f + gw(12.f), L.cloth_slot_h + gh(8.f),
                                 (r == 3) ? 1 : 1, (r == 3) ? 1 : 65, (r == 3) ? 216 : 91,
                                 (r == 3) ? 62 : 62, 0.f, 0.55f);
    for (int s = 0; s < kClothingContainerSlots; ++s) {
      const float sx = L.cloth_origin_x + s * (L.cloth_slot_w + gw(4.f));
      const bool focus = ui.focus_cloth_region == region && ui.focus_cloth_sub == s;
      if (spr(platform, "gui_item_backpack-sheet0"))
        platform.draw_sprite_frame("gui_item_backpack-sheet0", sx, row_y, L.cloth_slot_w,
                                   L.cloth_slot_h, 1, 1, 30, 30, 0.f, focus ? 1.f : 0.82f);
      else if (spr(platform, "gui_slot_bg-sheet0"))
        platform.draw_sprite("gui_slot_bg-sheet0", sx, row_y, L.cloth_slot_w, L.cloth_slot_h, 0.f,
                             focus ? 1.f : 0.75f);
      if (focus)
        platform.draw_rect(sx, row_y, L.cloth_slot_w, 2.f, 0.95f, 0.85f, 0.35f, 1.f);
      const InvSlot& cs = inv.clothing().slot(region, s);
      if (!cs.empty()) {
        draw_inv_item_icon(platform, cs.id, sx + gw(4.f), row_y + gh(4.f), L.cloth_slot_w - gw(8.f),
                           L.cloth_slot_h - gh(8.f));
        if (cs.count > 1) {
          char cb[8];
          std::snprintf(cb, sizeof(cb), "%d", cs.count);
          platform.draw_text(sx + gw(2.f), row_y + L.cloth_slot_h - gh(10.f), cb, 0.95f, 0.9f,
                             0.55f);
        }
      }
    }
  }

  const int cap = inv.capacity();
  const int cols = L.bag_cols;
  for (int i = 0; i < cap; ++i) {
    const int col = i % cols;
    const int row = i / cols;
    const float sx = L.bag_grid_x + col * (L.bag_slot_w + gw(3.f));
    const float sy = L.bag_grid_y + row * (L.bag_slot_h + gh(3.f));
    const bool eq = (i == game.equipped_slot());
    const bool focus = (i == ui.focus_bag_slot);
    if (spr(platform, "gui_item_backpack-sheet0"))
      platform.draw_sprite_frame("gui_item_backpack-sheet0", sx, sy, L.bag_slot_w, L.bag_slot_h,
                                 1, 1, 30, 30, 0.f, eq || focus ? 1.f : 0.78f);
    else if (spr(platform, "gui_slot_bg-sheet0"))
      platform.draw_sprite("gui_slot_bg-sheet0", sx, sy, L.bag_slot_w, L.bag_slot_h, 0.f,
                           eq || focus ? 1.f : 0.7f);
    if (eq)
      platform.draw_rect(sx, sy, L.bag_slot_w, 2.f, 0.9f, 0.75f, 0.3f, 1.f);
    else if (focus)
      platform.draw_rect(sx, sy, L.bag_slot_w, 2.f, 0.85f, 0.85f, 0.5f, 1.f);
    const InvSlot& sl = inv.slot(i);
    if (!sl.empty()) {
      draw_inv_item_icon(platform, sl.id, sx + gw(4.f), sy + gh(4.f), L.bag_slot_w - gw(8.f),
                         L.bag_slot_h - gh(8.f));
      if (sl.count > 1) {
        char cb[8];
        std::snprintf(cb, sizeof(cb), "%d", sl.count);
        platform.draw_text(sx + gw(2.f), sy + L.bag_slot_h - gh(10.f), cb, 0.95f, 0.9f, 0.55f);
      }
    }
  }
}

void draw_vertical_vehicle_bar(IPlatform& p, float x, float y, float w, float h,
                               float fill01, const char* chrome_sprite,
                               float r, float g, float b) {
  if (fill01 < 0.f) fill01 = 0.f;
  if (fill01 > 1.f) fill01 = 1.f;
  if (spr(p, chrome_sprite))
    p.draw_sprite(chrome_sprite, x, y, w, h, 0.f, 0.92f);
  else
    p.draw_rect(x, y, w, h, 0.12f, 0.12f, 0.12f, 0.85f);
  const float pad = gw(2.f);
  const float ih = (h - pad * 2.f) * fill01;
  if (ih >= 1.f)
    p.draw_rect(x + pad, y + h - pad - ih, w - pad * 2.f, ih, r, g, b, 0.95f);
}

// C2 Vehicles UI: GUI_draw_vehicle while driving; GUI_erase_vehicle when on foot.
void gui_erase_vehicle_hud() {
  static const char kC2_GUI_erase_vehicle[] = "GUI_erase_vehicle";
  (void)kC2_GUI_erase_vehicle;
}

// C2 GUI_draw_vehicle: gui_car_bg, bar_fuel (IV284), bar_hp (IV294), car controls.
void draw_vehicle_hud(IPlatform& p, const Vehicle& v) {
  static const char kC2_GUI_draw_vehicle[] = "GUI_draw_vehicle";
  static const char kC2_Vehicles_UI[] = "Vehicles UI";
  (void)kC2_GUI_draw_vehicle;
  (void)kC2_Vehicles_UI;
  const float bg_x = gx(8.f);
  const float bg_y = gy(168.f);
  if (spr(p, "gui_car_bg-sheet0"))
    p.draw_sprite("gui_car_bg-sheet0", bg_x, bg_y, gw(78.f), gh(106.f), 0.f, 0.9f);

  const float bar_w = gw(10.f);
  const float bar_h = gh(70.f);
  const float bar_y = bg_y + gh(76.f);
  const float fuel_x = bg_x + gw(52.f);
  const float hp_x = bg_x + gw(16.f);
  const float fuel01 = v.fuel / vehicle_balance::kFuelMax;
  const float hp01 = v.hp / vehicle_balance::kHpMax;

  draw_vertical_vehicle_bar(p, hp_x, bar_y, bar_w, bar_h, hp01, "gui_car_condition-sheet0",
                            0.35f, 0.75f, 0.35f);
  draw_vertical_vehicle_bar(p, fuel_x, bar_y, bar_w, bar_h, fuel01, "gui_car_fuel-sheet0",
                            0.85f, 0.75f, 0.25f);

  if (v.fuel < 15.f && spr(p, "gui_car_nofuel_icon-sheet0"))
    p.draw_sprite("gui_car_nofuel_icon-sheet0", fuel_x + gw(12.f), bar_y - gh(8.f),
                   gw(18.f), gh(18.f), 0.f, 1.f);
  if (v.hp < 25.f && spr(p, "gui_car_disable_icon-sheet0"))
    p.draw_sprite("gui_car_disable_icon-sheet0", hp_x - gw(4.f), bar_y - gh(8.f),
                   gw(18.f), gh(18.f), 0.f, 1.f);

  if (spr(p, "gui_btn_car_leave-sheet0"))
    p.draw_sprite("gui_btn_car_leave-sheet0", gx(8.f), gy(228.f), gw(40.f), gh(28.f), 0.f, 0.85f);
  if (spr(p, "gui_btn_car_forward-sheet0"))
    p.draw_sprite("gui_btn_car_forward-sheet0", gx(48.f), gy(200.f), gw(36.f), gh(28.f), 0.f, 0.75f);
  if (spr(p, "gui_btn_car_backward-sheet0"))
    p.draw_sprite("gui_btn_car_backward-sheet0", gx(48.f), gy(232.f), gw(36.f), gh(28.f), 0.f, 0.75f);
  if (spr(p, "gui_btn_car_wheel_left-sheet0"))
    p.draw_sprite("gui_btn_car_wheel_left-sheet0", gx(92.f), gy(200.f), gw(32.f), gh(26.f), 0.f, 0.7f);
  if (spr(p, "gui_btn_car_wheel_right-sheet0"))
    p.draw_sprite("gui_btn_car_wheel_right-sheet0", gx(128.f), gy(200.f), gw(32.f), gh(26.f), 0.f, 0.7f);
}

// C2 Call_sub_menu_craft + Check_craft: sub_menu chrome, recipe rows, GUI_craft_possible tint.
void draw_craft_submenu(IPlatform& p, const Game& game) {
  static const char kC2_Call_sub_menu_craft[] = "Call_sub_menu_craft";
  static const char kC2_Check_craft[] = "Check_craft";
  (void)kC2_Call_sub_menu_craft;
  (void)kC2_Check_craft;

  const float px = 8.f;
  const float py = 118.f;
  const float pw = 200.f;
  const float title_h = 18.f;
  const float row_h = 16.f;
  if (spr(p, "sub_menu_title-sheet0"))
    p.draw_sprite("sub_menu_title-sheet0", px, py, pw, title_h, 0.f, 0.95f);
  else
    p.draw_rect(px, py, pw, title_h, 0.35f, 0.32f, 0.28f, 0.92f);
  p.draw_text(px + gw(8.f), py + gh(4.f), "CRAFT", 0.95f, 0.9f, 0.75f);

  const bool at_fire = game.near_campfire();
  const int n = craft_recipe_count();
  const int sel = game.craft_menu_index();
  const int max_rows = 8;
  int start = 0;
  if (n > max_rows) {
    start = sel - max_rows / 2;
    if (start < 0) start = 0;
    if (start > n - max_rows) start = n - max_rows;
  }
  int row = 0;
  for (int i = start; i < n && row < max_rows; ++i) {
    const CraftRecipe* r = craft_recipe_at(i);
    if (!r) continue;
    const float ry = py + title_h + row * row_h;
    if (spr(p, "sub_menu-sheet0"))
      p.draw_sprite("sub_menu-sheet0", px, ry, pw, row_h, 0.f, i == sel ? 1.f : 0.88f);
    else
      p.draw_rect(px, ry, pw, row_h, 0.12f, 0.11f, 0.1f, i == sel ? 0.95f : 0.82f);

    const bool possible = craft_can(game.inventory(), *r) &&
                          (!r->requires_fire || at_fire);
    const ItemDef* res = game.items().find(r->result_id);
    char line[72];
    if (res)
      std::snprintf(line, sizeof(line), "%s x%d", res->name.c_str(), r->result_count);
    else
      std::snprintf(line, sizeof(line), "#%d x%d", r->result_id, r->result_count);
    const float tr = possible ? 0.55f : 0.45f;
    const float tg = possible ? 0.95f : 0.55f;
    const float tb = possible ? 0.45f : 0.45f;
    p.draw_text(px + gw(6.f), ry + gh(6.f), line, tr, tg, tb);
    ++row;
  }
  if (craft_any_possible(game.inventory(), at_fire))
#if defined(MDAZ_PSP)
    p.draw_text(px, py + title_h + max_rows * row_h + gh(4.f), "R+SQ: craft", 0.75f, 0.85f, 0.55f);
#else
    p.draw_text(px, py + title_h + max_rows * row_h + gh(4.f), "R: craft", 0.75f, 0.85f, 0.55f);
#endif
}

void draw_weapon_unlock_toast_impl(IPlatform& platform, const Game& game) {
  static const char kC2_Show_unlock_wpn[] = "Show_unlock_wpn";
  (void)kC2_Show_unlock_wpn;
  if (game.weapon_unlock_toast_t() <= 0.f || game.weapon_unlock_toast_weapon() <= 0) return;
  const char* title = weapon_unlock_toast_title_c2();
  const char* body = weapon_unlock_toast_body_c2(game.weapon_unlock_toast_weapon());
  if (!title || !title[0]) return;

  const float banner_w = gw(220.f);
  const float banner_h = gh(52.f);
  const float bx = gx(512.f) - banner_w * 0.5f;
  const float by = gy(200.f);
  const float alpha =
      game.weapon_unlock_toast_t() > 0.25f ? 1.f : game.weapon_unlock_toast_t() / 0.25f;

  const char* banner = weapon_unlock_toast_banner_sprite();
  if (banner && banner[0] && spr(platform, banner))
    platform.draw_sprite(banner, bx, by, banner_w, banner_h, 0.f, alpha);
  else
    platform.draw_rect(bx, by, banner_w, banner_h, 0.08f, 0.1f, 0.12f, 0.88f * alpha);

  platform.draw_text(bx + gw(10.f), by + gh(8.f), title, 0.95f * alpha, 0.88f * alpha,
                     0.35f * alpha);
  if (body && body[0])
    platform.draw_text(bx + gw(10.f), by + gh(26.f), body, 0.85f * alpha, 0.9f * alpha,
                       0.75f * alpha);
}

} // namespace

void draw_weapon_unlock_toast(IPlatform& platform, const Game& game) {
  draw_weapon_unlock_toast_impl(platform, game);
}

void draw_debug_hud(IPlatform& platform, const Game& game) {
  touch_hud_c2_audit();
  const Player& pl = game.player();
  char buf[160];

  // Portrait — frame 39×53 @ (83,1) in 128 sheet; layout instance 78×106
  if (spr(platform, "gui_portrait_bg-sheet0"))
    platform.draw_sprite_frame("gui_portrait_bg-sheet0", 0.f, 0.f, gw(78.f), gh(106.f),
                               83, 1, 39, 53, 0.f, 1.f);
  if (spr(platform, "player_skin_def_1-sheet0")) {
    platform.draw_sprite_frame("player_skin_def_1-sheet0",
                               gw(24.f), gh(28.f), gw(30.f), gh(30.f),
                               1, 1, 30, 30, 0.f, 1.f);
  }

  // Vital bars — layout positions from data.js GUI_layout
  // blood panel (321,29), water (455,29), food (601,30), heat (760,30)
  draw_vital_bar(platform, gx(321.f), gy(29.f), pl.survival.blood / 100.f,
                 "gui_blood_bar-sheet0", 0.75f, 0.15f, 0.12f);
  draw_vital_bar(platform, gx(455.f), gy(29.f), pl.survival.water / 100.f,
                 "gui_water_bar-sheet0", 0.2f, 0.45f, 0.85f);
  draw_vital_bar(platform, gx(601.f), gy(30.f), pl.survival.food / 100.f,
                 "gui_food_bar-sheet0", 0.75f, 0.55f, 0.15f);
  draw_vital_bar(platform, gx(760.f), gy(30.f), pl.survival.temperature / 100.f,
                 "gui_heat_bar-sheet0", 0.85f, 0.35f, 0.1f);

  // Status icons near blood cluster
  {
    float ix = gx(283.f), iy = gy(70.f);
    if (pl.survival.bleeding > 0.f)
      draw_status_icon(platform, ix, iy, "gui_bleeding-sheet0", 7.f, 11.f);
    if (pl.survival.infected)
      draw_status_icon(platform, ix, iy, "gui_sick-sheet0", 13.f, 15.f);
    if (pl.survival.food < 20.f)
      draw_status_icon(platform, ix, iy, "gui_starving-sheet0", 14.f, 18.f);
    if (pl.survival.water < 20.f)
      draw_status_icon(platform, ix, iy, "gui_thirsty-sheet0", 11.f, 17.f);
    if (pl.survival.temperature < 25.f)
      draw_status_icon(platform, ix, iy, "gui_heat-sheet0", 11.f, 18.f);
  }

  // Wetness / gasmask filter (compact bars under status)
  if (game.wetness() > 8.f) {
    const float wx = gx(8.f), wy = gh(118.f), ww = gw(70.f);
    platform.draw_rect(wx, wy, ww, 3.f, 0.08f, 0.1f, 0.15f, 0.8f);
    platform.draw_rect(wx, wy, ww * (game.wetness() / 100.f), 3.f, 0.25f, 0.45f, 0.85f, 0.95f);
  }
  if (game.wearing_gasmask()) {
    const float gx0 = gx(8.f), gy0 = gh(124.f), gw0 = gw(70.f);
    platform.draw_rect(gx0, gy0, gw0, 3.f, 0.08f, 0.1f, 0.08f, 0.8f);
    platform.draw_rect(gx0, gy0, gw0 * (game.gasmask_filter() / 100.f), 3.f,
                       0.35f, 0.7f, 0.35f, 0.95f);
  }
  if (game.adrenaline_t() > 0.f)
    platform.draw_text(gx(50.f), gh(132.f), "ADR", 0.5f, 0.9f, 0.55f);

  // Day counter — gui_time_bg right-anchored in original; pin top-right of bars
  if (spr(platform, "gui_time_bg-sheet0")) {
    const float tw = gw(118.f), th = gh(48.f);
    platform.draw_sprite("gui_time_bg-sheet0", 480.f - tw - 4.f, gy(8.f), tw, th, 0.f, 0.95f);
    std::snprintf(buf, sizeof(buf), "DAY %d", pl.days_survived);
    platform.draw_text(480.f - tw + 10.f, gy(22.f), buf, 0.95f, 0.9f, 0.75f);
  } else {
    std::snprintf(buf, sizeof(buf), "DAY %d", pl.days_survived);
    platform.draw_text(400.f, 8.f, buf, 0.8f, 0.8f, 0.7f);
  }

  // FPS (top-left, under portrait / above world) — desktop + PSP.
  {
    const int fps_i = static_cast<int>(game.fps() + 0.5f);
    std::snprintf(buf, sizeof(buf), "FPS %d", fps_i);
    platform.draw_text(4.f, 4.f, buf, 0.55f, 0.95f, 0.55f);
  }

  // Torch / NVG battery strip under portrait
  if (pl.torch_on || pl.nvg_on || game.torch_battery() < 99.f) {
    const float bw = gw(70.f), bh = 4.f;
    const float bx = gw(4.f), by = gh(110.f);
    platform.draw_rect(bx, by, bw, bh, 0.1f, 0.1f, 0.1f, 0.85f);
    const float fill = game.torch_battery() / 100.f;
    platform.draw_rect(bx, by, bw * fill, bh,
                       pl.nvg_on ? 0.3f : 0.9f,
                       pl.nvg_on ? 0.85f : 0.75f,
                       pl.nvg_on ? 0.35f : 0.2f, 1.f);
  }

  if (game.inv_full_flash() > 0.f)
    platform.draw_text(200, 140, "INV FULL", 1.f, 0.55f, 0.2f);
  if (game.reload_cd() > 0.f) {
    // Original reloading spinner / icon near centre
    const float ang = game.reload_cd() * 360.f;
    if (spr(platform, "reloading_spinner-sheet0"))
      platform.draw_sprite_frame("reloading_spinner-sheet0",
                                 216.f, 110.f, 48.f, 48.f, 1, 1, 47, 47, ang, 0.95f);
    else if (spr(platform, "reloading_icon-sheet0"))
      platform.draw_sprite("reloading_icon-sheet0", 228.f, 118.f, 25.f, 36.f, 0.f, 0.95f);
    else
      platform.draw_text(210, 120, "RELOADING", 0.85f, 0.85f, 0.55f);
  } else if (game.gun_jammed()) {
    platform.draw_text(210, 120, "JAMMED", 1.f, 0.45f, 0.25f);
  } else if (game.mag_capacity_equipped() > 0 && game.mag_rounds() <= 0 &&
             game.ammo_for_equipped() > 0) {
    if (spr(platform, "reloading_icon-sheet0"))
      platform.draw_sprite("reloading_icon-sheet0", 228.f, 118.f, 25.f, 36.f, 0.f, 0.7f);
    else
      platform.draw_text(210, 120, "RELOAD", 0.9f, 0.8f, 0.35f);
  }

  if (game.driving()) {
    const Vehicle* v = game.driven_vehicle();
    if (v) draw_vehicle_hud(platform, *v);
  } else {
    gui_erase_vehicle_hud();
  }

  if (game.quest().show_panel()) {
    if (spr(platform, "gui_quest_panel-sheet0"))
      platform.draw_sprite("gui_quest_panel-sheet0", gx(4.f), gy(100.f),
                           gw(220.f), gh(36.f), 0.f, 0.85f);
    if (spr(platform, "quest_icon-sheet0"))
      platform.draw_sprite("quest_icon-sheet0", gx(8.f), gy(105.f),
                           gw(22.f), gh(22.f), 0.f, 1.f);
    char qbuf[96];
    if (game.quest().q_task_id == 1) {
      int have = 0;
      for (int i = 0; i < game.inventory().slot_count(); ++i) {
        const InvSlot& s = game.inventory().slot(i);
        if (s.id == static_cast<ItemId>(game.quest().q_target_sid)) have += s.count;
      }
      const char* qline = quest_hud_line(game.quest(), game.items());
      std::snprintf(qbuf, sizeof(qbuf), "%s (%d/1)", qline, have > 1 ? 1 : have);
    } else {
      const char* qline = quest_hud_line(game.quest(), game.items());
      if (qline && qline[0])
        std::snprintf(qbuf, sizeof(qbuf), "%.120s", qline);
      else
        std::snprintf(qbuf, sizeof(qbuf), "TASK %d: %s", game.quest().q_task_id,
                      game.quest().label(game.items()));
    }
    platform.draw_text(gx(36.f), gy(112.f), qbuf, 0.9f, 0.85f, 0.5f);
  }

  // Radio airdrop ETA (Airdrop_Timer_Hours = 12 shown as game-hours estimate)
  if (game.radio_t() > 0.f) {
    const float eta_s = game.airdrop_eta();
    const float hours = eta_s / static_cast<float>(balance::Day_Night_speed) *
                        static_cast<float>(balance::Airdrop_Timer_Hours);
    char rbuf[64];
    if (eta_s <= 0.f)
      std::snprintf(rbuf, sizeof(rbuf), "RADIO: AIRDROP SOON");
    else
      std::snprintf(rbuf, sizeof(rbuf), "RADIO: DROP ~%.1fh (%.0fs)",
                    hours > 0.05f ? hours : eta_s / 60.f, eta_s);
    platform.draw_text(140.f, 248.f, rbuf, 0.55f, 0.85f, 0.95f);
  }
  if (game.airdrop_alert() > 0.f)
    platform.draw_text(160.f, 8.f, "AIRDROP INBOUND", 0.95f, 0.75f, 0.25f);

  // Compact compass
  if (!game.in_bunker()) {
    float best = 1e12f;
    LocationType best_t = Loc_BunkerEntrance;
    bool found = false;
    for (int i = 0; i < game.mapgen().location_count(); ++i) {
      const MapLocation& L = game.mapgen().location(i);
      if (L.type != Loc_BunkerEntrance && L.type != Loc_Hospital &&
          L.type != Loc_Military && L.type != Loc_GasStation &&
          L.type != Loc_Police && L.type != Loc_Firestation &&
          L.type != Loc_Church && L.type != Loc_Secret) continue;
      const float dx = L.pos.x - pl.pos.x;
      const float dy = L.pos.y - pl.pos.y;
      const float d2 = dx * dx + dy * dy;
      if (d2 < best) { best = d2; best_t = L.type; found = true; }
    }
    if (found) {
      float bdx = 0.f, bdy = 0.f;
      for (int i = 0; i < game.mapgen().location_count(); ++i) {
        const MapLocation& L = game.mapgen().location(i);
        if (L.type != best_t) continue;
        const float dx = L.pos.x - pl.pos.x;
        const float dy = L.pos.y - pl.pos.y;
        if (dx * dx + dy * dy <= best + 0.1f) { bdx = dx; bdy = dy; break; }
      }
      const char* dir = "?";
      const float ax = std::fabs(bdx), ay = std::fabs(bdy);
      if (ax < 1.f && ay < 1.f) dir = "HERE";
      else if (ay >= ax * 2.f) dir = (bdy < 0.f) ? "N" : "S";
      else if (ax >= ay * 2.f) dir = (bdx > 0.f) ? "E" : "W";
      else if (bdy < 0.f) dir = (bdx > 0.f) ? "NE" : "NW";
      else dir = (bdx > 0.f) ? "SE" : "SW";
      std::snprintf(buf, sizeof(buf), "%s %s", location_label(best_t), dir);
      platform.draw_text(gx(36.f), gy(150.f), buf, 0.65f, 0.7f, 0.5f);
    }
  }

  // Context tip
  {
    const ItemId eqid = game.inventory().slot(game.equipped_slot()).id;
    const char* tip = 0;
    if (game.near_campfire()) tip = "R COOK";
    else if (game.near_choppable_tree() &&
             (eqid == 202 || eqid == 204 || eqid == 206)) tip = "E CHOP";
    else if (eqid == 201) tip = "E DIG";
    else if (game.near_vehicle() && !game.driving()) tip = "E MOUNT / G STASH";
    else if (game.crouching()) tip = "CROUCH";
    if (tip) platform.draw_text(gx(200.f), gy(700.f), tip, 0.65f, 0.75f, 0.55f);
  }

  // Mag readout
  {
    const int ammo = game.ammo_for_equipped();
    const int cap = game.mag_capacity_equipped();
    if (ammo >= 0 || cap > 0) {
      char abuf[32];
      if (cap > 0)
        std::snprintf(abuf, sizeof(abuf), "%d/%d", game.mag_rounds(), cap);
      else
        std::snprintf(abuf, sizeof(abuf), "%d", ammo);
      platform.draw_text(340, 252, abuf, 0.85f, 0.85f, 0.55f);
    }
  }

  // Hotbar — right of inventory button; original item icon crops
  for (int i = 0; i < 8; ++i) {
    const float sx = gx(110.f) + i * gw(38.f);
    const float sy = 272.f - gh(44.f);
    const float slot_w = gw(36.f), slot_h = gh(36.f);
    const bool eq = (i == game.equipped_slot());
    if (spr(platform, "gui_slot_bg-sheet0"))
      platform.draw_sprite("gui_slot_bg-sheet0", sx, sy, slot_w, slot_h, 0.f, eq ? 1.f : 0.7f);
    else
      platform.draw_rect(sx, sy, slot_w, slot_h, eq ? 0.3f : 0.12f, eq ? 0.3f : 0.12f, 0.15f, 0.9f);
    if (eq)
      platform.draw_rect(sx, sy, slot_w, 2.f, 0.9f, 0.75f, 0.3f, 1.f);
    const InvSlot& sl = game.inventory().slot(i);
    if (!sl.empty()) {
      const ItemIcon* ic = find_item_icon(sl.id);
      bool drew = false;
      if (ic && spr(platform, ic->sprite)) {
        const float pad = 3.f;
        platform.draw_sprite_frame(ic->sprite, sx + pad, sy + pad,
                                   slot_w - pad * 2.f, slot_h - pad * 2.f,
                                   ic->sx, ic->sy, ic->sw, ic->sh, 0.f, 1.f);
        drew = true;
      }
      if (!drew) {
        const ItemDef* d = game.items().find(sl.id);
        if (d && !d->name.empty()) {
          char lb[8];
          lb[0] = static_cast<char>(d->name[0]);
          lb[1] = 0;
          platform.draw_text(sx + gw(12.f), sy + gh(10.f), lb, 0.95f, 0.95f, 0.85f);
        }
      }
      if (sl.count > 1) {
        std::snprintf(buf, sizeof(buf), "%d", sl.count);
        platform.draw_text(sx + slot_w - 10.f, sy + slot_h - 10.f, buf, 0.95f, 0.9f, 0.7f);
      }
    }
  }

  // Equipped name
  {
    const InvSlot& eq = game.inventory().slot(game.equipped_slot());
    if (!eq.empty()) {
      const WeaponDef* w = find_weapon(eq.id);
      const ItemDef* idef = game.items().find(eq.id);
      if (w) platform.draw_text(gx(340.f), gy(700.f), w->name, 0.85f, 0.9f, 0.7f);
      else if (idef) platform.draw_text(gx(340.f), gy(700.f), idef->name.c_str(), 0.85f, 0.9f, 0.7f);
    }
  }

  // Touch chrome (scaled from GUI_layout) — inventory / attack / interact / reload / dpad
  {
    const float inv_s = gw(50.f);
    if (spr(platform, "gui_btn_inventory-sheet0"))
      platform.draw_sprite("gui_btn_inventory-sheet0", 2.f, 272.f - inv_s - 2.f,
                           inv_s, inv_s, 0.f, 0.9f);

    const float btn_w = gw(50.f), btn_h = gh(40.f);
    const float rx = 480.f - btn_w - 4.f;
    if (spr(platform, "gui_btn_attack-sheet0"))
      platform.draw_sprite("gui_btn_attack-sheet0", rx, gy(461.f), btn_w, btn_h, 0.f, 0.88f);
    if (spr(platform, "gui_btn_interact-sheet0"))
      platform.draw_sprite_frame("gui_btn_interact-sheet0", rx, gy(374.f), btn_w, gw(50.f),
                                 1, 1, 50, 50, 0.f, 0.88f);
    if (spr(platform, "gui_btn_reload-sheet0"))
      platform.draw_sprite("gui_btn_reload-sheet0", rx, gy(540.f), btn_w, btn_w, 0.f, 0.85f);

    // Compact dpad (original 250×250 — shrink for 480×272)
    const float dpad = 56.f;
    const float dx = 4.f, dy = 272.f - inv_s - dpad - 6.f;
    if (spr(platform, "gui_dpad_field-sheet0"))
      platform.draw_sprite("gui_dpad_field-sheet0", dx, dy, dpad, dpad, 0.f, 0.55f);
    if (spr(platform, "gui_dpad_stick-sheet0")) {
      const float stick = 28.f;
      platform.draw_sprite("gui_dpad_stick-sheet0",
                           dx + (dpad - stick) * 0.5f, dy + (dpad - stick) * 0.5f,
                           stick, stick, 0.f, 0.7f);
    }
  }

  if (game.show_craft_menu()) draw_craft_submenu(platform, game);

  if (game.show_bag()) draw_inventory_bag_panel(platform, game);

  // Minimap — bottom-right; C2 minimap_tile + sector fog + POI marks (generate_minimap stand-in)
  {
    const float mw = 64.f, mh = 64.f;
    const float mx = 480.f - mw - 6.f, my = 272.f - mh - gh(50.f) - 4.f;
    platform.draw_rect(mx - 1.f, my - 1.f, mw + 2.f, mh + 2.f, 0.08f, 0.07f, 0.05f, 0.95f);
    // Tile 20×20 @ (1,1) from GUI_layout (layer 53 minimap_tile).
    const float tile = 20.f;
    for (float ty = my; ty < my + mh - 0.5f; ty += tile) {
      for (float tx = mx; tx < mx + mw - 0.5f; tx += tile) {
        const float tw = (tx + tile > mx + mw) ? (mx + mw - tx) : tile;
        const float th = (ty + tile > my + mh) ? (my + mh - ty) : tile;
        if (spr(platform, "minimap_tile-sheet0"))
          platform.draw_sprite_frame("minimap_tile-sheet0", tx, ty, tw, th,
                                     1, 1, 20, 20, 0.f, 0.88f);
        else
          platform.draw_rect(tx, ty, tw, th, 0.22f, 0.26f, 0.16f, 0.92f);
      }
    }
    const float cell_w = mw / static_cast<float>(kMapSectorsX);
    const float cell_h = mh / static_cast<float>(kMapSectorsY);
    for (int sy = 0; sy < kMapSectorsY; ++sy) {
      for (int sx = 0; sx < kMapSectorsX; ++sx) {
        const float cx = mx + sx * cell_w;
        const float cy = my + sy * cell_h;
        if (!game.sector_visited(sx, sy)) {
          platform.draw_rect(cx, cy, cell_w, cell_h, 0.04f, 0.04f, 0.05f, 0.82f);
          continue;
        }
        if (game.mapgen().is_road_sector(sx, sy))
          platform.draw_rect(cx, cy, cell_w - 0.5f, cell_h - 0.5f,
                             0.42f, 0.38f, 0.28f, 0.65f);
        else
          platform.draw_rect(cx, cy, cell_w - 0.5f, cell_h - 0.5f,
                             0.32f, 0.38f, 0.22f, 0.45f);
      }
    }
    // Fixed world→minimap: full map fits in the square (not camera-relative).
    auto to_mini = [&](float wx, float wy, float& ox, float& oy) {
      ox = mx + (wx / kMapLayoutW) * mw;
      oy = my + (wy / kMapLayoutH) * mh;
    };
    for (int i = 0; i < game.mapgen().location_count(); ++i) {
      const MapLocation& L = game.mapgen().location(i);
      if (!game.sector_visited(L.sector_x, L.sector_y) && L.type != Loc_BunkerEntrance) continue;
      float px = 0.f, py = 0.f;
      to_mini(L.pos.x, L.pos.y, px, py);
      if (px < mx || py < my || px > mx + mw - 2.f || py > my + mh - 2.f) continue;
      float r = 0.7f, g = 0.65f, b = 0.4f;
      if (L.type == Loc_BunkerEntrance) { r = 0.95f; g = 0.65f; b = 0.2f; }
      else if (L.type == Loc_Military) { r = 0.25f; g = 0.75f; b = 0.3f; }
      else if (L.type == Loc_Hospital) { r = 0.9f; g = 0.35f; b = 0.35f; }
      else if (L.type == Loc_GasStation) { r = 0.9f; g = 0.8f; b = 0.2f; }
      else if (L.type == Loc_Police) { r = 0.35f; g = 0.45f; b = 0.95f; }
      else if (L.type == Loc_City) { r = 0.85f; g = 0.85f; b = 0.75f; }
      else if (L.type == Loc_Helicrash) { r = 0.9f; g = 0.4f; b = 0.15f; }
      if (L.type == Loc_BunkerEntrance && spr(platform, "minimap_hint-sheet0"))
        platform.draw_sprite_frame("minimap_hint-sheet0", px - 3.f, py - 4.f, 6.f, 8.f,
                                   1, 1, 13, 16, 0.f, 1.f);
      else if (spr(platform, "minimap_mark-sheet0"))
        platform.draw_sprite("minimap_mark-sheet0", px - 2.f, py - 2.f, 4.f, 4.f, 0.f, 0.85f);
      else
        platform.draw_rect(px, py, 2.5f, 2.5f, r, g, b, 1.f);
    }
    if (game.quest().map_pin && game.quest().q_task_id == 2) {
      float qx = 0.f, qy = 0.f;
      const Vec2 pin = game.quest().map_pin_pos();
      to_mini(pin.x, pin.y, qx, qy);
      if (qx >= mx && qy >= my && qx <= mx + mw && qy <= my + mh) {
        if (spr(platform, "minimap_mark-sheet0"))
          platform.draw_sprite("minimap_mark-sheet0", qx - 4.f, qy - 4.f, 8.f, 8.f, 0.f, 1.f);
        else if (spr(platform, "walk_marker-sheet0"))
          platform.draw_sprite("walk_marker-sheet0", qx - 4.f, qy - 5.f, 8.f, 10.f, 0.f, 0.95f);
        else
          platform.draw_rect(qx - 2.f, qy - 2.f, 4.f, 4.f, 0.95f, 0.25f, 0.2f, 1.f);
      }
    }
    float ppx = 0.f, ppy = 0.f;
    to_mini(pl.pos.x, pl.pos.y, ppx, ppy);
    const float dot = gw(20.f);
    if (spr(platform, "minimap_player-sheet0"))
      platform.draw_sprite("minimap_player-sheet0", ppx - dot * 0.5f, ppy - dot * 0.5f, dot, dot,
                           0.f, 1.f);
    else
      platform.draw_rect(ppx - dot * 0.25f, ppy - dot * 0.25f, dot * 0.5f, dot * 0.5f,
                         0.3f, 0.95f, 0.4f, 1.f);
    if (spr(platform, "minimap_dragger-sheet0"))
      platform.draw_sprite("minimap_dragger-sheet0", mx + mw - gw(10.f), my + mh - gh(15.f),
                           gw(10.f), gh(15.f), 0.f, 0.75f);
  }
}

} // namespace mdaz
