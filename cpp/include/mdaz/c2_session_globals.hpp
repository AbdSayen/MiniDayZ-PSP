#pragma once

#include "mdaz/inventory.hpp"
#include "mdaz/loot.hpp"
#include "mdaz/mapgen.hpp"
#include "mdaz/survival.hpp"
#include "mdaz/types.hpp"
#include "mdaz/weapons.hpp"

namespace mdaz {

struct CraftRecipe;

// Runtime mirrors of C2 Game_events globals (Mini DAYZ 1.4.1 data.js names).
struct C2SessionGlobals {
  double Melee_mode;
  double Silencer_name_id;
  double Scope_name_id;
  double Belt_name_id;
  double water_left;
  double player_water_left;
  double pickpile_index;
  double slots_filled;
  double full_panels;
  double avalible_interaction;
  double item_for_remove_id;
  double required_count;
  double charge_count;
  double Condition;
  double Condition2;
  double hotspotX;
  double hotspotY;
  double HotspotX;
  double HotspotY;
  double TilemapspotX;
  double TilemapspotY;
  double hotspot_mapgen_X;
  double hotspot_mapgen_Y;
  double PlantX;
  double PlantY;
  double rad_dist;
  double minimap_positionX;
  double minimap_positionY;
  double Loop_secrets;
  double Loop_secrets2;
  double smth_found;
  double post_points;
  double main_tile;
  double anotherX;
  double anotherX2;
  double Map_loading;
  double to_checked;
  double Variable1;
  double char_select_gap;
  double btn_pos_y_margin;

  // Tutorial / mobile movement (C2 globals; desktop uses init_desktop_controls).
  double tutorBleedingStopped;
  double tutorMoveChoosen;
  double tutorMovementType;
  double tutor_bleeding_step;
  double movement_tap_enabled;
  double movement_stick_enabled;
  double run_touch_id;
  double stick_touch_id;
  double force_stick;
  double alt_move_for_tap;

  C2SessionGlobals();
  void reset_menu_layout();
};

void touch_c2_session_global_audit();

void sync_melee_mode(C2SessionGlobals& g, const WeaponDef* equipped);
void sync_weapon_attachment_globals(C2SessionGlobals& g, const Inventory& inv,
                                    const WeaponDef* equipped);
float scope_dispersion_mul_from_scope_id(ItemId scope_id);
bool silencer_matches_ammo(ItemId silencer_id, ItemId ammo_id);

void sync_water_globals(C2SessionGlobals& g, const Inventory& inv,
                        const SurvivalState& survival);
void sync_worn_condition_globals(C2SessionGlobals& g, const Inventory& inv);

void sync_pickpile_globals(C2SessionGlobals& g, const LootSystem& loot, Vec2 player,
                           const Inventory& inv, float reach);

void c2_stage_craft_removal(C2SessionGlobals& g, ItemId id, int count);
void c2_note_loot_found(C2SessionGlobals& g, bool found);

float compute_rad_dist(Vec2 player, const MapGenerator& map);

void mapgen_note_location_hotspot(C2SessionGlobals& g, int sector_x, int sector_y,
                                  float world_x, float world_y);
void mapgen_note_road_pair(C2SessionGlobals& g, int ax, int ax2, unsigned char tile);

void set_minimap_position_globals(C2SessionGlobals& g, float mx, float my);

float menu_char_select_y(float base_y, const C2SessionGlobals& g);
float menu_button_y(float base_y, int button_index, const C2SessionGlobals& g);

} // namespace mdaz
