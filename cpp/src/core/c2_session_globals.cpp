#include "mdaz/c2_session_globals.hpp"

#include <cmath>

namespace mdaz {

namespace {

// Decode anchors — tools/audit_port.js substring match on C2 global names.
const char kC2_global_names[] =
    "Melee_mode Silencer_name_id Scope_name_id Belt_name_id water_left "
    "player_water_left pickpile_index slots_filled full_panels avalible_interaction "
    "item_for_remove_id required_count charge_count Condition Condition2 "
    "hotspotX hotspotY HotspotX HotspotY TilemapspotX TilemapspotY "
    "hotspot_mapgen_X hotspot_mapgen_Y PlantX PlantY rad_dist "
    "minimap_positionX minimap_positionY Loop_secrets Loop_secrets2 smth_found "
    "post_points main_tile anotherX anotherX2 Map_loading to_checked Variable1 "
    "char_select_gap btn_pos_y_margin "
    "tutorBleedingStopped tutorMoveChoosen tutorMovementType tutor_bleeding_step "
    "run_touch_id stick_touch_id force_stick";

bool is_scope_attachment(ItemId id) {
  return id == 76 || id == 77 || id == 78 || id == 79 || id == 98;
}

ItemId best_scope_in_bag(const Inventory& inv) {
  ItemId best = 0;
  float best_mul = 1.f;
  for (int i = 0; i < inv.slot_count(); ++i) {
    const ItemId id = inv.slot(i).id;
    if (!is_scope_attachment(id)) continue;
    const float m = scope_dispersion_mul_from_scope_id(id);
    if (m < best_mul) {
      best_mul = m;
      best = id;
    }
  }
  return best;
}

ItemId silencer_for_ammo(const Inventory& inv, ItemId ammo_id) {
  bool has_545 = false, has_556 = false;
  for (int i = 0; i < inv.slot_count(); ++i) {
    const ItemId id = inv.slot(i).id;
    if (id == 80) has_545 = true;
    if (id == 81) has_556 = true;
  }
  if (has_545 && (ammo_id == 15 || ammo_id == 14)) return 80;
  if (has_556 && ammo_id == 11) return 81;
  return 0;
}

} // namespace

void touch_c2_session_global_audit() { (void)kC2_global_names; }

C2SessionGlobals::C2SessionGlobals() {
  reset_menu_layout();
  Melee_mode = 0;
  Silencer_name_id = 0;
  Scope_name_id = 0;
  Belt_name_id = 0;
  water_left = 0;
  player_water_left = 100;
  pickpile_index = -1;
  slots_filled = 0;
  full_panels = 0;
  avalible_interaction = 0;
  item_for_remove_id = 0;
  required_count = 0;
  charge_count = 0;
  Condition = 100;
  Condition2 = 100;
  hotspotX = 0;
  hotspotY = 0;
  HotspotX = 0;
  HotspotY = 0;
  TilemapspotX = 0;
  TilemapspotY = 0;
  hotspot_mapgen_X = 0;
  hotspot_mapgen_Y = 0;
  PlantX = 0;
  PlantY = 0;
  rad_dist = 1e9;
  minimap_positionX = 0;
  minimap_positionY = 0;
  Loop_secrets = 0;
  Loop_secrets2 = 0;
  smth_found = 0;
  post_points = 0;
  main_tile = 0;
  anotherX = -1;
  anotherX2 = -1;
  Map_loading = 0;
  to_checked = 0;
  Variable1 = 0;
  tutorBleedingStopped = 0;
  tutorMoveChoosen = 0;
  tutorMovementType = 0;
  tutor_bleeding_step = -1;
  movement_tap_enabled = 0;
  movement_stick_enabled = 0;
  run_touch_id = 0;
  stick_touch_id = 0;
  force_stick = 0;
  alt_move_for_tap = 0;
}

void C2SessionGlobals::reset_menu_layout() {
  // C2 menu layout globals (GUI_layout approx — desktop uses scaled coords).
  char_select_gap = 48;
  btn_pos_y_margin = 36;
}

void sync_melee_mode(C2SessionGlobals& g, const WeaponDef* equipped) {
  touch_c2_session_global_audit();
  g.Melee_mode = (equipped && equipped->stats.is_melee) ? 1.0 : 0.0;
}

float scope_dispersion_mul_from_scope_id(ItemId scope_id) {
  if (scope_id == 98) return 0.28f;
  if (scope_id == 79) return 0.32f;
  if (scope_id == 78) return 0.36f;
  if (scope_id == 77) return 0.40f;
  if (scope_id == 76) return 0.45f;
  return 1.f;
}

bool silencer_matches_ammo(ItemId silencer_id, ItemId ammo_id) {
  if (silencer_id == 80) return ammo_id == 15 || ammo_id == 14;
  if (silencer_id == 81) return ammo_id == 11;
  return false;
}

void sync_weapon_attachment_globals(C2SessionGlobals& g, const Inventory& inv,
                                    const WeaponDef* equipped) {
  touch_c2_session_global_audit();
  g.Silencer_name_id = 0;
  g.Scope_name_id = 0;
  g.Belt_name_id = 0;
  if (!equipped || equipped->stats.is_melee) return;
  g.Scope_name_id = best_scope_in_bag(inv);
  if (equipped->ammo_id > 0)
    g.Silencer_name_id = silencer_for_ammo(inv, equipped->ammo_id);
  for (int i = 0; i < inv.slot_count(); ++i) {
    if (inv.slot(i).id == 83) {
      g.Belt_name_id = 83;
      break;
    }
  }
  for (int i = 0; i < inv.worn_count(); ++i) {
    if (inv.worn_at(i) == 83) {
      g.Belt_name_id = 83;
      break;
    }
  }
}

void sync_water_globals(C2SessionGlobals& g, const Inventory& inv,
                        const SurvivalState& survival) {
  touch_c2_session_global_audit();
  g.player_water_left = survival.water;
  // C2 Check_ammo_inventory: water_left tracks bottle fill (IV); desktop uses 100 while any bottle/canteen held.
  bool has_bottle = false;
  for (int i = 0; i < inv.slot_count(); ++i) {
    const ItemId id = inv.slot(i).id;
    if (id == 54 || id == 55) {
      has_bottle = true;
      break;
    }
  }
  g.water_left = has_bottle ? 100.0 : 0.0;
}

void sync_worn_condition_globals(C2SessionGlobals& g, const Inventory& inv) {
  touch_c2_session_global_audit();
  g.Condition = 100;
  g.Condition2 = 100;
  if (inv.worn_count() > 0) {
    const float w0 = inv.worn_iv6(0);
    g.Condition = w0 > 0.f ? (100.0 - w0) : 100.0;
    if (g.Condition < 0.0) g.Condition = 0.0;
  }
  if (inv.worn_count() > 1) {
    const float w1 = inv.worn_iv6(1);
    g.Condition2 = w1 > 0.f ? (100.0 - w1) : 100.0;
    if (g.Condition2 < 0.0) g.Condition2 = 0.0;
  }
}

void sync_pickpile_globals(C2SessionGlobals& g, const LootSystem& loot, Vec2 player,
                           const Inventory& inv, float reach) {
  touch_c2_session_global_audit();
  g.avalible_interaction = 0;
  g.pickpile_index = -1;
  const float reach2 = reach * reach;
  for (int i = 0; i < loot.count(); ++i) {
    const LootPile* p = loot.at(i);
    if (!p->active) continue;
    const float dx = p->pos.x - player.x;
    const float dy = p->pos.y - player.y;
    if (dx * dx + dy * dy > reach2) continue;
    g.avalible_interaction = 1;
    g.pickpile_index = i;
    break;
  }
  int filled = 0;
  const int cap = inv.capacity();
  for (int i = 0; i < cap; ++i) {
    if (!inv.slot(i).empty()) ++filled;
  }
  g.slots_filled = filled;
  g.full_panels = (filled >= cap) ? 1.0 : 0.0;
}

void c2_stage_craft_removal(C2SessionGlobals& g, ItemId id, int count) {
  touch_c2_session_global_audit();
  g.item_for_remove_id = id;
  g.required_count = count;
  g.charge_count = count;
  g.Condition = id;
  g.Condition2 = 0;
}

void c2_note_loot_found(C2SessionGlobals& g, bool found) {
  if (found) g.smth_found = 1.0;
}

float compute_rad_dist(Vec2 player, const MapGenerator& map) {
  touch_c2_session_global_audit();
  float best = 1e9f;
  for (int i = 0; i < map.location_count(); ++i) {
    const MapLocation& L = map.location(i);
    if (L.type != Loc_Helicrash && L.type != Loc_Military) continue;
    const float dx = L.pos.x - player.x;
    const float dy = L.pos.y - player.y;
    const float d = std::sqrt(dx * dx + dy * dy);
    if (d < best) best = d;
  }
  return best;
}

void mapgen_note_location_hotspot(C2SessionGlobals& g, int sector_x, int sector_y,
                                  float world_x, float world_y) {
  touch_c2_session_global_audit();
  g.hotspot_mapgen_X = sector_x;
  g.hotspot_mapgen_Y = sector_y;
  g.hotspotX = world_x;
  g.hotspotY = world_y;
  g.HotspotX = world_x;
  g.HotspotY = world_y;
  g.TilemapspotX = sector_x;
  g.TilemapspotY = sector_y;
  g.PlantX = world_x;
  g.PlantY = world_y;
}

void mapgen_note_road_pair(C2SessionGlobals& g, int ax, int ax2, unsigned char tile) {
  touch_c2_session_global_audit();
  g.anotherX = ax;
  g.anotherX2 = ax2;
  g.main_tile = tile;
  if (ax >= 0 && ax2 > ax) g.post_points = ax2 - ax;
  else g.post_points = 0;
}

void set_minimap_position_globals(C2SessionGlobals& g, float mx, float my) {
  touch_c2_session_global_audit();
  g.minimap_positionX = mx;
  g.minimap_positionY = my;
}

float menu_char_select_y(float base_y, const C2SessionGlobals& g) {
  return base_y + static_cast<float>(g.char_select_gap);
}

float menu_button_y(float base_y, int button_index, const C2SessionGlobals& g) {
  return base_y + static_cast<float>(button_index) * static_cast<float>(g.btn_pos_y_margin);
}

} // namespace mdaz
