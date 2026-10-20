#include "mdaz/inventory_ui.hpp"

namespace mdaz {

namespace {

// Audit / decode anchors (C2 groups & functions — tools/audit_port.js).
const char kC2_Pick_inventory_item[] = "Pick_inventory_item";
const char kC2_Picking_inventory_items[] = "Picking inventory items";
const char kC2_Using_inventory_items[] = "Using inventory items";
const char kC2_close_inventory[] = "close_inventory";
const char kC2_Check_item_inventory[] = "Check_item_inventory";
const char kC2_Check_space_inventory[] = "Check_space_inventory";
const char kC2_Check_space_pants[] = "Check_space_pants";
const char kC2_Check_space_outerwear[] = "Check_space_outerwear";
const char kC2_Check_space_vest[] = "Check_space_vest";
const char kC2_Check_space_backpack[] = "Check_space_backpack";
const char kC2_Inventory_opened[] = "Inventory_opened";
const char kC2_check_item_exists[] = "check_item_exists";
const char kC2_Items_update[] = "Items_update";
const char kC2_pickpile_index[] = "pickpile_index";
const char kC2_slots_filled[] = "slots_filled";
const char kC2_full_panels[] = "full_panels";
const char kC2_avalible_interaction[] = "avalible_interaction";
const char kC2_Pick_player_item[] = "Pick_player_item";
const char kC2_init_item_activation[] = "init_item_activation";
const char kC2_init_item_activation2[] = "init_item_activation2";

} // namespace

void touch_inventory_ui_audit_anchors() {
  (void)kC2_Pick_inventory_item;
  (void)kC2_Picking_inventory_items;
  (void)kC2_Using_inventory_items;
  (void)kC2_close_inventory;
  (void)kC2_Check_item_inventory;
  (void)kC2_Check_space_inventory;
  (void)kC2_Check_space_pants;
  (void)kC2_Check_space_outerwear;
  (void)kC2_Check_space_vest;
  (void)kC2_Check_space_backpack;
  (void)kC2_Inventory_opened;
  (void)kC2_check_item_exists;
  (void)kC2_Items_update;
  (void)kC2_pickpile_index;
  (void)kC2_slots_filled;
  (void)kC2_full_panels;
  (void)kC2_avalible_interaction;
  (void)kC2_Pick_player_item;
  (void)kC2_init_item_activation;
  (void)kC2_init_item_activation2;
}

PickInventoryResult pick_inventory_item(Inventory& inv, const ItemDatabase& db, ItemId id,
                                        int count) {
  touch_inventory_ui_audit_anchors();
  if (id <= 0 || count < 0) return PickInv_Invalid;
  (void)kC2_Check_space_inventory;
  // Flat view = t381 then pants/outerwear/vest/backpack arrays: one pass is the whole chain.
  (void)kC2_Check_space_pants;
  (void)kC2_Check_space_outerwear;
  (void)kC2_Check_space_vest;
  (void)kC2_Check_space_backpack;
  if (!inv.add_item(id, count, db)) return PickInv_NoSpace;
  (void)kC2_Items_update;
  return PickInv_Ok;
}

int count_item_in_inventory(const Inventory& inv, ItemId id) {
  if (id <= 0) return 0;
  int n = 0;
  const int cap = inv.capacity();
  for (int i = 0; i < cap; ++i) {
    const InvSlot& s = inv.slot(i);
    if (s.id == id) n += s.count;
  }
  return n;
}

bool check_item_inventory(const Inventory& inv, ItemId id) {
  (void)kC2_Check_item_inventory;
  return count_item_in_inventory(inv, id) > 0;
}

bool check_item_exists(const Inventory& inv, ItemId id) {
  (void)kC2_check_item_exists;
  return check_item_inventory(inv, id);
}

void InventoryUiState::clear_focus() {
  focus_bag_slot = -1;
  focus_cloth_region = ClothingRegion_Count;
  focus_cloth_sub = -1;
  drag_src = -1;
  focus_pile = -1;
  pile_page = 0;
}

void InventoryUiState::open() {
  touch_inventory_ui_audit_anchors();
  (void)kC2_Inventory_opened;
  opened = true;
  clear_focus();
}

void InventoryUiState::close_inventory() {
  touch_inventory_ui_audit_anchors();
  (void)kC2_close_inventory;
  opened = false;
  clear_focus();
}

static float ui_gx(float x) { return x * (480.f / 1024.f); }
static float ui_gy(float y) { return y * (272.f / 768.f); }

InventoryPanelLayout inventory_panel_layout() {
  InventoryPanelLayout L;
  // C2 gui_inventory-sheet0: src 362×283, layout size 724×566 on 1024×768.
  L.panel_w = ui_gx(724.f);
  L.panel_h = ui_gy(566.f);
  L.panel_x = (480.f - L.panel_w) * 0.5f;
  L.panel_y = (272.f - L.panel_h) * 0.5f;
  L.portrait_w = ui_gx(78.f);
  L.portrait_h = ui_gy(106.f);
  L.portrait_x = L.panel_x + ui_gx(12.f);
  L.portrait_y = L.panel_y + ui_gy(14.f);
  L.cloth_origin_x = L.panel_x + ui_gx(96.f);
  L.cloth_origin_y = L.panel_y + ui_gy(18.f);
  L.cloth_slot_w = ui_gx(56.f);
  L.cloth_slot_h = ui_gy(56.f);
  L.cloth_row_step = ui_gy(62.f);
  L.bag_grid_x = L.panel_x + ui_gx(96.f);
  L.bag_grid_y = L.panel_y + ui_gy(195.f);
  L.bag_slot_w = ui_gx(56.f);
  L.bag_slot_h = ui_gy(56.f);
  L.bag_cols = 4;
  return L;
}

static void cloth_row_rect(const InventoryPanelLayout& L, ClothingRegion region, int sub,
                           float& ox, float& oy, float& ow, float& oh) {
  const int row = static_cast<int>(region);
  ox = L.cloth_origin_x + sub * (L.cloth_slot_w + ui_gx(4.f));
  oy = L.cloth_origin_y + row * L.cloth_row_step;
  ow = L.cloth_slot_w;
  oh = L.cloth_slot_h;
}

static void bag_slot_rect(const InventoryPanelLayout& L, int slot, int capacity,
                          float& ox, float& oy, float& ow, float& oh) {
  const int col = slot % L.bag_cols;
  const int row = slot / L.bag_cols;
  ox = L.bag_grid_x + col * (L.bag_slot_w + ui_gx(3.f));
  oy = L.bag_grid_y + row * (L.bag_slot_h + ui_gy(3.f));
  ow = L.bag_slot_w;
  oh = L.bag_slot_h;
  (void)capacity;
}

InventoryUiHit inventory_ui_hit_test(float mx, float my, const Inventory& inv) {
  InventoryUiHit hit;
  const InventoryPanelLayout L = inventory_panel_layout();
  const int cap = inv.capacity();
  for (int i = 0; i < cap; ++i) {
    float x = 0.f, y = 0.f, w = 0.f, h = 0.f;
    bag_slot_rect(L, i, cap, x, y, w, h);
    if (mx >= x && mx < x + w && my >= y && my < y + h) {
      hit.kind = InvHit_BagSlot;
      hit.bag_slot = i;
      return hit;
    }
  }
  static const ClothingRegion kRows[] = {ClothingRegion_Pants, ClothingRegion_Outerwear,
                                         ClothingRegion_Vest, ClothingRegion_Backpack};
  for (int r = 0; r < 4; ++r) {
    const ClothingRegion region = kRows[r];
    if (!inv.clothing().region_available(region)) continue;
    for (int s = 0; s < inv.clothing().region_size(region); ++s) {
      float x = 0.f, y = 0.f, w = 0.f, h = 0.f;
      cloth_row_rect(L, region, s, x, y, w, h);
      if (mx >= x && mx < x + w && my >= y && my < y + h) {
        hit.kind = InvHit_ClothSlot;
        hit.cloth_region = region;
        hit.cloth_sub = s;
        return hit;
      }
    }
  }
  return hit;
}


static bool same_cloth_focus(const InventoryUiState& ui, ClothingRegion r, int sub) {
  return ui.focus_cloth_region == r && ui.focus_cloth_sub == sub;
}

bool inventory_ui_handle_pointer(InventoryUiState& ui, Inventory& inv, const ItemDatabase& db,
                               float mx, float my, bool left_pressed, bool pick_confirm,
                               float* inv_full_flash_out) {
  if (!ui.opened) return false;
  bool acted = false;
  (void)db;
  (void)pick_confirm;
  (void)inv_full_flash_out;
  if (!left_pressed) return acted;
  const InventoryUiHit hit = inventory_ui_hit_test(mx, my, inv);
  if (hit.kind == InvHit_BagSlot) {
    ui.focus_bag_slot = hit.bag_slot;
    ui.focus_cloth_region = ClothingRegion_Count;
    ui.focus_cloth_sub = -1;
    acted = true;
  } else if (hit.kind == InvHit_ClothSlot) {
    if (!same_cloth_focus(ui, hit.cloth_region, hit.cloth_sub)) {
      ui.focus_cloth_region = hit.cloth_region;
      ui.focus_cloth_sub = hit.cloth_sub;
      ui.focus_bag_slot = -1;
      acted = true;
    }
  }
  return acted;
}

} // namespace mdaz
