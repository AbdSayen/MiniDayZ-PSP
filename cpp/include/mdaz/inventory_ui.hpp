#pragma once

#include "mdaz/clothing_containers.hpp"
#include "mdaz/inventory.hpp"
#include "mdaz/item_db.hpp"
#include "mdaz/types.hpp"

namespace mdaz {

class IPlatform;

// C2 Pick_inventory_item / Check_space_inventory — add from ground or pile to bag.
enum PickInventoryResult {
  PickInv_Ok = 0,
  PickInv_NoSpace,
  PickInv_Invalid
};

PickInventoryResult pick_inventory_item(Inventory& inv, const ItemDatabase& db, ItemId id,
                                        int count);

// C2 Pick_player_item — move one stack from worn clothing IV93 sub-slot into bag (no drag UI).


// C2 Check_item_inventory / check_item_exists — bag slots only (not worn clothing containers).
int count_item_in_inventory(const Inventory& inv, ItemId id);
bool check_item_inventory(const Inventory& inv, ItemId id);
bool check_item_exists(const Inventory& inv, ItemId id);

// Layout from GUI_layout layer 53: gui_inventory 362×283 @ 2× scale → 724×566 in 1024×768.
struct InventoryPanelLayout {
  float panel_x;
  float panel_y;
  float panel_w;
  float panel_h;
  float portrait_x;
  float portrait_y;
  float portrait_w;
  float portrait_h;
  float cloth_origin_x;
  float cloth_origin_y;
  float cloth_slot_w;
  float cloth_slot_h;
  float cloth_row_step;
  float bag_grid_x;
  float bag_grid_y;
  float bag_slot_w;
  float bag_slot_h;
  int bag_cols;
};

InventoryPanelLayout inventory_panel_layout();

enum InventoryUiHitKind {
  InvHit_None = 0,
  InvHit_BagSlot,
  InvHit_ClothSlot
};

struct InventoryUiHit {
  InventoryUiHitKind kind;
  int bag_slot;
  ClothingRegion cloth_region;
  int cloth_sub;

  InventoryUiHit()
    : kind(InvHit_None), bag_slot(-1), cloth_region(ClothingRegion_Count), cloth_sub(-1) {}
};

InventoryUiHit inventory_ui_hit_test(float mx, float my, const Inventory& inv);

// C2 close_inventory / Inventory_opened — bag + craft sub-menu.
struct InventoryUiState {
  bool opened;
  int focus_bag_slot;
  ClothingRegion focus_cloth_region;
  int focus_cloth_sub;
  // PSP input adaptation of C2 drag-onto-slot + sub_menu "Combine": marked source bag slot.
  int combine_src;

  InventoryUiState()
    : opened(false), focus_bag_slot(-1), focus_cloth_region(ClothingRegion_Count),
      focus_cloth_sub(-1), combine_src(-1) {}

  void open();
  void close_inventory();
  void clear_focus();
};

// Desktop: click to focus bag / clothing slots; E or second click on clothing → Pick_player_item.
bool inventory_ui_handle_pointer(InventoryUiState& ui, Inventory& inv, const ItemDatabase& db,
                                 float mx, float my, bool left_pressed, bool pick_confirm,
                                 float* inv_full_flash_out);

void touch_inventory_ui_audit_anchors();

} // namespace mdaz
