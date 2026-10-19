#include "mdaz/game.hpp"
#include "mdaz/combat.hpp"
#include "mdaz/craft.hpp"
#include "mdaz/hud.hpp"
#include "mdaz/item_sprites.hpp"
#include "mdaz/map_labels.hpp"
#include "mdaz/psp_ui.hpp"
#include "mdaz/save_game.hpp"
#include "mdaz/weapons.hpp"

#include <cmath>
#include <new>
#include <cstdio>
#include <cstring>

namespace mdaz {

namespace {

float dist2(Vec2 a, Vec2 b) {
  const float dx = a.x - b.x;
  const float dy = a.y - b.y;
  return dx * dx + dy * dy;
}

float world_to_screen_scale() { return 0.55f; }

// POIs this close to the new-game spawn get no hostiles at start.
const float kSpawnSafeRadius = 600.f;

// C2 character sheets (data.js 1.4.1): 30×30 cells, stride 32, origin (1,1).
// Idle: one frame per facing on row 0 — down(1,1) up(33,1) right(65,1) left(97,1).
// Run: animation run_{dir} = 4 frames across columns on a fixed row:
//   run_down y=33, run_up y=65, run_right y=97, run_left y=1 (reuses idle row).
enum FaceDir { Face_Down = 0, Face_Up = 1, Face_Right = 2, Face_Left = 3 };

FaceDir facing_to_dir(float deg) {
  while (deg > 180.f) deg -= 360.f;
  while (deg < -180.f) deg += 360.f;
  if (deg >= -45.f && deg < 45.f) return Face_Right;
  if (deg >= 45.f && deg < 135.f) return Face_Down;
  if (deg >= -135.f && deg < -45.f) return Face_Up;
  return Face_Left;
}

void sprite_sheet_name(char* out, int out_n, const char* base, int sheet) {
  if (sheet < 0) sheet = 0;
  if (sheet > 2) sheet = sheet % 3;
  std::snprintf(out, out_n, "%s-sheet%d", base, sheet);
}

void draw_c2_frame(IPlatform* p, const char* base, int sheet,
                   float sx, float sy, float size, int srcx, int srcy,
                   int cell = 30) {
  if (!p || !base) return;
  char name[64];
  sprite_sheet_name(name, (int)sizeof(name), base, sheet);
  if (!p->has_sprite(name)) return;
  p->draw_sprite_frame(name, sx - size * 0.5f, sy - size * 0.5f, size, size,
                       srcx, srcy, cell, cell, 0.f, 1.f);
}

// Draw via CharAnims tables when available; else UV-grid fallback.
void draw_c2_char(IPlatform* p, const CharAnims* anims, float sx, float sy, float size,
                  const char* base, float facing_deg, float anim_t, const char* prefix,
                  int cell = 30, int stride = 32) {
  if (!p || !base) return;
  char aname[40];
  CharAnims::make_name(aname, (int)sizeof(aname), prefix ? prefix : "idle", facing_deg);
  if (anims && anims->loaded() && anims->find(base, aname)) {
    anims->draw(*p, base, aname, anim_t, sx, sy, size, size);
    return;
  }
  // Fallback: 30×30 grid matching data.js idle/run packing.
  const FaceDir dir = facing_to_dir(facing_deg);
  const bool moving = prefix && (std::strcmp(prefix, "run") == 0 || std::strcmp(prefix, "walk") == 0);
  int sheet = 0, col = 0, row = 0;
  int walk_frame = moving ? (1 + (static_cast<int>(anim_t * 10.f) % 4)) : 0;
  if (cell == 38 && stride == 40) {
    if (walk_frame <= 0) {
      if (dir == Face_Down) { col = 0; row = 0; }
      else if (dir == Face_Up) { col = 1; row = 0; }
      else if (dir == Face_Right) { col = 2; row = 0; }
      else { col = 0; row = 1; }
    } else {
      const int fr = (walk_frame - 1) % 3;
      if (dir == Face_Down) { col = fr; row = 1; }
      else if (dir == Face_Up) { col = fr; row = 2; }
      else if (dir == Face_Right) { col = fr % 3; row = 2; }
      else { col = fr % 3; row = 1; }
    }
  } else if (walk_frame <= 0) {
    col = static_cast<int>(dir);
    row = 0;
  } else {
    col = (walk_frame - 1) % 4;
    if (dir == Face_Down) { row = 1; }
    else if (dir == Face_Up) { row = 2; }
    else if (dir == Face_Right) { row = 3; }
    else { sheet = 1; row = 0; }
  }
  draw_c2_frame(p, base, sheet, sx, sy, size,
                1 + col * stride, 1 + row * stride, cell);
}

enum HoldPose { Hold_None, Hold_Pistol, Hold_TwoHand, Hold_Axe };

HoldPose hold_pose_for(const WeaponDef* w) {
  if (!w) return Hold_None;
  if (w->stats.is_melee) {
    if (w->item_id == 202 || w->item_id == 204 || w->item_id == 206) return Hold_Axe;
    return Hold_None;
  }
  // One-hand pistols / SMGs from weapons table
  const ItemId id = w->item_id;
  if (id == 151 || id == 152 || id == 153 || id == 165 || id == 169 ||
      id == 170 || id == 179) return Hold_Pistol;
  return Hold_TwoHand;
}

// Pick body skin from worn clothing (C2 style skins, not inventory icon overlays).
const char* player_body_skin(const Inventory& inv) {
  bool mil = false, hunt = false, civil = false, mask = false;
  for (int i = 0; i < inv.worn_count(); ++i) {
    const ItemId id = inv.worn_at(i);
    if (id == 250 || id == 257 || id == 262) mask = true;
    if (id == 251 || id == 263 || id == 265 || id == 303 || id == 355 ||
        id == 400 || id == 402 || id == 403 || id == 404)
      mil = true;
    if (id == 306 || id == 364 || id == 451 || id == 254 || id == 258)
      hunt = true;
    if (id == 353 || id == 300 || id == 301 || id == 350)
      civil = true;
  }
  if (mask) return "player_skin_maniac";
  if (mil) return "player_skin_veteran";
  if (hunt) return "player_skin_survivalist";
  if (civil) return "player_skin_redneck";
  return "player_skin_def_1";
}

void draw_player_c2(IPlatform* p, const CharAnims* anims, float sx, float sy, float size,
                    float facing_deg, float move_anim_t, float melee_anim_t, float use_anim_t,
                    bool moving, bool running, HoldPose hold, const char* body_skin) {
  if (!p) return;
  if (!body_skin) body_skin = "player_skin_def_1";
  char probe[64];
  sprite_sheet_name(probe, (int)sizeof(probe), body_skin, 0);
  if (!p->has_sprite(probe)) body_skin = "player_skin_def_1";

  char aname[40];
  float t = 0.f;
  if (use_anim_t > 0.f) {
    std::snprintf(aname, sizeof(aname), "using_item");
    t = 0.5f - use_anim_t; // started at 0.5s
    if (t < 0.f) t = 0.f;
  } else if (melee_anim_t > 0.f && hold == Hold_Axe) {
    CharAnims::make_name(aname, (int)sizeof(aname), "axe", facing_deg);
    t = 0.35f - melee_anim_t;
    if (t < 0.f) t = 0.f;
  } else if (moving) {
    CharAnims::make_name(aname, (int)sizeof(aname), running ? "run" : "run", facing_deg);
    t = move_anim_t;
  } else if (hold == Hold_Pistol) {
    CharAnims::make_name(aname, (int)sizeof(aname), "pistol", facing_deg);
    t = 0.f;
  } else if (hold == Hold_TwoHand) {
    CharAnims::make_name(aname, (int)sizeof(aname), "twohand", facing_deg);
    t = 0.f;
  } else if (hold == Hold_Axe) {
    CharAnims::make_name(aname, (int)sizeof(aname), "axe", facing_deg);
    t = 0.f;
  } else {
    CharAnims::make_name(aname, (int)sizeof(aname), "idle", facing_deg);
    t = 0.f;
  }

  if (anims && anims->loaded() && anims->find(body_skin, aname)) {
    anims->draw(*p, body_skin, aname, t, sx, sy, size, size);
    if (anims->find("player_hands_white", aname))
      anims->draw(*p, "player_hands_white", aname, t, sx, sy, size, size);
    return;
  }

  // UV fallback
  const char* prefix = "idle";
  if (moving) prefix = "run";
  draw_c2_char(p, 0, sx, sy, size, body_skin, facing_deg, t, prefix);
  draw_c2_char(p, 0, sx, sy, size, "player_hands_white", facing_deg, t, prefix);
}

// Pine tree: first frame is 47×92 at 1,1 (not the whole 128 sheet).
void draw_pine(IPlatform* p, float sx, float sy, float h) {
  if (!p) return;
  const char* name = "tree_pine-sheet0";
  if (!p->has_sprite(name)) name = "tree_pine2-sheet0";
  if (!p->has_sprite(name)) return;
  const float w = h * (47.f / 92.f);
  // Anchor = trunk base (C2 tree_pine Imagepoint 1 at 0.53,0.924 of the 47x92 frame) so the
  // drawn trunk sits on the collision point (was drawn ~12px too low: player seemed to walk
  // through trunks).
  p->draw_sprite_frame(name, sx - w * 0.53f, sy - h * 0.924f, w, h, 1, 1, 47, 92, 0.f, 1.f);
}

const char* poi_building_sprite(LocationType t, int variant) {
  switch (t) {
  case Loc_Hospital: return "b_hospital-sheet0";
  case Loc_GasStation: return "b_gas_station-sheet0";
  case Loc_Police: return "b_police_station-sheet0";
  case Loc_Firestation: return "b_fire_station-sheet0";
  case Loc_Church: return "b_church-sheet0";
  case Loc_Military: return (variant & 1) ? "b_military_barrack-sheet0" : "b_military_shtab-sheet0";
  case Loc_Helicrash: return "heli_crash-sheet0";
  case Loc_BunkerEntrance: return "b_hunter_shelter-sheet0";
  case Loc_Secret: return "b_hunter_shelter-sheet0";
  case Loc_City: return "b_hostel-sheet0";
  case Loc_Village:
  default:
    switch (variant % 4) {
    case 0: return "b_village_brown-sheet0";
    case 1: return "b_village_green-sheet0";
    case 2: return "b_village_red-sheet0";
    default: return "b_village_yellow-sheet0";
    }
  }
}

const char* poi_interior_sprite(LocationType t, int variant) {
  switch (t) {
  case Loc_Hospital: return "b_hospital_int-sheet0";
  case Loc_GasStation: return "b_gas_station_int-sheet0";
  case Loc_Police: return "b_police_station_int-sheet0";
  case Loc_Firestation: return "b_fire_station_int-sheet0";
  case Loc_Church: return "b_church_int-sheet0";
  case Loc_Military:
    return (variant & 1) ? "b_military_barrack_int-sheet0" : "b_military_shtab_int-sheet0";
  case Loc_BunkerEntrance:
  case Loc_Secret: return "b_hunter_shelter_int-sheet0";
  case Loc_City: return "b_hostel_int-sheet0";
  case Loc_Village:
  default:
    switch (variant % 4) {
    case 0: return "b_village_brown_int-sheet0";
    case 1: return "b_village_green_int-sheet0";
    case 2: return "b_village_red_int-sheet0";
    default: return "b_village_yellow_int-sheet0";
    }
  }
}

// Native frame sizes from data.js (sheet0 frame 0).
void poi_native_px(const char* spr, int& nw, int& nh) {
  nw = 100; nh = 100;
  if (!spr) return;
  if (std::strstr(spr, "b_hospital")) { nw = 512; nh = 404; }
  else if (std::strstr(spr, "b_gas_station")) { nw = 107; nh = 107; }
  else if (std::strstr(spr, "b_police")) { nw = 238; nh = 235; }
  else if (std::strstr(spr, "b_fire")) { nw = 545; nh = 287; }
  else if (std::strstr(spr, "b_church")) { nw = 296; nh = 532; }
  else if (std::strstr(spr, "b_military_barrack")) { nw = 282; nh = 143; }
  else if (std::strstr(spr, "b_military_shtab")) { nw = 353; nh = 197; }
  else if (std::strstr(spr, "b_hunter")) { nw = 76; nh = 68; }
  else if (std::strstr(spr, "b_hostel")) { nw = 170; nh = 155; }
  else if (std::strstr(spr, "b_village_brown")) { nw = 172; nh = 156; }
  else if (std::strstr(spr, "b_village_green")) { nw = 137; nh = 134; }
  else if (std::strstr(spr, "b_village_red")) { nw = 175; nh = 155; }
  else if (std::strstr(spr, "b_village_yellow")) { nw = 187; nh = 120; }
  else if (std::strstr(spr, "heli_crash")) { nw = 240; nh = 186; }
  else if (std::strstr(spr, "car_explode")) { nw = 100; nh = 100; }
}

void draw_poi_sprite(IPlatform* p, float sx, float sy, const char* spr, float world_scale) {
  if (!p || !spr) return;
  int nw = 100, nh = 100;
  poi_native_px(spr, nw, nh);
  const float s = world_scale;
  const float dw = static_cast<float>(nw) * s;
  const float dh = static_cast<float>(nh) * s;
  if (p->has_sprite(spr))
    p->draw_sprite(spr, sx - dw * 0.5f, sy - dh * 0.5f, dw, dh, 0.f, 1.f);
  else
    p->draw_rect(sx - 8, sy - 8, 16, 16, 0.5f, 0.45f, 0.3f, 0.9f);
}

} // namespace

Game::Game()
  : platform_(0), audio_(0), state_(AppState_Loading), quit_(false),
    state_timer_(0.f), attack_cd_(0.f), reload_cd_(0.f),
    player_anim_t_(0.f), melee_anim_t_(0.f), use_anim_t_(0.f),
    airdrop_alert_(0.f), radio_t_(0.f),
    torch_battery_(100.f), inv_full_flash_(0.f), tent_warmth_t_(0.f), indoor_t_(0.f),
    footstep_t_(0.f),
    wetness_(0.f), gasmask_filter_(100.f), adrenaline_t_(0.f), melee_break_mul_(1.f), has_headlamp_(false),
    gun_jammed_(false), mag_rounds_(0), mag_weapon_id_(0),
    equipped_slot_(0), in_bunker_(false), bunker_looted_(false),
    show_bag_(false), menu_difficulty_(Difficulty_Novice),
    trap_count_(0), crop_count_(0), howl_cd_(0.f), cough_cd_(0.f), tree_count_(0),
    blood_drip_count_(0), bleed_drip_cd_(0.f), crouching_(false), fuel_warn_cd_(0.f),
    rain_sfx_cd_(0.f), lightning_t_(0.f), thunder_cd_(0.f), cough_alert_t_(0.f),
    wind_sfx_cd_(0.f), flare_t_(0.f), fx_count_(0), corpse_count_(0) {
  flare_pos_ = Vec2();
  data_dir_[0] = 0;
  last_airdrop_pos_ = Vec2();
  for (int i = 0; i < kMapSectorsX * kMapSectorsY; ++i) visited_sectors_[i] = false;
  prev_scream_ = false;
  // Members not covered by the init list (were read uninitialised before 2026-10-19).
  fire_anim_t_ = 0.f; energy_sprint_t_ = 0.f; vitamins_buff_t_ = 0.f; item_use_lock_t_ = 0.f;
  bunker_reopen_t_ = 0.f; show_craft_menu_ = false; show_controls_ = false; craft_menu_index_ = 0;
  hit_effect_t_ = 0.f; fps_ = 0.f; fps_accum_ = 0.f; fps_frames_ = 0;
  molotov_dmg_cd_ = 0.f; molotov_reload_t_ = 0.f; vss_bleed_unlock_cd_ = 0.f;
  wpn_unlock_toast_t_ = 0.f; wpn_unlock_toast_id_ = 0;
  map_prop_count_ = 0; quest_bandits_spawned_ = false; secret_spawn3_done_ = false;
  hammerspawned_ = 0; helispawned_ = 0; location_reload_anchor_ = Vec2();
  overworld_pos_ = Vec2();
  menu_focus_ = 0; pause_focus_ = 0; ui_t_ = 0.f; toast_[0] = 0; toast_t_ = 0.f;
  has_save_cache_ = -1; half_res_ = false; perf_overlay_ = false;
  for (int i = 0; i < 4; ++i) { nav_hold_t_[i] = 0.f; nav_prev_[i] = false; }
  for (int i = 0; i < kMaxTraps; ++i) traps_[i].active = false;
  for (int i = 0; i < kMaxFx; ++i) fx_[i].active = false;
  for (int i = 0; i < kMaxCorpses; ++i) corpses_[i].active = false;
}

int Game::ammo_for_equipped() const {
  const InvSlot& eq = inventory_.slot(equipped_slot_);
  if (eq.empty()) return -1;
  const WeaponDef* w = find_weapon(eq.id);
  if (!w || w->stats.is_melee || w->ammo_id <= 0) return -1;
  int n = 0;
  for (int i = 0; i < inventory_.slot_count(); ++i) {
    if (inventory_.slot(i).id == w->ammo_id) n += inventory_.slot(i).count;
  }
  return n;
}

int Game::mag_capacity_equipped() const {
  const InvSlot& eq = inventory_.slot(equipped_slot_);
  if (eq.empty()) return 0;
  const WeaponDef* w = find_weapon(eq.id);
  if (!w || w->stats.is_melee) return 0;
  int cap = w->mag_capacity;
  if (cap <= 0) return 0;
  // Magpull (89) — larger mag for magazine-fed guns
  for (int i = 0; i < inventory_.slot_count(); ++i) {
    if (inventory_.slot(i).id == 89) {
      cap += (cap >= 20) ? 10 : 2;
      break;
    }
  }
  return cap;
}

void Game::sync_mag_for_equipped() {
  const InvSlot& eq = inventory_.slot(equipped_slot_);
  ItemId wid = eq.empty() ? 0 : eq.id;
  if (wid != mag_weapon_id_) {
    mag_weapon_id_ = wid;
    mag_rounds_ = 0;
    const WeaponDef* w = find_weapon(wid);
    // Fresh equip: start empty so V reload matters (bows feed from bag)
    if (w && w->mag_capacity <= 0) mag_rounds_ = 0;
  }
}

bool Game::sector_visited(int sx, int sy) const {
  if (sx < 0 || sy < 0 || sx >= kMapSectorsX || sy >= kMapSectorsY) return false;
  return visited_sectors_[sy * kMapSectorsX + sx];
}

void Game::log_helicrash_coords() {
  if (!platform_) return;
  int n = 0;
  char line[128];
  for (int i = 0; i < mapgen_.location_count(); ++i) {
    const MapLocation& L = mapgen_.location(i);
    if (L.type != Loc_Helicrash) continue;
    std::snprintf(line, sizeof(line), "helicrash #%d at world (%.0f, %.0f) sector (%d,%d)",
                  n, L.pos.x, L.pos.y, L.sector_x, L.sector_y);
    platform_->log_line(line);
    ++n;
  }
  std::snprintf(line, sizeof(line), "new game: %d helicrash(es), %d locations", n, mapgen_.location_count());
  platform_->log_line(line);
}

// C2: zed/wolf/dog/deer/bandit movers are Bullet behaviours with "Bounce off solids" = 1
// (data.js instance props: attack/run/Bullet/Run = [speed,0,0,1,1,..]) and tilemap_forest is
// Solid, so NPCs cannot enter forest tiles. Port: same per-tile polygons as the player; the
// move is undone per axis (slide) instead of reflecting the Bullet angle.
bool Game::npc_forest_block(Vec2& pos, Vec2 prev) const {
  if (!world_tiles_.forest_solid_at(pos.x, pos.y)) return false;
  if (world_tiles_.forest_solid_at(prev.x, prev.y)) return false; // spawned inside: let it walk out
  if (!world_tiles_.forest_solid_at(pos.x, prev.y)) pos.y = prev.y;
  else if (!world_tiles_.forest_solid_at(prev.x, pos.y)) pos.x = prev.x;
  else pos = prev;
  return true;
}

void Game::mark_sector_visited() {
  const SectorCoord c = world_.sector_at(player_.pos);
  // Reveal a 3×3 neighbourhood (original fog clears nearby cells as you walk).
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      const int sx = c.sx + dx;
      const int sy = c.sy + dy;
      if (sx < 0 || sy < 0 || sx >= kMapSectorsX || sy >= kMapSectorsY) continue;
      visited_sectors_[sy * kMapSectorsX + sx] = true;
    }
  }
}

bool Game::init(IPlatform* platform, const char* data_dir) {
  platform_ = platform;
  if (data_dir) {
    std::strncpy(data_dir_, data_dir, sizeof(data_dir_) - 1);
    data_dir_[sizeof(data_dir_) - 1] = 0;
  } else {
    std::strcpy(data_dir_, "data");
  }

  char path[600];
  std::snprintf(path, sizeof(path), "%s/items.json", data_dir_);
  if (!items_.load_from_json_file(path)) {
    const char* fallback =
      "{\"items\":[{\"id\":1,\"name\":\"Canned beans\"},{\"id\":18,\"name\":\"Bandage\"},"
      "{\"id\":8,\"name\":\"Pipsi\"},{\"id\":54,\"name\":\"Water bottle\"},"
      "{\"id\":204,\"name\":\"Fire axe\"},{\"id\":151,\"name\":\"FNX pistol\"},"
      "{\"id\":12,\"name\":\".45 ACP ammo\"}]}";
    items_.load_from_json_string(fallback);
  }

  std::snprintf(path, sizeof(path), "%s/bunker_json", data_dir_);
  bunker_.load_from_file(path);

  std::snprintf(path, sizeof(path), "%s/world_tilemaps.txt", data_dir_);
  if (!world_tiles_.load(*platform_, path))
    std::fprintf(stderr, "[mdaz] world tilemaps missing: %s\n", path);
  std::snprintf(path, sizeof(path), "%s/menu_tilemaps.txt", data_dir_);
  menu_tiles_.load(*platform_, path);
  std::snprintf(path, sizeof(path), "%s/menu_sprites.txt", data_dir_);
  menu_sprites_.load(*platform_, path);
  std::snprintf(path, sizeof(path), "%s/char_anims.txt", data_dir_);
  if (!char_anims_.load(*platform_, path))
    std::fprintf(stderr, "[mdaz] char_anims missing: %s (using UV fallback)\n", path);

  // Original 1.4.1 Construct sprites.
  // PSP: data/sprites.pak (+ absolute ms0 fallbacks). Desktop: original_docs/images.
  {
    bool loaded = false;
#if defined(MDAZ_PSP)
    static const char* kPakCandidates[] = {
      "data/sprites.pak",
      "sprites.pak",
      "ms0:/PSP/GAME/MINIDAYZ/data/sprites.pak",
      "ms0:/PSP/GAME/minidayz/data/sprites.pak",
      "ms0:/PSP/GAME/MiniDAYZ/data/sprites.pak",
      "host0:/data/sprites.pak",
      "umd0:/data/sprites.pak",
      0
    };
    for (int i = 0; kPakCandidates[i]; ++i) {
      if (platform_->load_sprites(kPakCandidates[i])) {
        loaded = true;
        break;
      }
    }
    if (!loaded) {
      char pak[600];
      std::snprintf(pak, sizeof(pak), "%s/sprites.pak", data_dir_);
      if (platform_->load_sprites(pak)) loaded = true;
    }
#else
    static const char* kImgCandidates[] = {
      "C:/Users/abdur/Desktop/MiniDayZ/original_docs/images",
      "../../original_docs/images",
      "../original_docs/images",
      "original_docs/images",
      "../../../original_docs/images",
      0
    };
    for (int i = 0; kImgCandidates[i]; ++i) {
      if (platform_->load_sprites(kImgCandidates[i])) {
        loaded = true;
        break;
      }
    }
    if (!loaded) {
      char img[600];
      std::snprintf(img, sizeof(img), "%s/../../original_docs/images", data_dir_);
      if (platform_->load_sprites(img)) loaded = true;
    }
#endif
    if (!loaded)
      std::printf("[mdaz] WARNING: original sprites not found — colored rects fallback\n");
  }

  load_settings();
  state_ = AppState_Menu;
  state_timer_ = 0.f;
  return true;
}

// settings.cfg: "half_res=0|1" (PSP half-resolution world render; default OFF).
void Game::load_settings() {
  half_res_ = false;
  char* data = 0;
  std::size_t size = 0;
  if (platform_ && platform_->read_file("settings.cfg", &data, &size) && data) {
    const char* k = std::strstr(data, "half_res=");
    if (k && k + 9 < data + size) half_res_ = (k[9] == '1');
    const char* o = std::strstr(data, "perf_overlay=");
    if (o && o + 13 < data + size) perf_overlay_ = (o[13] == '1');
    delete[] data;
  }
  if (platform_ && platform_->supports_half_res()) platform_->set_half_res(half_res_);
  if (platform_ && platform_->supports_perf_overlay()) platform_->set_perf_overlay(perf_overlay_);
}

void Game::save_settings() {
  if (!platform_) return;
  char buf[64];
  const int n = std::snprintf(buf, sizeof(buf), "half_res=%d\nperf_overlay=%d\n", half_res_ ? 1 : 0,
                              perf_overlay_ ? 1 : 0);
  platform_->write_file("settings.cfg", buf, (std::size_t)n);
}

void Game::toggle_half_res() {
  if (!platform_ || !platform_->supports_half_res()) return;
  half_res_ = !half_res_;
  platform_->set_half_res(half_res_);
  save_settings();
  show_toast(half_res_ ? "HALF RES: ON" : "HALF RES: OFF");
}

void Game::toggle_perf_overlay() {
  if (!platform_ || !platform_->supports_perf_overlay()) return;
  perf_overlay_ = !perf_overlay_;
  platform_->set_perf_overlay(perf_overlay_);
  save_settings();
  show_toast(perf_overlay_ ? "MS OVERLAY: ON" : "MS OVERLAY: OFF");
}

void Game::shutdown() {}

void Game::start_new_game(Difficulty d) {
  player_.reset_new_game(d);
  set_npc_damage_for_difficulty(d);
  // Free starter perks by difficulty (scaffold — original perk shop not ported)
  if (d == Difficulty_Novice) {
    player_.perk_metabolism = true;
    player_.perk_hamster = true;
  } else if (d == Difficulty_Regular) {
    player_.perk_metabolism = true;
    player_.perk_farmer = true;
  } else {
    player_.perk_runner = true; // Veteran: speed over comfort
    player_.perk_hunter = true;
  }
  inventory_.clear();
  // Approximate 1.4.1-style survival start (exact C2 seed table not extracted —
  // see KNOWN_ISSUES). No free military guns/armor.
  inventory_.add_item(1, 1, items_);   // canned beans
  inventory_.add_item(6, 1, items_);   // apple
  inventory_.add_item(18, 1, items_);  // bandage
  inventory_.add_item(54, 1, items_);  // waterbottle
  inventory_.add_item(42, 1, items_);  // hunter knife
  inventory_.add_item(26, 2, items_);  // rags
  inventory_.add_item(38, 3, items_);  // sticks
  inventory_.add_item(24, 2, items_);  // matches
  inventory_.add_item(350, 1, items_); // t-shirt
  inventory_.add_item(300, 1, items_); // jeans
  if (d == Difficulty_Novice) {
    inventory_.add_item(18, 1, items_);  // extra bandage
    inventory_.add_item(202, 1, items_); // hatchet
    inventory_.add_item(453, 1, items_); // school backpack
  } else if (d == Difficulty_Regular) {
    inventory_.add_item(202, 1, items_); // hatchet
  } else {
    // Veteran: leaner — knife + food only extras already given
    inventory_.add_item(38, 2, items_);
  }
  equipped_slot_ = 0;
  for (int i = 0; i < inventory_.slot_count(); ++i) {
    if (inventory_.slot(i).id == 202 || inventory_.slot(i).id == 42) {
      equipped_slot_ = i;
      break;
    }
  }
  for (int i = 0; i < inventory_.slot_count(); ++i) {
    const ItemId wid = inventory_.slot(i).id;
    if (wid == 350 || wid == 300 || wid == 453)
      inventory_.wear_clothing(i, items_);
  }
  mag_weapon_id_ = 0;
  mag_rounds_ = 0;
  adrenaline_t_ = 0.f;
  melee_break_mul_ = 1.f;
  has_headlamp_ = false;
  gun_jammed_ = false;
  sync_mag_for_equipped();

  world_.reset();
  weather_.reset();
  zombies_.clear();
  animals_.clear();
  bandits_.clear();
  vehicles_.clear();
  campfires_.clear();
  loot_.clear();
  bullets_.clear();
  trap_count_ = 0;
  crop_count_ = 0;
  for (int i = 0; i < kMaxCrops; ++i) crops_[i].active = false;
  howl_cd_ = 0.f;
  cough_cd_ = 0.f;
  blood_drip_count_ = 0;
  bleed_drip_cd_ = 0.f;
  fx_count_ = 0;
  corpse_count_ = 0;
  for (int i = 0; i < kMaxTraps; ++i) traps_[i].active = false;
  for (int i = 0; i < kMaxFx; ++i) fx_[i].active = false;
  for (int i = 0; i < kMaxCorpses; ++i) corpses_[i].active = false;
  quest_.reset();
  quest_.generate_quest(rng_, mapgen_, 1);
  timers_.reset();
  dispersion_.reset_from_globals();
  attack_cd_ = 0.f;
  airdrop_alert_ = 0.f;
  radio_t_ = 0.f;
  torch_battery_ = 100.f;
  inv_full_flash_ = 0.f;
  tent_warmth_t_ = 0.f;
  indoor_t_ = 0.f;
  footstep_t_ = 0.f;
  wetness_ = 0.f;
  gasmask_filter_ = 100.f;
  for (int i = 0; i < kMapSectorsX * kMapSectorsY; ++i) visited_sectors_[i] = false;
  rng_.seed(140u);
  mapgen_.generate_new_game(140);
  mapgen_.apply_collision(world_);
  world_tiles_.stamp_from_mapgen(mapgen_);
  // Scatter forest tree stubs around POIs (denser near villages / church / secret)
  tree_count_ = 0;
  for (int i = 0; i < mapgen_.location_count() && tree_count_ < kMaxTrees; ++i) {
    const MapLocation& L = mapgen_.location(i);
    int n = 0;
    if (L.type == Loc_Village || L.type == Loc_Secret || L.type == Loc_Church) n = 10;
    else if (L.type == Loc_City) n = 6;
    else if (L.type == Loc_Military || L.type == Loc_Helicrash) n = 4;
    else if (L.type == Loc_Hospital || L.type == Loc_Police || L.type == Loc_Firestation) n = 3;
    for (int t = 0; t < n && tree_count_ < kMaxTrees; ++t) {
      trees_[tree_count_++] = Vec2(L.pos.x + rng_.range(-180.f, 180.f),
                                   L.pos.y + rng_.range(-180.f, 180.f));
    }
  }
  {
    Vec2 car = Vec2(player_.pos.x + 900.f, player_.pos.y);
    for (int i = 0; i < mapgen_.location_count(); ++i) {
      const MapLocation& L = mapgen_.location(i);
      if (L.type == Loc_GasStation || L.type == Loc_City) {
        car = Vec2(L.pos.x - 60.f, L.pos.y + 50.f);
        break;
      }
    }
    vehicles_.spawn(Vehicle_Car, car);
  }
  // Boat near map-edge water band
  vehicles_.spawn(Vehicle_Boat, Vec2(180.f, player_.pos.y));
  // Bike near first village if any, else beside player
  {
    Vec2 bike = Vec2(player_.pos.x + 40.f, player_.pos.y + 70.f);
    for (int i = 0; i < mapgen_.location_count(); ++i) {
      if (mapgen_.location(i).type == Loc_Village) {
        bike = Vec2(mapgen_.location(i).pos.x + 30.f, mapgen_.location(i).pos.y - 20.f);
        break;
      }
    }
    vehicles_.spawn(Vehicle_Bike, bike);
  }
  mark_sector_visited();
  log_helicrash_coords();

  spawn_poi_loot();
  // Extra hostiles around POIs — Zed_per_base (5) as density budget per site
  const int zed_budget = static_cast<int>(balance::Zed_per_base);
  const Vec2 spawn_pos = player_.pos;
  for (int i = 0; i < mapgen_.location_count(); ++i) {
    const MapLocation& L = mapgen_.location(i);
    if (dist2(L.pos, spawn_pos) < kSpawnSafeRadius * kSpawnSafeRadius) continue;
    if (L.type == Loc_Military) {
      for (int k = 0; k < zed_budget; ++k) {
        const ZedType zt = (k % 3 == 0) ? Zed_ArmyShooter
                           : (k % 3 == 1) ? Zed_Army : Zed_Normal;
        zombies_.spawn(zt, Vec2(L.pos.x + rng_.range(-55.f, 55.f),
                                L.pos.y + rng_.range(-55.f, 55.f)));
      }
      bandits_.spawn(Vec2(L.pos.x + 60.f, L.pos.y - 30.f), true);
    } else if (L.type == Loc_City || L.type == Loc_Village) {
      for (int k = 0; k < zed_budget; ++k) {
        const ZedType zt = (rng_.next_float() < 0.2f) ? Zed_Buried : Zed_Normal;
        zombies_.spawn(zt, Vec2(L.pos.x + rng_.range(-50.f, 50.f),
                                L.pos.y + rng_.range(-50.f, 50.f)));
      }
    } else if (L.type == Loc_Hospital) {
      for (int k = 0; k < zed_budget - 1; ++k)
        zombies_.spawn(Zed_Fast, Vec2(L.pos.x + rng_.range(-40.f, 40.f),
                                      L.pos.y + rng_.range(-40.f, 40.f)));
    } else if (L.type == Loc_Police || L.type == Loc_Firestation) {
      for (int k = 0; k < zed_budget - 2; ++k)
        zombies_.spawn(Zed_Normal, Vec2(L.pos.x + rng_.range(-40.f, 40.f),
                                        L.pos.y + rng_.range(-40.f, 40.f)));
    } else if (L.type == Loc_Church) {
      zombies_.spawn(Zed_Normal, Vec2(L.pos.x + 15.f, L.pos.y));
      zombies_.spawn(Zed_Buried, Vec2(L.pos.x - 20.f, L.pos.y + 10.f));
    }
  }
  in_bunker_ = false;
  bunker_looted_ = false;

  // Starter craft materials so craft path is playable
  inventory_.add_item(26, 2, items_); // rags
  inventory_.add_item(38, 5, items_); // wood sticks (arrows craft)
  inventory_.add_item(37, 1, items_); // ashwood
  inventory_.add_item(41, 2, items_); // rope (bow + bag)
  inventory_.add_item(40, 1, items_); // burlap sack
  inventory_.add_item(42, 1, items_); // hunter knife (arrow tool)
  inventory_.add_item(63, 1, items_); // energy drink
  inventory_.add_item(39, 2, items_); // duct tape
  inventory_.add_item(87, 1, items_); // fishing rod
  inventory_.add_item(36, 1, items_); // cleaning kit
  inventory_.add_item(35, 1, items_); // sewing kit
  inventory_.add_item(34, 1, items_); // heatpack
  inventory_.add_item(46, 1, items_); // tetracycline
  inventory_.add_item(354, 1, items_); // Jacket — equip with C for insulation
  inventory_.add_item(300, 1, items_); // Jeans
  inventory_.add_item(353, 1, items_); // Raincoat
  inventory_.add_item(24, 3, items_);  // Matches
  inventory_.add_item(23, 1, items_);  // Campfire kit
  inventory_.add_item(50, 2, items_);  // Wood piles
  inventory_.add_item(24, 2, items_);  // Matches
  inventory_.add_item(60, 1, items_);  // Raw steak to cook at fire
  inventory_.add_item(53, 1, items_);  // Gasoline for car
  inventory_.add_item(59, 1, items_);  // Car toolbox
  inventory_.add_item(19, 1, items_);  // Blood bag
  inventory_.add_item(28, 1, items_);  // Saline
  inventory_.add_item(45, 1, items_);  // Morphine
  inventory_.add_item(20, 2, items_);  // Flares
  inventory_.add_item(265, 1, items_); // NVG
  inventory_.add_item(21, 2, items_);  // F1 grenades
  inventory_.add_item(85, 1, items_);  // MRE
  inventory_.add_item(22, 2, items_);  // Batteries for torch
  inventory_.add_item(353, 1, items_); // Raincoat
  inventory_.add_item(456, 1, items_); // Civilian tent
  inventory_.add_item(102, 1, items_); // Molotov
  inventory_.add_item(250, 1, items_); // Gasmask

  state_ = AppState_Playing;
  if (audio_) audio_->play("ambient", 0.25f);
}

void Game::spawn_poi_loot() {
  for (int i = 0; i < mapgen_.location_count(); ++i) {
    const MapLocation& L = mapgen_.location(i);
    int piles = 1;
    if (L.type == Loc_Military || L.type == Loc_Helicrash) piles = 3;
    else if (L.type == Loc_Hospital || L.type == Loc_City || L.type == Loc_Secret ||
             L.type == Loc_Police || L.type == Loc_Church || L.type == Loc_Firestation)
      piles = 2;
    for (int n = 0; n < piles; ++n) {
      const float ox = rng_.range(-40.f, 40.f);
      const float oy = rng_.range(-40.f, 40.f);
      const Vec2 p(L.pos.x + ox, L.pos.y + oy);
      switch (L.type) {
      case Loc_Military:
        loot_.spawn_military(p, rng_);
        break;
      case Loc_Helicrash:
        loot_.spawn_helicrash(p, rng_);
        break;
      case Loc_Hospital:
        loot_.spawn_hospital(p, rng_);
        break;
      case Loc_GasStation:
        loot_.spawn_gas_station(p, rng_);
        break;
      case Loc_City:
      case Loc_Village:
        loot_.spawn_civilian(p, rng_);
        break;
      case Loc_Firestation:
        loot_.spawn_firestation(p, rng_);
        break;
      case Loc_Police:
        loot_.spawn_police(p, rng_);
        break;
      case Loc_Church:
        loot_.spawn_church(p, rng_);
        break;
      case Loc_Secret:
        loot_.spawn_secret(p, rng_);
        break;
      default:
        loot_.spawn_random(p, rng_);
        break;
      }
    }
  }
}

void Game::apply_incoming_damage(float amount, bool can_bleed, PlayerDamageKind kind,
                                 const char* death_if_lethal,
                                 MeleeDefPercentTier melee_def_tier, bool try_melee_def) {
  (void)kind; (void)death_if_lethal; (void)melee_def_tier; (void)try_melee_def;
  if (amount <= 0.f) return;
  // Worn helmet/vest stub mitigation (full C2 armor curves not extracted)
  amount *= inventory_.armor_damage_mul();
  if (amount < 0.5f) amount = 0.5f;
  // Player_get_hit (decoded): SubInstanceVar IV15 (blood) by damage param;
  // bleed start: Compare random(3) >= 1.5 → ~50%, then StartTimer("Bleed", 60) + IV22=1.
  player_.survival.blood -= amount;
  if (player_.survival.blood < 0.f) player_.survival.blood = 0.f;
  if (player_.survival.blood <= 0.f && !player_.survival.death_reason[0]) {
    std::strncpy(player_.survival.death_reason, "Zombie", sizeof(player_.survival.death_reason) - 1);
  }
  if (can_bleed && player_.survival.bleeding <= 0.f) {
    // random(3) in C2 is [0,3); P(>=1.5) ≈ 0.5
    if (rng_.next_float() * 3.f >= 1.5f) {
      player_.survival.bleeding = 1.f;
    }
  }
  // Infection chance stub (full C2 disease group not ported); gasmask+filter helps
  if (can_bleed && !player_.survival.infected) {
    const bool gasmask = wearing_gasmask() && gasmask_filter_ > 5.f;
    const float chance = gasmask ? 0.015f : 0.08f;
    if (rng_.next_float() < chance) player_.survival.infected = true;
  }
  if (audio_ && amount > 0.f) audio_->play("hit", 0.55f);
}

bool Game::wearing_gasmask() const {
  for (int wi = 0; wi < inventory_.worn_count(); ++wi) {
    if (inventory_.worn_at(wi) == 250) return true;
  }
  return false;
}

bool Game::near_choppable_tree() const {
  for (int i = 0; i < tree_count_; ++i) {
    const float dx = trees_[i].x - player_.pos.x;
    const float dy = trees_[i].y - player_.pos.y;
    if (dx * dx + dy * dy < 55.f * 55.f) return true;
  }
  return false;
}

bool Game::near_vehicle() const {
  for (int i = 0; i < vehicles_.count(); ++i) {
    const Vehicle* v = vehicles_.at(i);
    if (!v || !v->active) continue;
    const float dx = v->pos.x - player_.pos.x;
    const float dy = v->pos.y - player_.pos.y;
    if (dx * dx + dy * dy < 50.f * 50.f) return true;
  }
  return false;
}

void Game::try_melee_attack() {
  WeaponStats stats = default_melee_stats();
  const InvSlot& eq = inventory_.slot(equipped_slot_);
  bool using_melee_item = false;
  if (!eq.empty()) {
    const WeaponDef* w = find_weapon(eq.id);
    if (w && w->stats.is_melee) {
      stats = w->stats;
      using_melee_item = true;
    }
  }
  attack_cd_ = stats.fire_interval;
  melee_anim_t_ = 0.35f; // C2 axe_* ~3 frames @ speed 10
  if (audio_) audio_->play_at("melee_swing", player_.pos.x, player_.pos.y, 0.6f);

  bool hit_something = false;
  for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
    if (!z->alive) continue;
    if (dist2(z->pos, player_.pos) <= stats.range * stats.range) {
      hurt_zombie(*z, stats.damage, equipped_id_or_zero());
      hit_something = true;
    }
  }
  for (int i = 0; i < animals_.count(); ++i) {
    Animal* a = animals_.at(i);
    if (!a->alive) continue;
    if (dist2(a->pos, player_.pos) <= stats.range * stats.range) {
      hurt_animal(*a, stats.damage);
      hit_something = true;
    }
  }
  for (Bandit* b = bandits_.begin(); b != bandits_.end(); ++b) {
    if (!b->alive) continue;
    if (dist2(b->pos, player_.pos) <= stats.range * stats.range) {
      hurt_bandit(*b, stats.damage);
      hit_something = true;
    }
  }
  // Melee durability break: no C2 string/event in 1.4.1 export — not invented.
  (void)hit_something;
  (void)using_melee_item;
}

void Game::try_ranged_attack() {
  if (reload_cd_ > 0.f) return;
  sync_mag_for_equipped();
  const InvSlot& eq = inventory_.slot(equipped_slot_);
  if (eq.empty()) return;
  const WeaponDef* w = find_weapon(eq.id);
  if (!w || w->stats.is_melee) return;
  // gun_jammed_: no "jam"/"misfire" events in 1.4.1 export — flag kept for save compat only.
  gun_jammed_ = false;

  if (w->ammo_id > 0) {
    if (w->mag_capacity > 0) {
      // Chambered magazine
      if (mag_rounds_ <= 0) {
        if (audio_) audio_->play("click", 0.55f); // dry fire — press V
        attack_cd_ = 0.25f;
        return;
      }
      mag_rounds_ -= 1;
    } else {
      // Bow / no-mag: spend from inventory
      bool spent = false;
      for (int i = 0; i < inventory_.slot_count(); ++i) {
        if (inventory_.slot(i).id == w->ammo_id && inventory_.slot(i).count > 0) {
          inventory_.remove_at(i, 1);
          spent = true;
          break;
        }
      }
      if (!spent) {
        if (audio_) audio_->play("melee_swing", 0.25f);
        attack_cd_ = 0.2f;
        return;
      }
    }
  }

  attack_cd_ = w->stats.fire_interval;
  // Silencer 5.45/5.56 (80/81) only match ammo family (14/15 ≈5.45/7.62x39; 11=5.56)
  bool silenced = false;
  {
    const ItemId ammo = w->ammo_id;
    bool has_545 = false, has_556 = false;
    for (int i = 0; i < inventory_.slot_count(); ++i) {
      const ItemId sid = inventory_.slot(i).id;
      if (sid == 80) has_545 = true;
      if (sid == 81) has_556 = true;
    }
    if (has_545 && (ammo == 15 || ammo == 14)) silenced = true;
    if (has_556 && ammo == 11) silenced = true;
  }
  if (audio_) {
    const char* shot = "gunshot";
    if (silenced) shot = "gunshot_silent";
    else {
      const ItemId wid = inventory_.slot(equipped_slot_).id;
      if (wid == 159 || wid == 174) shot = "gun_akm";
      else if (wid == 160 || wid == 161) shot = "gun_ak74";
      else if (wid == 151) shot = "gun_fnx";
      else if (wid == 152 || wid == 176) shot = "gun_colt";
      else if (wid == 170) shot = "gun_glock";
      else if (wid == 163 || wid == 164) shot = "gun_bow";
      else if (wid == 154 || wid == 155 || wid == 171) shot = "gun_shotgun";
      else if (wid == 178) shot = "gun_saiga";
      else if (wid == 156 || wid == 172) shot = "gun_mosin";
      else if (wid == 158 || wid == 162 || wid == 177) shot = "gun_m4";
      else if (wid == 169 || wid == 173 || wid == 179) shot = "gun_mp5";
      else if (wid == 168 || wid == 186) shot = "gun_svd";
    }
    audio_->play_at(shot, player_.pos.x, player_.pos.y, silenced ? 0.45f : 0.8f);
  }
  {
    // Player_Hear_radius = 1000 (globals); gunshot pull uses a fraction; silencer quieter
    const float hear_r = silenced ? 90.f
                                  : static_cast<float>(balance::Player_Hear_radius) * 0.42f;
    const float boost = silenced ? 260.f
                                 : static_cast<float>(balance::Player_Hear_radius) * 0.45f;
    for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
      if (!z->alive) continue;
      if (dist2(z->pos, player_.pos) > hear_r * hear_r) continue;
      if (z->aggro_radius < boost) z->aggro_radius = boost;
    }
    // Loud shots stampede deer/rabbits; silenced barely scares
    const float scare_r = silenced ? 120.f : 480.f;
    bool deer_ran = false;
    for (int ai = 0; ai < animals_.count(); ++ai) {
      Animal* a = animals_.at(ai);
      if (!a->alive) continue;
      if (a->type != Animal_Deer && a->type != Animal_Rabbit && a->type != Animal_Crow) continue;
      if (dist2(a->pos, player_.pos) > scare_r * scare_r) continue;
      if (a->type == Animal_Deer) deer_ran = true;
      const float dx = a->pos.x - player_.pos.x;
      const float dy = a->pos.y - player_.pos.y;
      const float len = std::sqrt(dx * dx + dy * dy);
      if (len > 1.f) {
        const float push = silenced ? 40.f : 110.f;
        a->pos.x += (dx / len) * push;
        a->pos.y += (dy / len) * push;
      }
    }
    if (deer_ran && audio_ && !silenced) audio_->play("deer", 0.4f);
  }
  dispersion_.on_shot(player_.running && !crouching_);
  float ang_off = dispersion_.sample_offset_deg(rng_);
  if (crouching_) ang_off *= 0.55f; // braced aim
  // Optics in bag tighten aim (best wins)
  float scope_mul = 1.f;
  for (int i = 0; i < inventory_.slot_count(); ++i) {
    const ItemId sid = inventory_.slot(i).id;
    float m = 1.f;
    if (sid == 98) m = 0.28f;      // LR / sniper
    else if (sid == 79) m = 0.32f; // ACOG
    else if (sid == 78) m = 0.36f; // PSO-1
    else if (sid == 77) m = 0.40f; // PU
    else if (sid == 76) m = 0.45f; // RDS
    if (m < scope_mul) scope_mul = m;
  }
  ang_off *= scope_mul;
  // Choke (90) tightens shotgun spread (ammo 17)
  if (w->ammo_id == 17) {
    for (int i = 0; i < inventory_.slot_count(); ++i) {
      if (inventory_.slot(i).id == 90) { ang_off *= 0.55f; break; }
    }
  }
  const float ang = player_.facing_deg + ang_off;
  const float dmg = roll_bullet_damage(w->stats.damage, rng_);
  const bool bow = (w->item_id == 163 || w->item_id == 164);
  Bullet* shot = bullets_.spawn(player_.pos, ang, bow ? 380.f : 520.f, dmg,
                                w->stats.range / 520.f + 0.15f, true);
  if (shot && bow) shot->is_arrow = true;
  {
    const float rad = ang * 0.01745329252f;
    spawn_fx(Fx_Muzzle,
             Vec2(player_.pos.x + std::cos(rad) * 14.f,
                  player_.pos.y + std::sin(rad) * 14.f),
             ang, 0.08f);
    // Ejected casing drifts sideways briefly
    const float side = rad + 1.5707963f;
    spawn_fx(Fx_Shell,
             Vec2(player_.pos.x + std::cos(side) * 6.f,
                  player_.pos.y + std::sin(side) * 6.f),
             ang + 90.f, 0.35f);
  }
}

void Game::try_pickup_loot() {
  const float reach = 40.f;
  for (int i = 0; i < loot_.count(); ++i) {
    LootPile* p = loot_.at(i);
    if (!p->active) continue;
    if (dist2(p->pos, player_.pos) > reach * reach) continue;
    int remaining = 0;
    for (int j = 0; j < p->item_count; ++j) {
      if (p->counts[j] <= 0) continue;
      if (inventory_.add_item(p->items[j], p->counts[j], items_)) {
        p->counts[j] = 0;
      } else {
        ++remaining;
      }
    }
    if (remaining == 0) {
      p->active = false;
      p->item_count = 0;
    } else {
      // Compact remaining items in pile
      int w = 0;
      for (int j = 0; j < p->item_count; ++j) {
        if (p->counts[j] <= 0) continue;
        p->items[w] = p->items[j];
        p->counts[w] = p->counts[j];
        ++w;
      }
      p->item_count = w;
      inv_full_flash_ = 1.5f;
    }
    if (audio_) audio_->play("pickup", 0.5f);
    break;
  }
}

void Game::spawn_fx(FxKind kind, Vec2 pos, float angle_deg, float life) {
  FxSprite* slot = 0;
  for (int i = 0; i < fx_count_; ++i) {
    if (!fx_[i].active) { slot = &fx_[i]; break; }
  }
  if (!slot && fx_count_ < kMaxFx) slot = &fx_[fx_count_++];
  if (!slot) {
    // Steal oldest
    slot = &fx_[0];
    for (int i = 1; i < fx_count_; ++i)
      if (fx_[i].life < slot->life) slot = &fx_[i];
  }
  slot->pos = pos;
  slot->life = life;
  slot->angle = angle_deg;
  slot->kind = kind;
  slot->active = true;
}

void Game::spawn_corpse(CorpseKind kind, Vec2 pos, float angle_deg, int variant,
                        const char* anim_base, float die_anim_sec, int die_style) {
  Corpse* slot = 0;
  for (int i = 0; i < corpse_count_; ++i) {
    if (!corpses_[i].active) { slot = &corpses_[i]; break; }
  }
  if (!slot && corpse_count_ < kMaxCorpses) slot = &corpses_[corpse_count_++];
  if (!slot) {
    float worst = 1e9f;
    for (int i = 0; i < corpse_count_; ++i) {
      if (corpses_[i].life < worst) { worst = corpses_[i].life; slot = &corpses_[i]; }
    }
  }
  if (!slot) return;
  slot->pos = pos;
  // Bot_corpse_stay_time = 90 (globals); used for all corpse kinds as sverka base
  slot->life = static_cast<float>(balance::Bot_corpse_stay_time);
  if (kind == Corpse_Zed) slot->life *= 1.15f; // zeds linger slightly longer
  slot->angle = angle_deg;
  slot->kind = kind;
  slot->variant = variant;
  slot->die_anim_t = die_anim_sec;
  slot->die_style = die_style;
  slot->anim_base[0] = 0;
  if (anim_base) { std::strncpy(slot->anim_base, anim_base, sizeof(slot->anim_base) - 1); slot->anim_base[sizeof(slot->anim_base) - 1] = 0; }
  slot->active = true;
}

void Game::update_fx(float dt) {
  for (int i = 0; i < fx_count_; ++i) {
    if (!fx_[i].active) continue;
    if (fx_[i].kind == Fx_Shell) {
      const float rad = fx_[i].angle * 0.01745329252f;
      fx_[i].pos.x += std::cos(rad) * 28.f * dt;
      fx_[i].pos.y += std::sin(rad) * 28.f * dt + 12.f * dt; // arc down
      fx_[i].angle += 220.f * dt;
    } else if (fx_[i].kind == Fx_Blood) {
      fx_[i].pos.y += 8.f * dt;
    }
    fx_[i].life -= dt;
    if (fx_[i].life <= 0.f) fx_[i].active = false;
  }
  for (int i = 0; i < corpse_count_; ++i) {
    if (!corpses_[i].active) continue;
    if (corpses_[i].die_anim_t > 0.f) corpses_[i].die_anim_t -= dt;
    corpses_[i].life -= dt;
    if (corpses_[i].life <= 0.f) corpses_[i].active = false;
  }
}

// C2 NPC_get_damage + dmg_hint: subtract from the kind's HP IV, spawn the floating "-N"
// (obj529 Bullet aimed angle(player -> target), orange 255,196,68), kill at hp <= 0.
const char* Game::zed_skin_base(ZedType t) {
  switch (t) {
  case Zed_Fast: return "zed_fast_skin1";
  case Zed_Army: case Zed_ArmyShooter: return "zed_army_skin1";
  case Zed_Buried: return "zed_buried_skin";
  case Zed_Tank: return "zed_tank_skin";
  case Zed_Screamer: return "zed_screamer_skin";
  default: return "zed_normal_skin1";
  }
}

static Vec2 away_from(Vec2 at, Vec2 from) { return Vec2(at.x + (at.x - from.x), at.y + (at.y - from.y)); }

ItemId Game::equipped_id_or_zero() const {
  return inventory_.slot(equipped_slot_).count > 0 ? inventory_.slot(equipped_slot_).id : 0;
}

void Game::hurt_zombie(Zombie& z, float amount, ItemId killer_weapon) {
  if (!z.alive) return;
  const float applied = npc_apply_damage(z.hp, npc_damage_kind_for_zed(z.type), amount);
  dmg_hints_.spawn(z.pos, away_from(z.pos, player_.pos), static_cast<int>(applied + 0.5f), false);
  if (z.hp <= 0.f) on_zed_killed(z, killer_weapon);
}

void Game::hurt_zombie_bullet(Zombie& z, float amount, bool armor_pierce_iv8, ItemId killer_weapon) {
  if (!z.alive) return;
  const float applied = zed_apply_bullet_hit_damage(z.hp, npc_damage_kind_for_zed(z.type), amount,
                                                    z.type, armor_pierce_iv8, rng_);
  dmg_hints_.spawn(z.pos, away_from(z.pos, player_.pos), static_cast<int>(applied + 0.5f), false);
  if (z.hp <= 0.f) on_zed_killed(z, killer_weapon);
}

void Game::hurt_animal(Animal& a, float amount) {
  if (!a.alive) return;
  const float applied = npc_apply_damage(a.hp, npc_damage_kind_for_animal(a.type), amount);
  dmg_hints_.spawn(a.pos, away_from(a.pos, player_.pos), static_cast<int>(applied + 0.5f), false);
  if (a.hp <= 0.f) on_animal_killed(a);
}

void Game::hurt_bandit(Bandit& b, float amount) {
  if (!b.alive) return;
  const float applied = npc_apply_damage(b.hp, npc_damage_kind_for_bandit(b.has_gun), amount);
  dmg_hints_.spawn(b.pos, away_from(b.pos, player_.pos), static_cast<int>(applied + 0.5f), false);
  if (b.hp <= 0.f) {
    b.alive = false;
    player_.bandits_killed += 1;
    spawn_corpse(Corpse_Bandit, b.pos, 0.f, 0);
    spawn_fx(Fx_Hit, b.pos, 0.f, 0.2f);
    loot_.spawn_bandit(b.pos, rng_);
  } else {
    spawn_fx(Fx_Hit, b.pos, 0.f, 0.1f);
  }
}

void Game::on_animal_killed(Animal& a) {
  a.alive = false;
  player_.animals_killed += 1;
  if (player_.animals_killed >= 5) player_.perk_hunter = true;
  ItemId meat = 60; // Raw steak default (deer / large)
  int n = player_.perk_hunter ? 2 : 1;
  CorpseKind ck = Corpse_Other;
  if (a.type == Animal_Rabbit || a.type == Animal_Crow) {
    meat = 71; // Rabbit meat
  } else if (a.type == Animal_Deer) {
    n = player_.perk_hunter ? 3 : 2; // deer yields more
    ck = Corpse_Deer;
  } else if (a.type == Animal_Wolf) {
    meat = 60;
    n = player_.perk_hunter ? 2 : 1;
    ck = Corpse_Wolf;
  } else if (a.type == Animal_Dog) {
    meat = 60;
    ck = Corpse_Dog;
  }
  {
    // wolf_skin die1 (3 frames @ 8 fps) before wolf_dead; other animals have no die anim.
    const char* base = (a.type == Animal_Wolf) ? "wolf_skin" : 0;
    const CharAnim* da = base ? char_anims_.find(base, "die1") : 0;
    const float die_sec = (da && da->speed > 0) ? static_cast<float>(da->frame_count) / da->speed : 0.f;
    spawn_corpse(ck, a.pos, 0.f, 0, da ? base : 0, die_sec, 1);
  }
  spawn_fx(Fx_Hit, a.pos, 0.f, 0.2f);
  spawn_fx(Fx_Blood, a.pos, 0.f, 0.45f);
  loot_.spawn_item(a.pos, meat, n);
  // Crow: also drop sticks (feather substitute for fletching)
  if (a.type == Animal_Crow && rng_.next_float() < 0.6f) {
    loot_.spawn_item(Vec2(a.pos.x + 8.f, a.pos.y), 38, 1);
  }
}

void Game::on_zed_killed(Zombie& z, ItemId killer_weapon) {
  (void)killer_weapon;
  z.alive = false;
  player_.zeds_killed += 1;
  int variant = 0;
  if (z.type == Zed_Fast) variant = 1;
  else if (z.type == Zed_Army || z.type == Zed_ArmyShooter) variant = 2;
  else if (z.type == Zed_Buried) variant = 3;
  else if (z.type == Zed_Tank) variant = 4;
  else if (z.type == Zed_Screamer) variant = 5;
  // C2: the skin plays die1 or die2 (random pick when the skin has both; 8 fps, no loop),
  // then the *_dead sprite stays (Fade). Frames from data/char_anims.txt.
  const char* base = zed_skin_base(z.type);
  int style = 1;
  const CharAnim* d2 = char_anims_.find(base, "die2");
  if (d2 && rng_.next_float() < 0.5f) style = 2;
  const CharAnim* da = char_anims_.find(base, style == 2 ? "die2" : "die1");
  const float die_sec = (da && da->speed > 0) ? static_cast<float>(da->frame_count) / da->speed : 0.f;
  const float face = std::atan2(player_.pos.y - z.pos.y, player_.pos.x - z.pos.x) * 57.2957795f;
  spawn_corpse(Corpse_Zed, z.pos, 0.f, variant, da ? base : 0, die_sec, style);
  (void)face;
  spawn_fx(Fx_Hit, z.pos, 0.f, 0.22f);
  spawn_fx(Fx_Blood, z.pos, 0.f, 0.5f);
  loot_.spawn_random(z.pos, rng_);
  // Crows scavenge corpse
  if (rng_.next_float() < 0.45f) {
    const int n = 1 + rng_.next_int(0, 2);
    for (int i = 0; i < n; ++i) {
      animals_.spawn(Animal_Crow,
                     Vec2(z.pos.x + rng_.range(-40.f, 40.f),
                          z.pos.y + rng_.range(-40.f, 40.f)));
    }
  }
}

void Game::write_death_log() {
  if (!platform_) return;
  char line[160];
  std::snprintf(line, sizeof(line),
                "reason=%s days=%d zeds=%d bots=%d animals=%d blood=%.0f\n",
                player_.survival.death_reason[0] ? player_.survival.death_reason : "Unknown",
                player_.days_survived, player_.zeds_killed, player_.bandits_killed,
                player_.animals_killed, player_.survival.blood);
  platform_->write_file("last_death.txt", line, std::strlen(line));
}

void Game::explode_at(Vec2 pos, float damage, bool leave_fire) {
  const float r2 = 75.f * 75.f;
  if (audio_) audio_->play_at(leave_fire ? "explode" : "explode", pos.x, pos.y, 1.f);
  spawn_fx(Fx_Explode, pos, leave_fire ? 1.f : 0.f, 0.45f); // angle=1 marks molotov
  if (leave_fire) campfires_.place(pos, 25.f);
  for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
    if (!z->alive) continue;
    if (dist2(z->pos, pos) > r2) continue;
    hurt_zombie(*z, damage);
  }
  for (int i = 0; i < animals_.count(); ++i) {
    Animal* a = animals_.at(i);
    if (!a->alive) continue;
    if (dist2(a->pos, pos) > r2) continue;
    hurt_animal(*a, damage * 0.8f);
  }
  for (Bandit* bd = bandits_.begin(); bd != bandits_.end(); ++bd) {
    if (!bd->alive) continue;
    if (dist2(bd->pos, pos) > r2) continue;
    hurt_bandit(*bd, damage);
  }
}

void Game::resolve_bullet_hits() {
  for (Bullet* b = bullets_.begin(); b != bullets_.end(); ++b) {
    // Timed grenade fuse: explode when life expired this frame
    if (!b->alive && b->explosive && b->from_player && b->life <= 0.f) {
      explode_at(b->pos, b->damage, b->leaves_fire);
      b->explosive = false; // one-shot
      b->leaves_fire = false;
      continue;
    }
    if (!b->alive) continue;
    if (!b->from_player) {
      // Enemy bullet vs player (Player_get_hit "bullet" path)
      if (dist2(b->pos, player_.pos) < 14.f * 14.f) {
        apply_incoming_damage(b->damage, true);
        spawn_fx(Fx_Hit, player_.pos, 0.f, 0.15f);
        if (player_.survival.blood <= 0.f) {
          std::strncpy(player_.survival.death_reason, "Bullet",
                       sizeof(player_.survival.death_reason) - 1);
        }
        b->alive = false;
      }
      continue;
    }
    if (b->explosive) {
      // Detonate on contact with any target or wall (wall cleared earlier)
      bool hit = false;
      for (Zombie* z = zombies_.begin(); z != zombies_.end() && !hit; ++z) {
        if (z->alive && dist2(b->pos, z->pos) < 22.f * 22.f) hit = true;
      }
      for (int i = 0; i < animals_.count() && !hit; ++i) {
        Animal* a = animals_.at(i);
        if (a->alive && dist2(b->pos, a->pos) < 20.f * 20.f) hit = true;
      }
      for (Bandit* bd = bandits_.begin(); bd != bandits_.end() && !hit; ++bd) {
        if (bd->alive && dist2(b->pos, bd->pos) < 20.f * 20.f) hit = true;
      }
      if (hit) {
        explode_at(b->pos, b->damage, b->leaves_fire);
        b->alive = false;
        b->explosive = false;
        b->leaves_fire = false;
      }
      continue;
    }
    for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
      if (!z->alive) continue;
      if (dist2(b->pos, z->pos) < 18.f * 18.f) {
        spawn_fx(Fx_Hit, z->pos, 0.f, 0.12f);
        b->alive = false;
        hurt_zombie_bullet(*z, b->damage, b->armor_pierce_iv8, b->source_weapon);
        break;
      }
    }
    if (!b->alive) continue;
    for (int i = 0; i < animals_.count(); ++i) {
      Animal* a = animals_.at(i);
      if (!a->alive) continue;
      if (dist2(b->pos, a->pos) < 16.f * 16.f) {
        spawn_fx(Fx_Hit, a->pos, 0.f, 0.12f);
        b->alive = false;
        hurt_animal(*a, b->damage);
        break;
      }
    }
    if (!b->alive) continue;
    for (Bandit* bd = bandits_.begin(); bd != bandits_.end(); ++bd) {
      if (!bd->alive) continue;
      if (dist2(b->pos, bd->pos) < 16.f * 16.f) {
        b->alive = false;
        hurt_bandit(*bd, b->damage);
        break;
      }
    }
  }
}

void Game::try_craft() {
  const bool at_fire = campfires_.near_fire(player_.pos);
  // Cooking (meat_cook / pan): inventory Cook_* recipes removed — no verified Spawn_drop.
  (void)at_fire;
  for (int i = 0; i < craft_recipe_count(); ++i) {
    const CraftRecipe* r = craft_recipe_at(i);
    if (!r) continue;
    if (r->requires_fire && !at_fire) continue;
    if (craft_do(inventory_, *r, items_)) {
      if (audio_) audio_->play("craft", 0.7f);
      return;
    }
  }
}

void Game::try_drop_equipped() { try_drop_slot(equipped_slot_); }

void Game::try_drop_slot(int slot_idx) {
  if (slot_idx < 0 || slot_idx >= inventory_.slot_count()) return;
  // Near vehicle: G stashes into trunk, or retrieves if hotbar slot empty
  Vehicle* near_v = 0;
  for (int i = 0; i < vehicles_.count(); ++i) {
    Vehicle* v = vehicles_.at(i);
    if (!v || !v->active) continue;
    if (dist2(v->pos, player_.pos) < 48.f * 48.f) { near_v = v; break; }
  }
  InvSlot& s = inventory_.slot(slot_idx);
  if (near_v) {
    if (s.empty()) {
      ItemId id = 0;
      int n = 0;
      if (near_v->stash_take(id, n) && inventory_.add_item(id, n, items_)) {
        if (audio_) audio_->play("pickup", 0.5f);
        return;
      }
      // Stash take failed or inv full — fall through if had item
      if (id > 0) near_v->stash_add(id, n); // put back
      return;
    }
    const ItemId id = s.id;
    if (near_v->stash_add(id, 1) && inventory_.remove_at(slot_idx, 1)) {
      if (audio_) audio_->play("craft", 0.4f);
      return;
    }
  }
  if (s.empty()) return;
  const ItemId id = s.id;
  if (!inventory_.remove_at(slot_idx, 1)) return;
  loot_.spawn_item(Vec2(player_.pos.x + 12.f, player_.pos.y), id, 1);
  if (audio_) audio_->play("pickup", 0.45f);
}

void Game::try_mag_reload() {
  if (reload_cd_ > 0.f) return;
  sync_mag_for_equipped();
  if (gun_jammed_) {
    gun_jammed_ = false;
    reload_cd_ = 1.0f;
    attack_cd_ = 1.0f;
    if (audio_) audio_->play("craft", 0.5f);
    return;
  }
  const InvSlot& eq = inventory_.slot(equipped_slot_);
  if (eq.empty()) return;
  const WeaponDef* w = find_weapon(eq.id);
  if (!w || w->stats.is_melee || w->ammo_id <= 0) return;
  if (w->mag_capacity <= 0) {
    // Bows: nothing to chamber
    if (audio_) audio_->play("melee_swing", 0.2f);
    return;
  }
  const int need = w->mag_capacity - mag_rounds_;
  if (need <= 0) return;
  int loaded = 0;
  for (int i = 0; i < inventory_.slot_count() && loaded < need; ++i) {
    InvSlot& s = inventory_.slot(i);
    if (s.id != w->ammo_id || s.count <= 0) continue;
    const int take = (s.count < (need - loaded)) ? s.count : (need - loaded);
    inventory_.remove_at(i, take);
    loaded += take;
  }
  if (loaded <= 0) {
    if (audio_) audio_->play("melee_swing", 0.2f);
    return;
  }
  mag_rounds_ += loaded;
  reload_cd_ = 1.4f;
  attack_cd_ = 1.4f;
  if (audio_) audio_->play("reload", 0.6f);
}

void Game::try_throw_projectile() {
  if (attack_cd_ > 0.f) return;
  // Prefer F1 (21) / Molotov (102), else wood sticks (38)
  int slot = -1;
  float dmg_base = 12.f;
  float speed = 220.f;
  float life = 1.1f;
  bool is_molotov = false;
  if (!inventory_.slot(equipped_slot_).empty()) {
    const ItemId eid = inventory_.slot(equipped_slot_).id;
    if (eid == 21) { slot = equipped_slot_; dmg_base = 55.f; speed = 180.f; life = 1.4f; }
    else if (eid == 102) { slot = equipped_slot_; dmg_base = 40.f; speed = 160.f; life = 1.2f; is_molotov = true; }
    else if (eid == 38) { slot = equipped_slot_; }
  }
  if (slot < 0) {
    for (int i = 0; i < inventory_.slot_count(); ++i) {
      if (inventory_.slot(i).id == 21 && inventory_.slot(i).count > 0) {
        slot = i; dmg_base = 55.f; speed = 180.f; life = 1.4f; break;
      }
    }
  }
  if (slot < 0) {
    for (int i = 0; i < inventory_.slot_count(); ++i) {
      if (inventory_.slot(i).id == 102 && inventory_.slot(i).count > 0) {
        slot = i; dmg_base = 40.f; speed = 160.f; life = 1.2f; is_molotov = true; break;
      }
    }
  }
  if (slot < 0) {
    for (int i = 0; i < inventory_.slot_count(); ++i) {
      if (inventory_.slot(i).id == 38 && inventory_.slot(i).count > 0) { slot = i; break; }
    }
  }
  if (slot < 0) return;
  const bool is_nade = dmg_base > 35.f;
  inventory_.remove_at(slot, 1);
  attack_cd_ = is_nade ? 0.9f : 0.55f;
  const float dmg = roll_bullet_damage(dmg_base, rng_);
  bullets_.spawn(player_.pos, player_.facing_deg, speed, dmg, life, true, is_nade, is_molotov);
  if (audio_) audio_->play(is_nade ? "gunshot" : "melee_swing", 0.55f);
}

void Game::try_place_campfire() {
  // Prefer Campfire kit (id 23); else matches (24) + sticks (38) / wood piles (50)
  for (int i = 0; i < inventory_.slot_count(); ++i) {
    if (inventory_.slot(i).id != 23 || inventory_.slot(i).count <= 0) continue;
    if (!campfires_.place(player_.pos, 240.f)) return;
    inventory_.remove_at(i, 1);
    if (audio_) audio_->play("craft", 0.5f);
    return;
  }
  int match_i = -1, fuel_i = -1;
  for (int i = 0; i < inventory_.slot_count(); ++i) {
    const ItemId id = inventory_.slot(i).id;
    if (id == 24 && inventory_.slot(i).count > 0) match_i = i;
    if ((id == 38 || id == 50) && inventory_.slot(i).count > 0) fuel_i = i;
  }
  if (match_i >= 0 && fuel_i >= 0) {
    // Matches fail in heavy rain outdoors (kit still works; tent/indoor OK)
    const bool wet_out = !in_bunker_ && weather_.rain > 0.75f &&
                         indoor_t_ <= 0.f && tent_warmth_t_ <= 0.f;
    if (wet_out) {
      inventory_.remove_at(match_i, 1); // waste a match
      if (audio_) audio_->play("melee_swing", 0.25f);
      return;
    }
    if (!campfires_.place(player_.pos, 90.f)) return;
    inventory_.remove_at(match_i, 1);
    inventory_.remove_at(fuel_i, 1);
    if (audio_) audio_->play("craft", 0.45f);
    return;
  }
  // Hand-drill: ≥2 sticks; base ~25% (farmer perk +10%; dry weather better)
  int sticks = 0, stick_slot = -1;
  for (int i = 0; i < inventory_.slot_count(); ++i) {
    if (inventory_.slot(i).id == 38) {
      sticks += inventory_.slot(i).count;
      if (stick_slot < 0) stick_slot = i;
    }
  }
  if (sticks < 2 || stick_slot < 0) return;
  inventory_.remove_at(stick_slot, 1);
  // consume second stick
  for (int i = 0; i < inventory_.slot_count(); ++i) {
    if (inventory_.slot(i).id == 38 && inventory_.slot(i).count > 0) {
      inventory_.remove_at(i, 1);
      break;
    }
  }
  // Hand-drill chance: still approximate (no proven C2 literals) — rain/night mods only.
  float chance = 0.25f;
  if (player_.perk_farmer) chance += 0.10f;
  if (weather_.rain < 0.3f && !weather_.is_night) chance += 0.08f;
  if (rng_.next_float() < chance) {
    campfires_.place(player_.pos, 50.f);
    if (audio_) audio_->play("craft", 0.4f);
  } else if (audio_) {
    audio_->play("melee_swing", 0.25f);
  }
}

bool Game::try_plant_seed(ItemId seed_id) {
  ItemId crop = 0;
  if (seed_id == 68) crop = 31;      // Zucchini seeds
  else if (seed_id == 47) crop = 5;  // Tomato seeds
  else if (seed_id == 48) crop = 32; // Pepper seeds
  if (crop == 0) return false;
  // Prefer outdoor dirt (not indoor / bunker / deep water edge)
  if (in_bunker_) return false;
  CropPlot* slot = 0;
  for (int i = 0; i < crop_count_; ++i) {
    if (!crops_[i].active) { slot = &crops_[i]; break; }
  }
  if (!slot) {
    if (crop_count_ >= kMaxCrops) return false;
    slot = &crops_[crop_count_++];
  }
  if (!inventory_.remove_at(equipped_slot_, 1)) return false;
  slot->pos = player_.pos;
  slot->crop_id = crop;
  // Grow time: ~50s base; farmer faster; rain helps a bit
  float grow = player_.perk_farmer ? 35.f : 50.f;
  if (weather_.rain > 0.4f) grow *= 0.85f;
  slot->grow_t = grow;
  slot->ready = false;
  slot->active = true;
  if (audio_) audio_->play("craft", 0.4f);
  return true;
}

void Game::update_crops(float dt) {
  for (int i = 0; i < crop_count_; ++i) {
    CropPlot& c = crops_[i];
    if (!c.active || c.ready) continue;
    float rate = 1.f;
    if (weather_.rain > 0.5f) rate = 1.25f; // rain waters crops
    if (weather_.is_night) rate *= 0.7f;
    c.grow_t -= dt * rate;
    if (c.grow_t <= 0.f) {
      c.grow_t = 0.f;
      c.ready = true;
    }
  }
}

void Game::pack_crops_view(CropSaveView& out) const {
  out.count = 0;
  for (int i = 0; i < crop_count_ && out.count < CropSaveView::kMax; ++i) {
    if (!crops_[i].active || crops_[i].crop_id <= 0) continue;
    SavedCrop& s = out.plots[out.count++];
    s.x = crops_[i].pos.x;
    s.y = crops_[i].pos.y;
    s.crop_id = crops_[i].crop_id;
    s.grow_t = crops_[i].grow_t;
    s.ready = crops_[i].ready ? 1 : 0;
  }
}

void Game::apply_crops_view(const CropSaveView& in) {
  crop_count_ = 0;
  for (int i = 0; i < kMaxCrops; ++i) crops_[i].active = false;
  for (int i = 0; i < in.count && crop_count_ < kMaxCrops; ++i) {
    if (in.plots[i].crop_id <= 0) continue;
    CropPlot& c = crops_[crop_count_++];
    c.pos = Vec2(in.plots[i].x, in.plots[i].y);
    c.crop_id = in.plots[i].crop_id;
    c.grow_t = in.plots[i].grow_t;
    c.ready = in.plots[i].ready != 0;
    c.active = true;
  }
}

void Game::pack_traps_view(TrapSaveView& out) const {
  out.count = 0;
  for (int i = 0; i < trap_count_ && out.count < TrapSaveView::kMax; ++i) {
    if (!traps_[i].active) continue;
    SavedTrap& s = out.traps[out.count++];
    s.x = traps_[i].pos.x;
    s.y = traps_[i].pos.y;
    s.life = traps_[i].life;
    s.kind = traps_[i].kind;
    s.sprung = traps_[i].sprung ? 1 : 0;
  }
}

void Game::apply_traps_view(const TrapSaveView& in) {
  trap_count_ = 0;
  for (int i = 0; i < kMaxTraps; ++i) traps_[i].active = false;
  for (int i = 0; i < in.count && trap_count_ < kMaxTraps; ++i) {
    PlaceTrap& t = traps_[trap_count_++];
    t.pos = Vec2(in.traps[i].x, in.traps[i].y);
    t.life = in.traps[i].life;
    t.kind = in.traps[i].kind;
    t.sprung = in.traps[i].sprung != 0;
    t.active = true;
  }
}

void Game::pack_fires_view(FireSaveView& out) const {
  out.count = 0;
  for (int i = 0; i < campfires_.count() && out.count < FireSaveView::kMax; ++i) {
    const Campfire* f = campfires_.at(i);
    if (!f || !f->active || f->life <= 0.f) continue;
    SavedFire& s = out.fires[out.count++];
    s.x = f->pos.x;
    s.y = f->pos.y;
    s.life = f->life;
  }
}

void Game::apply_fires_view(const FireSaveView& in) {
  campfires_.clear();
  for (int i = 0; i < in.count; ++i) {
    if (in.fires[i].life <= 0.f) continue;
    campfires_.place(Vec2(in.fires[i].x, in.fires[i].y), in.fires[i].life);
  }
}

bool Game::try_place_trap(int kind) {
  // 0 bear (57), 1 barbed (70), 2 fence (50+2 sticks), 3 claymore (99), 4 landmine (56)
  int slot = -1;
  int stick_slot = -1;
  if (kind == 2) {
    for (int i = 0; i < inventory_.slot_count(); ++i) {
      if (inventory_.slot(i).id == 50 && inventory_.slot(i).count > 0) slot = i;
      if (inventory_.slot(i).id == 38 && inventory_.slot(i).count >= 2) stick_slot = i;
    }
    if (slot < 0 || stick_slot < 0) return false;
  } else {
    ItemId need = 70;
    if (kind == 0) need = 57;
    else if (kind == 3) need = 99;
    else if (kind == 4) need = 56;
    for (int i = 0; i < inventory_.slot_count(); ++i) {
      if (inventory_.slot(i).id == need && inventory_.slot(i).count > 0) { slot = i; break; }
    }
    if (slot < 0) return false;
  }
  PlaceTrap* t = 0;
  for (int i = 0; i < trap_count_; ++i) {
    if (!traps_[i].active) { t = &traps_[i]; break; }
  }
  if (!t) {
    if (trap_count_ >= kMaxTraps) return false;
    t = &traps_[trap_count_++];
  }
  t->pos = player_.pos;
  t->life = (kind == 0) ? 600.f : (kind == 2) ? 480.f : (kind == 3 || kind == 4) ? 900.f : 300.f;
  t->kind = kind;
  t->active = true;
  t->sprung = false;
  inventory_.remove_at(slot, 1);
  if (kind == 2) inventory_.remove_at(stick_slot, 2);
  if (audio_) audio_->play("craft", 0.5f);
  return true;
}

void Game::update_traps(float dt) {
  for (int i = 0; i < trap_count_; ++i) {
    PlaceTrap& t = traps_[i];
    if (!t.active) continue;
    t.life -= dt;
    if (t.life <= 0.f) { t.active = false; continue; }

    if (t.kind == 0 && !t.sprung) {
      // Bear trap: snap on zed / wolf / bandit / player
      auto spring = [&](Vec2 p, float& hp, float dmg) -> bool {
        if (dist2(p, t.pos) > 22.f * 22.f) return false;
        hp -= dmg;
        t.sprung = true;
        t.life = 8.f; // leave marker briefly
        if (audio_) audio_->play("hit", 0.7f);
        return true;
      };
      for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
        if (!z->alive) continue;
        if (spring(z->pos, z->hp, 35.f)) {
          if (z->hp <= 0.f) on_zed_killed(*z);
          break;
        }
      }
      if (!t.sprung) {
        for (int ai = 0; ai < animals_.count(); ++ai) {
          Animal* a = animals_.at(ai);
          if (!a->alive) continue;
          if (a->type != Animal_Wolf && a->type != Animal_Dog && a->type != Animal_Deer) continue;
          if (spring(a->pos, a->hp, 40.f)) {
            if (a->hp <= 0.f) on_animal_killed(*a);
            break;
          }
        }
      }
      if (!t.sprung) {
        for (Bandit* b = bandits_.begin(); b != bandits_.end(); ++b) {
          if (!b->alive) continue;
          if (spring(b->pos, b->hp, 30.f)) {
            if (b->hp <= 0.f) {
              b->alive = false;
              player_.bandits_killed += 1;
              spawn_corpse(Corpse_Bandit, b->pos, 0.f, 0);
              loot_.spawn_bandit(b->pos, rng_);
            }
            break;
          }
        }
      }
      if (!t.sprung && dist2(player_.pos, t.pos) < 18.f * 18.f) {
        apply_incoming_damage(18.f, true);
        t.sprung = true;
        t.life = 8.f;
        if (audio_) audio_->play("hit", 0.8f);
      }
    } else if (t.kind == 1) {
      // Barbed wire zone: slow + nick enemies and player
      const float r2 = 40.f * 40.f;
      if (dist2(player_.pos, t.pos) < r2) {
        // Slow applied in move section via trap check below — nick here
        if (rng_.next_float() < 0.02f) apply_incoming_damage(1.5f, true);
      }
      for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
        if (!z->alive) continue;
        if (dist2(z->pos, t.pos) < r2) {
          // Drag speed down by nudging away from center less — cut HP slowly
          if (rng_.next_float() < 0.04f) {
            z->hp -= 2.f;
            if (z->hp <= 0.f) on_zed_killed(*z);
          }
          // Slow: pull slightly toward wire center (entangle)
          const float dx = t.pos.x - z->pos.x;
          const float dy = t.pos.y - z->pos.y;
          z->pos.x += dx * 0.01f;
          z->pos.y += dy * 0.01f;
        }
      }
    } else if (t.kind == 3 && !t.sprung) {
      // Claymore: proximity charge vs hostiles (not player-friendly — stay clear)
      const float trigger = 55.f * 55.f;
      bool trip = false;
      for (Zombie* z = zombies_.begin(); z != zombies_.end() && !trip; ++z) {
        if (z->alive && dist2(z->pos, t.pos) < trigger) trip = true;
      }
      for (Bandit* b = bandits_.begin(); b != bandits_.end() && !trip; ++b) {
        if (b->alive && dist2(b->pos, t.pos) < trigger) trip = true;
      }
      if (trip) {
        t.sprung = true;
        t.life = 0.4f;
        explode_at(t.pos, 70.f, false);
        if (audio_) audio_->play("gunshot", 0.9f);
      }
    } else if (t.kind == 4 && !t.sprung) {
      // Landmine: pressure plate — zeds/bandits/wolves/player
      const float trigger = 28.f * 28.f;
      bool trip = false;
      for (Zombie* z = zombies_.begin(); z != zombies_.end() && !trip; ++z) {
        if (z->alive && dist2(z->pos, t.pos) < trigger) trip = true;
      }
      for (Bandit* b = bandits_.begin(); b != bandits_.end() && !trip; ++b) {
        if (b->alive && dist2(b->pos, t.pos) < trigger) trip = true;
      }
      for (int ai = 0; ai < animals_.count() && !trip; ++ai) {
        Animal* a = animals_.at(ai);
        if (a->alive && (a->type == Animal_Wolf || a->type == Animal_Dog) &&
            dist2(a->pos, t.pos) < trigger) trip = true;
      }
      if (!trip && dist2(player_.pos, t.pos) < trigger) trip = true;
      if (trip) {
        t.sprung = true;
        t.life = 0.35f;
        explode_at(t.pos, 95.f, true); // bigger boom + fire
        if (audio_) audio_->play("gunshot", 1.f);
      }
    } else if (t.kind == 2) {
      // Wood fence: block/push zeds and animals; soft player collision
      const float r2 = 36.f * 36.f;
      for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
        if (!z->alive) continue;
        if (dist2(z->pos, t.pos) < r2) {
          const float dx = z->pos.x - t.pos.x;
          const float dy = z->pos.y - t.pos.y;
          const float len = std::sqrt(dx * dx + dy * dy);
          if (len > 0.1f) {
            z->pos.x = t.pos.x + (dx / len) * 38.f;
            z->pos.y = t.pos.y + (dy / len) * 38.f;
          }
          if (rng_.next_float() < 0.01f) {
            t.life -= 40.f; // zeds batter fence down
            if (audio_) audio_->play("hit", 0.25f);
          }
        }
      }
      if (dist2(player_.pos, t.pos) < 22.f * 22.f) {
        const float dx = player_.pos.x - t.pos.x;
        const float dy = player_.pos.y - t.pos.y;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.1f && len < 22.f) {
          player_.pos.x = t.pos.x + (dx / len) * 22.f;
          player_.pos.y = t.pos.y + (dy / len) * 22.f;
        }
      }
    }
  }
}

void Game::try_bunker_enter_exit() {
  if (in_bunker_) {
    in_bunker_ = false;
    player_.pos = overworld_pos_;
    // Despawn bunker-local zeds / loot (coords within bunker tile space)
    const float bw = bunker_.width * 8.f + 80.f;
    const float bh = bunker_.height * 8.f + 80.f;
    for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
      if (!z->alive) continue;
      if (z->pos.x >= 0.f && z->pos.y >= 0.f && z->pos.x < bw && z->pos.y < bh)
        z->alive = false;
    }
    for (int i = 0; i < loot_.count(); ++i) {
      LootPile* p = loot_.at(i);
      if (!p->active) continue;
      if (p->pos.x >= 0.f && p->pos.y >= 0.f && p->pos.x < bw && p->pos.y < bh)
        p->active = false;
    }
    bullets_.clear();
    if (audio_) audio_->play("bunker", 0.6f);
    return;
  }
  // Enter when near bunker entrance POI
  for (int i = 0; i < mapgen_.location_count(); ++i) {
    const MapLocation& L = mapgen_.location(i);
    if (L.type != Loc_BunkerEntrance) continue;
    if (dist2(L.pos, player_.pos) < 55.f * 55.f) {
      overworld_pos_ = player_.pos;
      in_bunker_ = true;
      // Prefer interior floor (20) near center, else any walkable
      {
        const int cx0 = bunker_.width / 2;
        const int cy0 = bunker_.height / 2;
        int found_x = cx0, found_y = cy0;
        bool ok = false;
        for (int pass = 0; pass < 2 && !ok; ++pass) {
          for (int r = 0; r < 50 && !ok; ++r) {
            for (int dy = -r; dy <= r && !ok; ++dy) {
              for (int dx = -r; dx <= r && !ok; ++dx) {
                const int tx = cx0 + dx, ty = cy0 + dy;
                const int id = bunker_.tile_at(tx, ty);
                if (pass == 0 && id != 20) continue;
                if (!bunker_.is_blocked_world((tx + 0.5f) * 8.f, (ty + 0.5f) * 8.f)) {
                  found_x = tx; found_y = ty; ok = true;
                }
              }
            }
          }
        }
        player_.pos = Vec2((found_x + 0.5f) * 8.f, (found_y + 0.5f) * 8.f);
      }
      if (!bunker_looted_) {
        bunker_looted_ = true;
        auto bunker_spot = [&](float ox, float oy) -> Vec2 {
          Vec2 p(player_.pos.x + ox, player_.pos.y + oy);
          if (!bunker_.is_blocked_world(p.x, p.y)) return p;
          return player_.pos;
        };
        // Dens: typed bunker caches + mixed hostiles (Zed_per_base budget)
        const int dens = static_cast<int>(balance::Zed_per_base);
        loot_.spawn_bunker(bunker_spot(40.f, 0.f), rng_);
        loot_.spawn_bunker(bunker_spot(-50.f, 30.f), rng_);
        loot_.spawn_hospital(bunker_spot(20.f, -40.f), rng_);
        loot_.spawn_military(bunker_spot(-30.f, -60.f), rng_);
        zombies_.spawn(Zed_Normal, bunker_spot(90.f, 0.f));
        zombies_.spawn(Zed_Fast, bunker_spot(-70.f, 40.f));
        zombies_.spawn(Zed_Army, bunker_spot(60.f, -50.f));
        zombies_.spawn(Zed_Buried, bunker_spot(-20.f, 70.f));
        if (dens >= 5) {
          zombies_.spawn(Zed_Screamer, bunker_spot(100.f, 50.f));
          zombies_.spawn(Zed_ArmyShooter, bunker_spot(-90.f, -20.f));
        }
        if (dens >= 5 && rng_.next_float() < 0.45f) {
          zombies_.spawn(Zed_Tank, bunker_spot(30.f, 90.f));
          loot_.spawn_military(bunker_spot(70.f, 80.f), rng_);
        }
      }
      if (audio_) audio_->play("bunker", 0.6f);
      return;
    }
  }
}

void Game::update_army_shooters(float dt) {
  (void)dt;
  for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
    if (!z->alive || z->type != Zed_ArmyShooter) continue;
    if (z->attack_cooldown > 0.f) continue;
    const float d2 = dist2(z->pos, player_.pos);
    if (d2 > 420.f * 420.f || d2 < 55.f * 55.f) continue;
    float ang = std::atan2(player_.pos.y - z->pos.y, player_.pos.x - z->pos.x) * 57.2957795f;
    ang += rng_.range(-6.f, 6.f); // imperfect aim
    // Scale army shooter pellet with difficulty army melee table (Regular ≈18)
    const float shot_base =
      18.f * (zed_melee_damage(Zed_Army) / static_cast<float>(balance::Zed_army_dmg));
    const float dmg = roll_bullet_damage(shot_base, rng_);
    bullets_.spawn(z->pos, ang, 480.f, dmg, 0.95f, false);
    {
      const float rad = ang * 0.01745329252f;
      spawn_fx(Fx_Muzzle,
               Vec2(z->pos.x + std::cos(rad) * 12.f, z->pos.y + std::sin(rad) * 12.f),
               ang, 0.07f);
    }
    z->attack_cooldown = 1.35f;
  }
}

void Game::update_bandit_guns(float dt) {
  (void)dt;
  for (Bandit* b = bandits_.begin(); b != bandits_.end(); ++b) {
    if (!b->alive || !b->has_gun) continue;
    if (b->hp < b->max_hp * 0.3f) continue; // fleeing — no shots
    if (b->attack_cooldown > 0.f) continue;
    const float d2 = dist2(b->pos, player_.pos);
    if (d2 > 380.f * 380.f || d2 < 45.f * 45.f) continue;
    float ang = std::atan2(player_.pos.y - b->pos.y, player_.pos.x - b->pos.x) * 57.2957795f;
    // Closer = tighter aim; crouching player slightly harder to hit
    float spread = 10.f;
    if (d2 < 160.f * 160.f) spread = 5.f;
    if (crouching_) spread += 4.f;
    ang += rng_.range(-spread, spread);
    const float shot_base =
      20.f * (npc_bandit_melee_damage(true) / 10.f); // Regular gun-melee = 10
    const float dmg = roll_bullet_damage(shot_base, rng_);
    bullets_.spawn(b->pos, ang, 500.f, dmg, 0.9f, false);
    {
      const float rad = ang * 0.01745329252f;
      spawn_fx(Fx_Muzzle,
               Vec2(b->pos.x + std::cos(rad) * 12.f, b->pos.y + std::sin(rad) * 12.f),
               ang, 0.07f);
    }
    b->attack_cooldown = 1.15f;
    if (audio_) audio_->play("gunshot", 0.35f);
  }
}

void Game::update_menu(float dt, const InputState& in) {
  (void)dt;
  // Controls sheet overlay (read-only): any back/confirm closes it.
  if (show_controls_) {
    if (in.back_pressed || in.confirm_pressed || in.attack_pressed || in.pause_pressed)
      show_controls_ = false;
    return;
  }
  // Shortcuts kept from the old menu: 1/2/3 (desktop) and L+X/T/S (PSP) pick difficulty.
  if (in.slot_digit == 1) menu_difficulty_ = Difficulty_Novice;
  if (in.slot_digit == 2) menu_difficulty_ = Difficulty_Regular;
  if (in.slot_digit == 3) menu_difficulty_ = Difficulty_Veteran;

  // Items: 0 NEW GAME, 1 CONTINUE, 2 DIFFICULTY, 3 CONTROLS, 4 EXIT
  const int kItems = 5;
  if (in.nav_up) menu_focus_ = (menu_focus_ + kItems - 1) % kItems;
  if (in.nav_down) menu_focus_ = (menu_focus_ + 1) % kItems;
  if (menu_focus_ == 2 && (in.nav_left || in.nav_right)) {
    int d = static_cast<int>(menu_difficulty_);
    d += in.nav_right ? 1 : -1;
    if (d < static_cast<int>(Difficulty_Novice)) d = static_cast<int>(Difficulty_Veteran);
    if (d > static_cast<int>(Difficulty_Veteran)) d = static_cast<int>(Difficulty_Novice);
    menu_difficulty_ = static_cast<Difficulty>(d);
  }
  if (in.load_pressed) { if (!load_game_from_disk()) show_toast("NO SAVE FOUND"); return; }
  if (in.confirm_pressed || in.attack_pressed) {
    switch (menu_focus_) {
    case 0: start_new_game(menu_difficulty_); break;
    case 1: if (!load_game_from_disk()) show_toast("NO SAVE FOUND"); break;
    case 2: {
      int d = static_cast<int>(menu_difficulty_) + 1;
      if (d > static_cast<int>(Difficulty_Veteran)) d = static_cast<int>(Difficulty_Novice);
      menu_difficulty_ = static_cast<Difficulty>(d);
      break;
    }
    case 3: show_controls_ = true; break;
    default: quit_ = true; break;
    }
  }
}

bool Game::has_save() const {
  // Cached: the menus call this every frame and a full file read per frame was a large
  // PSP cost (memstick open + read, 3 path probes on failure). Invalidated on save.
  if (!platform_) return false;
  if (has_save_cache_ >= 0) return has_save_cache_ != 0;
  char* data = 0;
  std::size_t size = 0;
  bool ok = platform_->read_file("save.mdaz", &data, &size);
  if (ok) delete[] data;
  ok = ok && size > 0;
  has_save_cache_ = ok ? 1 : 0;
  return ok;
}

void Game::show_toast(const char* msg) {
  std::snprintf(toast_, sizeof(toast_), "%s", msg ? msg : "");
  toast_t_ = 1.6f;
}

void Game::pack_loot_view(LootSaveView& out) const {
  // C2 Save_loot: every ground pile with items (position + up to 8 id/count pairs).
  out.count = 0;
  for (int i = 0; i < loot_.count() && out.count < LootSaveView::kMax; ++i) {
    const LootPile* p = loot_.at(i);
    if (!p->active || p->item_count <= 0 || p->quest_reward_case) continue;
    SavedLootPile& s = out.piles[out.count++];
    s.x = p->pos.x; s.y = p->pos.y;
    const int n = p->item_count < 8 ? p->item_count : 8;
    s.item_count = static_cast<unsigned char>(n);
    s.is_airdrop = p->is_airdrop ? 1 : 0;
    for (int k = 0; k < 8; ++k) {
      s.items[k] = k < n ? p->items[k] : 0;
      const int c = k < n ? p->counts[k] : 0;
      s.counts[k] = static_cast<unsigned char>(c < 0 ? 0 : (c > 255 ? 255 : c));
    }
  }
}

void Game::apply_loot_view(const LootSaveView& in) {
  // C2 Load_loot replaces the ground piles; saves without loot (count 0) keep the new world.
  if (in.count <= 0) return;
  loot_.load_from_save(in.piles, in.count);
}

void Game::save_game_to_disk() {
  CropSaveView crops_view;
  TrapSaveView traps_view;
  FireSaveView fires_view;
  WorldExtras extras;
  pack_crops_view(crops_view);
  pack_traps_view(traps_view);
  pack_fires_view(fires_view);
  LootSaveView* loot_view = &load_loot_view_;
  pack_loot_view(*loot_view);
  extras.airdrop_t = timers_.airdrop;
  extras.airdrop_x = last_airdrop_pos_.x;
  extras.airdrop_y = last_airdrop_pos_.y;
  extras.tent_warmth_t = tent_warmth_t_;
  extras.adrenaline_t = adrenaline_t_;
  extras.bunker_looted = bunker_looted_ ? 1 : 0;
  extras.melee_break_mul = melee_break_mul_;
  const bool ok = SaveSystem::save_file(*platform_, "save.mdaz", player_, inventory_, weather_,
                        equipped_slot_, visited_sectors_,
                        kMapSectorsX * kMapSectorsY,
                        torch_battery_, quest_.q_task_id, quest_.q_status,
                        mag_rounds_, wetness_, gasmask_filter_, player_.fish_caught,
                        &vehicles_, &crops_view, &traps_view, &extras, &fires_view, loot_view);
  has_save_cache_ = -1;
  show_toast(ok ? "GAME SAVED" : "SAVE FAILED");
}

bool Game::load_game_from_disk() {
  if (!has_save()) return false;
  // Loading from the main menu before any world exists: build the world first
  // (save v18 stores player/inventory/world extras, not the generated map).
  if (mapgen_.location_count() == 0) start_new_game(menu_difficulty_);
  int qact = 0, qprog = 0;
  int mag = 0;
  float wet = 0.f;
  float gfilt = 100.f;
  int fish = 0;
  CropSaveView crops_view;
  TrapSaveView traps_view;
  FireSaveView fires_view;
  WorldExtras extras;
  load_loot_view_.count = 0;
  if (!SaveSystem::load_file(*platform_, "save.mdaz", player_, inventory_, weather_,
                             &equipped_slot_, visited_sectors_,
                             kMapSectorsX * kMapSectorsY,
                             &torch_battery_, &qact, &qprog, &mag, &wet,
                             &gfilt, &fish, &vehicles_, &crops_view, &traps_view,
                             &extras, &fires_view, &load_loot_view_))
    return false;
  apply_loot_view(load_loot_view_);
  set_npc_damage_for_difficulty(player_.difficulty);
  menu_difficulty_ = player_.difficulty;
  in_bunker_ = false;
  bullets_.clear();
  mag_rounds_ = mag;
  wetness_ = wet;
  gasmask_filter_ = gfilt;
  player_.fish_caught = fish;
  equipped_slot_ = next_hotbar_slot(equipped_slot_, 0, 0); // clamp a bad save index
  mag_weapon_id_ = inventory_.slot(equipped_slot_).id;
  apply_crops_view(crops_view);
  apply_traps_view(traps_view);
  apply_fires_view(fires_view);
  tent_warmth_t_ = extras.tent_warmth_t;
  adrenaline_t_ = extras.adrenaline_t;
  bunker_looted_ = extras.bunker_looted != 0;
  melee_break_mul_ = extras.melee_break_mul > 0.f ? extras.melee_break_mul : 1.f;
  timers_.airdrop = extras.airdrop_t > 0.f ? extras.airdrop_t : 360.f;
  last_airdrop_pos_ = Vec2(extras.airdrop_x, extras.airdrop_y);
  mark_sector_visited();
  quest_.reset();
  if (qact > 0 && qact <= 3) {
    quest_.q_task_id = qact;
    quest_.q_status = qprog;
  } else {
    quest_.generate_quest(rng_, mapgen_, 1);
  }
  if (vehicles_.count() == 0) {
    vehicles_.spawn(Vehicle_Car, Vec2(player_.pos.x + 90.f, player_.pos.y - 50.f));
    vehicles_.spawn(Vehicle_Boat, Vec2(180.f, player_.pos.y));
  }
  inventory_ui_.close_inventory();
  show_bag_ = false;
  show_craft_menu_ = false;
  state_ = AppState_Playing;
  show_toast("GAME LOADED");
  return true;
}

void Game::compute_nav(InputState& in, float dt) {
  // D-pad / analog -> edge + auto-repeat (0.32s delay, 0.11s rate). Platform-agnostic.
  const bool held[4] = { in.move_y < -0.5f, in.move_y > 0.5f, in.move_x < -0.5f, in.move_x > 0.5f };
  bool* out[4] = { &in.nav_up, &in.nav_down, &in.nav_left, &in.nav_right };
  for (int i = 0; i < 4; ++i) {
    *out[i] = false;
    if (held[i]) {
      if (!nav_prev_[i]) { *out[i] = true; nav_hold_t_[i] = 0.f; }
      else {
        nav_hold_t_[i] += dt;
        if (nav_hold_t_[i] > 0.32f) { *out[i] = true; nav_hold_t_[i] -= 0.11f; }
      }
    } else {
      nav_hold_t_[i] = 0.f;
    }
    nav_prev_[i] = held[i];
  }
}

void Game::update_pause(float dt, const InputState& in) {
  (void)dt;
  if (show_controls_) {
    if (in.back_pressed || in.confirm_pressed || in.attack_pressed || in.pause_pressed)
      show_controls_ = false;
    return;
  }
  // 0 RESUME, 1 SAVE, 2 LOAD, 3 CANCEL TASK, 4 CONTROLS,
  // [PSP only: 5 HALF RES, 6 MS OVERLAY], last MAIN MENU
  const bool hr = platform_ && platform_->supports_half_res();
  const int kItems = hr ? 8 : 6;
  if (in.nav_up) pause_focus_ = (pause_focus_ + kItems - 1) % kItems;
  if (in.nav_down) pause_focus_ = (pause_focus_ + 1) % kItems;
  if (in.pause_pressed || in.back_pressed) { state_ = AppState_Playing; return; }
  if (in.save_pressed) save_game_to_disk();
  if (in.load_pressed && !load_game_from_disk()) show_toast("NO SAVE FOUND");
  if (in.confirm_pressed || in.attack_pressed) {
    switch (pause_focus_) {
    case 0: state_ = AppState_Playing; break;
    case 1: save_game_to_disk(); break;
    case 2: if (!load_game_from_disk()) show_toast("NO SAVE FOUND"); break;
    case 3:
      if (quest_.show_panel()) { try_cancel_quest(); show_toast("TASK CANCELED"); }
      else show_toast("NO ACTIVE TASK");
      break;
    case 4: show_controls_ = true; break;
    case 5: if (hr) { toggle_half_res(); break; }
            state_ = AppState_Menu; menu_focus_ = 0; break;
    case 6: if (hr) { toggle_perf_overlay(); break; }
            state_ = AppState_Menu; menu_focus_ = 0; break;
    default: state_ = AppState_Menu; menu_focus_ = 0; break;
    }
  }
}

void Game::try_cancel_quest() {
  // C2 Quest_canceled (quest pad cancel) + Karma(karma_points), then the next task is rolled
  // like after a finished reward (Generate_quest).
  if (!quest_.cancel()) return;
  quest_bandits_spawned_ = false;
  if (audio_) audio_->play("craft", 0.4f);
  quest_.generate_quest(rng_, mapgen_, 1);
}

void Game::close_inventory() {
  // C2 close_inventory: hide bag + craft sub-menu, drop focus.
  inventory_ui_.close_inventory();
  show_bag_ = false;
  show_craft_menu_ = false;
}

void Game::sync_bag_overlay_from_ui() { show_bag_ = inventory_ui_.opened; }

void Game::toggle_craft_menu() {
  // C2 Call_sub_menu_craft: open/close the recipe list (closes the bag panel).
  show_craft_menu_ = !show_craft_menu_;
  if (show_craft_menu_) {
    inventory_ui_.close_inventory();
    sync_bag_overlay_from_ui();
    const int n = craft_recipe_count();
    if (craft_menu_index_ >= n) craft_menu_index_ = 0;
  }
}

void Game::try_craft_menu_recipe() {
  const CraftRecipe* r = craft_recipe_at(craft_menu_index_);
  if (!r) return;
  const bool at_fire = campfires_.near_fire(player_.pos);
  if (r->requires_fire && !at_fire) { show_toast("NEED A FIRE"); return; }
  if (craft_do(inventory_, *r, items_)) {
    if (audio_) audio_->play("craft", 0.7f);
    show_toast("CRAFTED");
  } else {
    inv_full_flash_ = 0.6f;
    show_toast("MISSING ITEMS");
  }
}

void Game::update_craft_menu(const InputState& in) {
  const int n = craft_recipe_count();
  if (n <= 0) { show_craft_menu_ = false; return; }
  if (craft_menu_index_ < 0 || craft_menu_index_ >= n) craft_menu_index_ = 0;
  if (in.nav_up) craft_menu_index_ = (craft_menu_index_ + n - 1) % n;
  if (in.nav_down) craft_menu_index_ = (craft_menu_index_ + 1) % n;
  if (in.back_pressed || in.craft_pressed) { show_craft_menu_ = false; return; }
  if (in.confirm_pressed || in.attack_pressed) try_craft_menu_recipe();
}

int Game::next_hotbar_slot(int cur, int digit, int delta) {
  // Hotbar = bag slots 0..7 (8 quick slots). Digit 1..8 selects directly, delta cycles.
  if (cur < 0 || cur >= kQuickSlots) cur = 0;
  if (digit >= 1 && digit <= kQuickSlots) return digit - 1;
  if (delta != 0) {
    int s = (cur + delta) % kQuickSlots;
    if (s < 0) s += kQuickSlots;
    return s;
  }
  return cur;
}

void Game::use_bag_slot(int slot) {
  if (slot < 0 || slot >= inventory_.slot_count() || inventory_.slot(slot).empty()) return;
  const int prev = equipped_slot_;
  equipped_slot_ = slot;
  use_equipped_item();
  equipped_slot_ = prev;
  sync_mag_for_equipped();
}

void Game::equip_bag_slot(int slot) {
  if (slot < 0 || slot >= inventory_.capacity()) return;
  if (slot < 8) {
    equipped_slot_ = slot;
  } else {
    // Move the focused stack into the hand (hotbar) slot.
    InvSlot tmp = inventory_.slot(slot);
    inventory_.slot(slot) = inventory_.slot(equipped_slot_);
    inventory_.slot(equipped_slot_) = tmp;
  }
  sync_mag_for_equipped();
  if (audio_) audio_->play("pickup", 0.3f);
}

void Game::update_inventory_ui(const InputState& in) {
  InventoryUiState& ui = inventory_ui_;
  const int cap = inventory_.capacity();
  const int cols = 8;
  static const ClothingRegion kRows[4] = { ClothingRegion_Pants, ClothingRegion_Outerwear,
                                           ClothingRegion_Vest, ClothingRegion_Backpack };
  if (in.back_pressed || in.bag_pressed) { close_inventory(); return; }
  const bool in_cloth = ui.focus_cloth_region != ClothingRegion_Count;
  if (!in_cloth && (ui.focus_bag_slot < 0 || ui.focus_bag_slot >= cap))
    ui.focus_bag_slot = equipped_slot_ < cap ? equipped_slot_ : 0;

  if (in_cloth) {
    int row = 0;
    for (int r = 0; r < 4; ++r) if (kRows[r] == ui.focus_cloth_region) row = r;
    if (in.nav_left && ui.focus_cloth_sub > 0) ui.focus_cloth_sub -= 1;
    if (in.nav_right) {
      if (ui.focus_cloth_sub < kClothingContainerSlots - 1) ui.focus_cloth_sub += 1;
      else {
        int target = (row < 3 ? row : 2) * cols;
        if (target >= cap) target = cap - 1;
        ui.focus_cloth_region = ClothingRegion_Count;
        ui.focus_cloth_sub = -1;
        ui.focus_bag_slot = target < 0 ? 0 : target;
        return;
      }
    }
    if (in.nav_up || in.nav_down) {
      const int dir = in.nav_up ? -1 : 1;
      for (int k = 1; k <= 4; ++k) {
        const int r = row + dir * k;
        if (r < 0 || r > 3) break;
        if (inventory_.clothing().region_available(kRows[r])) { ui.focus_cloth_region = kRows[r]; break; }
      }
    }
    if (in.confirm_pressed || in.attack_pressed) {
      const PickPlayerItemResult res =
          pick_player_item(inventory_, items_, ui.focus_cloth_region, ui.focus_cloth_sub);
      if (res == PickPlayer_Ok) { if (audio_) audio_->play("pickup", 0.4f); }
      else if (res == PickPlayer_NoSpace) inv_full_flash_ = 1.f;
    }
    return;
  }

  int s = ui.focus_bag_slot;
  const int col = s % cols;
  if (in.nav_left) {
    if (col > 0) s -= 1;
    else {
      // Jump to the clothing container on the same row (if worn).
      const int row = s / cols;
      for (int k = 0; k < 4; ++k) {
        const int r = (row + k) % 4;
        if (inventory_.clothing().region_available(kRows[r])) {
          ui.focus_cloth_region = kRows[r];
          ui.focus_cloth_sub = kClothingContainerSlots - 1;
          ui.focus_bag_slot = -1;
          return;
        }
      }
    }
  }
  if (in.nav_right && col < cols - 1 && s + 1 < cap) s += 1;
  if (in.nav_up && s - cols >= 0) s -= cols;
  if (in.nav_down && s + cols < cap) s += cols;
  ui.focus_bag_slot = s;

  if (in.confirm_pressed || in.attack_pressed) use_bag_slot(s);          // Cross: use / wear
  else if (in.inventory_pressed) equip_bag_slot(s);                      // Triangle: to hand
  else if (in.mag_reload_pressed || in.reload_pressed) try_drop_slot(s); // Square: drop
}

const char* Game::interact_hint() const {
  if (vehicles_.player_driving()) return "EXIT CAR";
  for (int i = 0; i < loot_.count(); ++i) {
    const LootPile* p = loot_.at(i);
    if (p->active && p->item_count > 0 && dist2(p->pos, player_.pos) < 40.f * 40.f) return "LOOT";
  }
  if (near_vehicle()) return "ENTER CAR";
  for (int i = 0; i < mapgen_.location_count(); ++i) {
    const MapLocation& L = mapgen_.location(i);
    if (L.type == Loc_BunkerEntrance && dist2(L.pos, player_.pos) < 80.f * 80.f) return "BUNKER";
  }
  if (in_bunker_) return "SEARCH";
  const ItemId eq = inventory_.slot(equipped_slot_).id;
  if ((eq == 202 || eq == 204 || eq == 206) && near_choppable_tree()) return "CHOP";
  return 0;
}


void Game::update_playing(float dt, const InputState& in_raw) {
  // PSP UI overlays (inventory / craft) take the buttons; the world keeps running
  // (C2 does not pause while Inventory_opened), but the player gets no action input.
  InputState in = in_raw;
  if (inventory_ui_.opened) {
    update_inventory_ui(in_raw);
    in = InputState();
  } else if (show_craft_menu_) {
    update_craft_menu(in_raw);
    in = InputState();
  } else {
    if (in_raw.pause_pressed) {
      state_ = AppState_Paused;
      pause_focus_ = 0;
      return;
    }
    if (in_raw.bag_pressed) {
      inventory_ui_.open();
      inventory_ui_.focus_bag_slot = equipped_slot_;
      show_craft_menu_ = false;
      sync_bag_overlay_from_ui();
      in = InputState();
    } else if (in_raw.craft_pressed) {
      toggle_craft_menu();
      in = InputState();
    }
  }

  if ((in.slot_digit >= 1 && in.slot_digit <= 8) || in.equip_delta != 0) {
    equipped_slot_ = next_hotbar_slot(equipped_slot_, in.slot_digit, in.equip_delta);
    sync_mag_for_equipped();
  }

  if (adrenaline_t_ > 0.f) {
    adrenaline_t_ -= dt;
    if (adrenaline_t_ < 0.f) adrenaline_t_ = 0.f;
  }

  const bool was_driving = vehicles_.player_driving();
  bool has_paddle = false;
  for (int i = 0; i < inventory_.slot_count(); ++i) {
    const ItemId id = inventory_.slot(i).id;
    if ((id == 38 || id == 37) && inventory_.slot(i).count > 0) { has_paddle = true; break; }
  }
  const bool drove = (!in_bunker_) &&
    vehicles_.update(dt, player_.pos, in.move_x, in.move_y, in.interact_pressed, has_paddle);
  if (!drove) {
    float mx = in.move_x, my = in.move_y;
    if (!in_bunker_) {
      // Soft water band near map edges — wade slow (full water tiles TBD)
      const float margin = 400.f;
      const bool wading = player_.pos.x < margin || player_.pos.y < margin ||
          player_.pos.x > kMapLayoutW - margin || player_.pos.y > kMapLayoutH - margin;
      if (wading) {
        mx *= 0.55f;
        my *= 0.55f;
        if ((in.move_x != 0.f || in.move_y != 0.f) && rng_.next_float() < 0.04f)
          spawn_fx(Fx_Hit, player_.pos, 0.f, 0.15f);
      }
      // Heavy rain slows sprint; raincoat (353) resists wet slow + infection
      if (weather_.rain > 0.7f) {
        bool raincoat = false;
        for (int wi = 0; wi < inventory_.worn_count(); ++wi) {
          if (inventory_.worn_at(wi) == 353) { raincoat = true; break; }
        }
        if (!raincoat) {
          mx *= 0.85f;
          my *= 0.85f;
          if (!(wearing_gasmask() && gasmask_filter_ > 5.f) &&
              !player_.survival.infected && rng_.next_float() < 0.002f) {
            player_.survival.infected = true;
          }
        }
      }
      // Barbed wire / barricades near military POI — slow + occasional nick
      for (int i = 0; i < mapgen_.location_count(); ++i) {
        const MapLocation& L = mapgen_.location(i);
        if (L.type != Loc_Military) continue;
        if (dist2(L.pos, player_.pos) < 90.f * 90.f) {
          mx *= 0.45f;
          my *= 0.45f;
          if (rng_.next_float() < 0.015f) {
            apply_incoming_damage(1.f, true);
          }
          break;
        }
      }
      // Placed barbed wire traps
      for (int ti = 0; ti < trap_count_; ++ti) {
        if (!traps_[ti].active || traps_[ti].kind != 1) continue;
        if (dist2(traps_[ti].pos, player_.pos) < 40.f * 40.f) {
          mx *= 0.4f;
          my *= 0.4f;
          break;
        }
      }
    }
    if (adrenaline_t_ > 0.f) {
      mx *= 1.25f;
      my *= 1.25f;
    }
    crouching_ = in.crouch;
    if (crouching_) {
      mx *= 0.45f;
      my *= 0.45f;
    }
    const bool can_sprint = !crouching_ &&
                            player_.survival.food > 8.f &&
                            player_.survival.water > 8.f;
    const Vec2 pre_move = player_.pos;
    player_.apply_input(mx, my, in.run && can_sprint, dt);
    if (!in_bunker_ && world_.is_blocked(player_.pos.x, player_.pos.y)) {
      player_.pos.x -= player_.vel.x * dt;
      player_.pos.y -= player_.vel.y * dt;
    }
    // C2 tilemap_forest is Solid (per-tile collision polys): slide along the edge.
    if (!in_bunker_ && world_tiles_.forest_solid_at(player_.pos.x, player_.pos.y)) {
      if (!world_tiles_.forest_solid_at(player_.pos.x, pre_move.y)) player_.pos.y = pre_move.y;
      else if (!world_tiles_.forest_solid_at(pre_move.x, player_.pos.y)) player_.pos.x = pre_move.x;
      else player_.pos = pre_move;
    }
    // Soft collision vs forest trees
    if (!in_bunker_) {
      for (int ti = 0; ti < tree_count_; ++ti) {
        const float dx = player_.pos.x - trees_[ti].x;
        const float dy = player_.pos.y - trees_[ti].y;
        const float d2t = dx * dx + dy * dy;
        if (d2t < 14.f * 14.f && d2t > 0.01f) {
          const float len = std::sqrt(d2t);
          player_.pos.x = trees_[ti].x + (dx / len) * 14.f;
          player_.pos.y = trees_[ti].y + (dy / len) * 14.f;
        }
      }
    }
    if (in_bunker_ && bunker_.is_blocked_world(player_.pos.x, player_.pos.y)) {
      const float rx = player_.pos.x - player_.vel.x * dt;
      const float ry = player_.pos.y - player_.vel.y * dt;
      player_.pos.x = rx;
      player_.pos.y = ry;
      if (!bunker_.is_blocked_world(rx + player_.vel.x * dt, ry))
        player_.pos.x = rx + player_.vel.x * dt;
      if (!bunker_.is_blocked_world(player_.pos.x, ry + player_.vel.y * dt))
        player_.pos.y = ry + player_.vel.y * dt;
    }
  } else {
    if (in.move_x != 0.f || in.move_y != 0.f) {
      player_.facing_deg = std::atan2(in.move_y, in.move_x) * 57.2957795f;
    }
    // Footsteps — forest / sand / bunker / default run clips
    {
      const float speed2 = player_.vel.x * player_.vel.x + player_.vel.y * player_.vel.y;
      if (speed2 > 8.f * 8.f && audio_) {
        const bool sprinting = speed2 > 55.f * 55.f;
        footstep_t_ -= dt;
        if (footstep_t_ <= 0.f) {
          footstep_t_ = sprinting ? 0.28f : 0.42f;
          const float margin = 420.f;
          const bool shore = player_.pos.x < margin || player_.pos.y < margin ||
            player_.pos.x > kMapLayoutW - margin || player_.pos.y > kMapLayoutH - margin;
          const char* step = in_bunker_ ? "step_concrete" :
                             shore ? "step_sand" : "step_forest";
          audio_->play_at(step, player_.pos.x, player_.pos.y, sprinting ? 0.35f : 0.22f);
        }
      } else {
        footstep_t_ = 0.f;
      }
    }
    // Crash into blocked cells / Solid forest tiles while driving
    if (world_.is_blocked(player_.pos.x, player_.pos.y) ||
        world_tiles_.forest_solid_at(player_.pos.x, player_.pos.y)) {
      player_.pos.x -= in.move_x * 8.f;
      player_.pos.y -= in.move_y * 8.f;
      Vehicle* v = vehicles_.driven();
      if (v) {
        v->hp -= 8.f * dt * 60.f;
        if (v->hp < 0.f) v->hp = 0.f;
        apply_incoming_damage(2.f * dt * 10.f, false);
        if (audio_ && rng_.next_float() < 0.05f) audio_->play("hit", 0.4f);
      }
    }
    // Hit a tree while driving — damage vehicle + fell tree
    if (vehicles_.player_driving()) {
      for (int ti = 0; ti < tree_count_; ++ti) {
        if (dist2(trees_[ti], player_.pos) > 28.f * 28.f) continue;
        Vehicle* v = vehicles_.driven();
        if (v) {
          v->hp -= 15.f;
          if (v->hp < 0.f) v->hp = 0.f;
          apply_incoming_damage(4.f, true);
        }
        // Snap tree into sticks loot
        const Vec2 fell = trees_[ti];
        loot_.spawn_item(fell, 38, 2 + rng_.next_int(0, 2));
        trees_[ti] = trees_[tree_count_ - 1];
        tree_count_ -= 1;
        if (audio_) audio_->play("hit", 0.6f);
        break;
      }
    }
  }
  const bool vehicle_interact = was_driving || vehicles_.player_driving();
  if (fuel_warn_cd_ > 0.f) fuel_warn_cd_ -= dt;
  if (vehicles_.player_driving() && vehicles_.driven() &&
      vehicles_.driven()->type != Vehicle_Bike &&
      vehicles_.driven()->fuel < 20.f && fuel_warn_cd_ <= 0.f) {
    fuel_warn_cd_ = 4.f;
    if (audio_) audio_->play("melee_swing", 0.35f);
  }
  if (rain_sfx_cd_ > 0.f) rain_sfx_cd_ -= dt;
  if (thunder_cd_ > 0.f) thunder_cd_ -= dt;
  if (lightning_t_ > 0.f) lightning_t_ -= dt;
  if (!in_bunker_ && weather_.rain > 0.85f && rain_sfx_cd_ <= 0.f && audio_) {
    audio_->play("rain", 0.3f);
    rain_sfx_cd_ = 3.5f;
  }
  if (wind_sfx_cd_ > 0.f) wind_sfx_cd_ -= dt;
  if (flare_t_ > 0.f) {
    flare_t_ -= dt;
    // Flare pulls infected toward the beacon
    for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
      if (!z->alive) continue;
      const float dx = flare_pos_.x - z->pos.x;
      const float dy = flare_pos_.y - z->pos.y;
      const float d2 = dx * dx + dy * dy;
      if (d2 > 500.f * 500.f || d2 < 1.f) continue;
      const float len = std::sqrt(d2);
      z->pos.x += (dx / len) * z->speed * 0.55f * dt;
      z->pos.y += (dy / len) * z->speed * 0.55f * dt;
      if (z->aggro_radius < 380.f) z->aggro_radius = 380.f;
    }
  }
  if (!in_bunker_ && weather_.is_night && weather_.rain < 0.4f &&
      wind_sfx_cd_ <= 0.f && audio_ && rng_.next_float() < 0.008f) {
    audio_->play("wind", 0.25f);
    wind_sfx_cd_ = 12.f + rng_.next_float() * 18.f;
  }
  // Nearby crow call
  if (!in_bunker_ && audio_ && wind_sfx_cd_ <= 0.f && rng_.next_float() < 0.01f) {
    for (int i = 0; i < animals_.count(); ++i) {
      const Animal* a = animals_.at(i);
      if (!a->alive || a->type != Animal_Crow) continue;
      if (dist2(a->pos, player_.pos) > 220.f * 220.f) continue;
      audio_->play_at("crow", a->pos.x, a->pos.y, 0.35f);
      wind_sfx_cd_ = 6.f + rng_.next_float() * 10.f;
      break;
    }
  }
  if (!in_bunker_ && weather_.rain > 0.9f && thunder_cd_ <= 0.f &&
      rng_.next_float() < 0.015f) {
    thunder_cd_ = 8.f + rng_.next_float() * 14.f;
    lightning_t_ = 0.12f;
    if (audio_) audio_->play("thunder", 0.7f);
    // Thunder startles nearby zeds — brief aggro enlarge
    for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
      if (!z->alive) continue;
      if (dist2(z->pos, player_.pos) < 500.f * 500.f && z->aggro_radius < 360.f)
        z->aggro_radius = 360.f;
    }
  }

  if (!in_bunker_) {
    const float prev_phase = weather_.day_phase;
    weather_.update(dt);
    // Day rolled over
    if (weather_.day_phase < prev_phase) {
      player_.days_survived += 1;
    }
  }
  {
    // Wetness: soak in rain, dry near fire / indoors / bunker
    bool raincoat = false;
    for (int wi = 0; wi < inventory_.worn_count(); ++wi) {
      if (inventory_.worn_at(wi) == 353) { raincoat = true; break; }
    }
    bool bandana = false;
    for (int wi = 0; wi < inventory_.worn_count(); ++wi) {
      if (inventory_.worn_at(wi) == 266) { bandana = true; break; }
    }
    if (!in_bunker_ && weather_.rain > 0.7f && indoor_t_ <= 0.f) {
      float soak = raincoat ? 4.f : 14.f;
      if (bandana) soak *= 0.85f; // bandana sheds a bit of rain
      wetness_ += soak * dt;
    } else {
      float dry = 3.f * dt;
      if (campfires_.near_fire(player_.pos, 70.f)) dry += 18.f * dt;
      if (tent_warmth_t_ > 0.f) dry += 12.f * dt;
      if (indoor_t_ > 0.f || in_bunker_) dry += 14.f * dt;
      wetness_ -= dry;
    }
    if (wetness_ < 0.f) wetness_ = 0.f;
    if (wetness_ > 100.f) wetness_ = 100.f;

    // Gasmask filter drains while worn (blocks infection only while >5)
    if (wearing_gasmask()) {
      gasmask_filter_ -= 0.35f * dt;
      if (gasmask_filter_ < 0.f) gasmask_filter_ = 0.f;
    }

    // Clothing insulation stub: count clothing items in bag (full equip slots TBD)
    float insulation = in_bunker_ ? 12.f : 0.f; // bunker is strong shelter
    if (player_.torch_on) insulation += 3.f;
    if (tent_warmth_t_ > 0.f) {
      insulation += 6.f; // tent shelters alone
      if (campfires_.near_fire(player_.pos, 70.f)) insulation += 3.f; // tent+fire
    }
    if (indoor_t_ > 0.f) insulation += 4.f; // building shelter stub
    insulation += inventory_.clothing_insulation();
    insulation += campfires_.warmth_near(player_.pos) * 6.f; // fire warmth
    if (player_.perk_snowborn) insulation += 4.f;
    if (insulation > 18.f) insulation = 18.f;
    if (insulation < 0.f) insulation = 0.f;
    // Endure freeze → unlock snowborn; thirsty → camel
    if (!player_.perk_snowborn && player_.survival.temperature < 22.f) {
      player_.cold_endure_t_ += dt;
      if (player_.cold_endure_t_ >= 120.f) player_.perk_snowborn = true;
    }
    if (!player_.perk_camel && player_.survival.water < 25.f) {
      player_.thirst_endure_t_ += dt;
      if (player_.thirst_endure_t_ >= 100.f) player_.perk_camel = true;
    }
    if (!player_.perk_hamster && player_.survival.food < 25.f) {
      player_.starve_endure_t_ += dt;
      if (player_.starve_endure_t_ >= 100.f) player_.perk_hamster = true;
    }
    if (!player_.perk_regen && player_.survival.blood < 40.f &&
        player_.survival.bleeding <= 0.f) {
      player_.wound_endure_t_ += dt;
      if (player_.wound_endure_t_ >= 90.f) player_.perk_regen = true;
    }
    if (!player_.perk_runner && player_.running) {
      player_.sprint_t_ += dt;
      if (player_.sprint_t_ >= 180.f) player_.perk_runner = true;
    }
    if (!player_.perk_metabolism && player_.survival.bleeding > 0.f) {
      player_.bleed_endure_t_ += dt;
      if (player_.bleed_endure_t_ >= 60.f) player_.perk_metabolism = true;
    }
    {
      float rain_on_fire = (!in_bunker_ && indoor_t_ <= 0.f) ? weather_.rain : 0.f;
      // Tent shelters nearby fire from rain soak
      if (tent_warmth_t_ > 0.f && campfires_.near_fire(player_.pos, 80.f)) rain_on_fire = 0.f;
      campfires_.update(dt, rain_on_fire);
    }
    player_.update_survival(dt, weather_.ambient_temp, weather_.is_night && !in_bunker_,
                            !in_bunker_ && wetness_ > 50.f, insulation);
    // Rest near fire while standing still — mild blood regen + night sleep
    if (campfires_.near_fire(player_.pos, 55.f) &&
        player_.vel.x * player_.vel.x + player_.vel.y * player_.vel.y < 1.f) {
      if (player_.survival.blood < 100.f && player_.survival.bleeding <= 0.f) {
        player_.survival.blood += 1.2f * dt;
        if (player_.survival.blood > 100.f) player_.survival.blood = 100.f;
      }
      if (!in_bunker_ && weather_.is_night) {
        // Sleep accelerates dawn
        weather_.day_phase += dt * 0.08f;
        if (weather_.day_phase >= 1.f) {
          weather_.day_phase -= 1.f;
          player_.days_survived += 1;
        }
        weather_.is_night = (weather_.day_phase < 0.25f || weather_.day_phase > 0.75f);
      }
    }
    // Auto heatpack when freezing hard
    if (player_.survival.temperature < 18.f) {
      for (int i = 0; i < inventory_.slot_count(); ++i) {
        if (inventory_.slot(i).id != 34 || inventory_.slot(i).count <= 0) continue;
        if (inventory_.use_consumable(i, player_.survival, items_, &rng_)) {
          use_anim_t_ = 0.5f; // C2 using_item ~4 frames @ speed 8
          if (audio_) audio_->play("craft", 0.35f);
          break;
        }
      }
    }
  }
  timers_.update(dt);
  dispersion_.update_settled(dt, crouching_ || !player_.running);
  if (attack_cd_ > 0.f) attack_cd_ -= dt;
  if (reload_cd_ > 0.f) reload_cd_ -= dt;
  if (melee_anim_t_ > 0.f) melee_anim_t_ -= dt;
  if (use_anim_t_ > 0.f) use_anim_t_ -= dt;
  {
    const float spd2 = player_.vel.x * player_.vel.x + player_.vel.y * player_.vel.y;
    if (spd2 > 4.f) {
      // Match C2 run speed≈10 / walk≈6 (anim Speed field).
      player_anim_t_ += dt * (player_.running ? 1.f : 0.7f);
    } else {
      player_anim_t_ = 0.f;
    }
  }
  if (inv_full_flash_ > 0.f) inv_full_flash_ -= dt;
  if (tent_warmth_t_ > 0.f) tent_warmth_t_ -= dt;
  if (indoor_t_ > 0.f) indoor_t_ -= dt;
  {
    float drain = 0.f;
    if (player_.torch_on) drain += has_headlamp_ ? 0.9f : 1.5f; // headlamp more efficient
    if (player_.nvg_on) drain += 2.2f;
    if (drain > 0.f) {
      torch_battery_ -= drain * dt;
      if (torch_battery_ <= 0.f) {
        torch_battery_ = 0.f;
        player_.torch_on = false;
        player_.nvg_on = false;
      }
    }
  }

  if (!in_bunker_) {
    // Soft cap ≈ 4× Zed_per_base (global dens budget); was hard 12
    const int zed_cap = static_cast<int>(balance::Zed_per_base) * 4;
    if (timers_.consume_zed_respawn() && zombies_.alive_count() < zed_cap) {
      // Prefer spawn near a distant POI; fallback ring around player
      Vec2 spawn_at;
      bool used_poi = false;
      if (mapgen_.location_count() > 0) {
        const MapLocation& L = mapgen_.location(rng_.next_int(0, mapgen_.location_count() - 1));
        if (dist2(L.pos, player_.pos) > 180.f * 180.f) {
          spawn_at = Vec2(L.pos.x + rng_.range(-40.f, 40.f), L.pos.y + rng_.range(-40.f, 40.f));
          used_poi = true;
          ZedType zt = Zed_Normal;
          if (L.type == Loc_Military) {
            const float r = rng_.next_float();
            if (r < 0.15f) zt = Zed_Tank;
            else if (r < 0.55f) zt = Zed_Army;
            else zt = Zed_ArmyShooter;
          } else if (L.type == Loc_Hospital) zt = Zed_Fast;
          else if (L.type == Loc_Police) {
            zt = (rng_.next_float() < 0.45f) ? Zed_ArmyShooter : Zed_Normal;
          } else if (L.type == Loc_Secret) zt = Zed_Screamer;
          else if (L.type == Loc_Village || L.type == Loc_City) {
            zt = (rng_.next_float() < 0.2f) ? Zed_Buried : Zed_Normal;
          } else zt = static_cast<ZedType>(rng_.next_int(0, Zed_ArmyShooter));
          zombies_.spawn(zt, spawn_at);
        }
      }
      if (!used_poi) {
        const float ang = rng_.next_float() * 6.28318f;
        const float dist = 220.f + rng_.next_float() * 180.f;
        const ZedType zt = static_cast<ZedType>(rng_.next_int(0, Zed_ArmyShooter));
        zombies_.spawn(zt, Vec2(player_.pos.x + std::cos(ang) * dist,
                                player_.pos.y + std::sin(ang) * dist));
      }
    }
    if (timers_.consume_loot_respawn()) {
      // Respawn at a random POI (typed table) + one near-player pile
      if (mapgen_.location_count() > 0) {
        const MapLocation& L = mapgen_.location(rng_.next_int(0, mapgen_.location_count() - 1));
        const Vec2 p(L.pos.x + rng_.range(-30.f, 30.f), L.pos.y + rng_.range(-30.f, 30.f));
        switch (L.type) {
        case Loc_Military: loot_.spawn_military(p, rng_); break;
        case Loc_Hospital: loot_.spawn_hospital(p, rng_); break;
        case Loc_GasStation: loot_.spawn_gas_station(p, rng_); break;
        case Loc_Helicrash: loot_.spawn_helicrash(p, rng_); break;
        case Loc_Police: loot_.spawn_police(p, rng_); break;
        case Loc_Firestation: loot_.spawn_firestation(p, rng_); break;
        case Loc_Church: loot_.spawn_church(p, rng_); break;
        case Loc_Secret: loot_.spawn_secret(p, rng_); break;
        default: loot_.spawn_civilian(p, rng_); break;
        }
      }
      const float ang = rng_.next_float() * 6.28318f;
      const float dist = 100.f + rng_.next_float() * 220.f;
      loot_.spawn_random(Vec2(player_.pos.x + std::cos(ang) * dist,
                              player_.pos.y + std::sin(ang) * dist), rng_);
    }
    if (timers_.consume_airdrop()) {
      last_airdrop_pos_ = Vec2(player_.pos.x + rng_.range(-500.f, 500.f),
                               player_.pos.y + rng_.range(-500.f, 500.f));
      loot_.spawn_airdrop(last_airdrop_pos_, rng_);
      airdrop_alert_ = 12.f;
      if (audio_) audio_->play("airdrop", 0.85f);
    }
    if (airdrop_alert_ > 0.f) airdrop_alert_ -= dt;
    if (radio_t_ > 0.f) radio_t_ -= dt;

    // Wolf howl cue when pack is close
    if (howl_cd_ > 0.f) howl_cd_ -= dt;
    if (howl_cd_ <= 0.f && audio_) {
      bool near_wolf = false;
      for (int ai = 0; ai < animals_.count(); ++ai) {
        Animal* a = animals_.at(ai);
        if (!a->alive) continue;
        if (a->type != Animal_Wolf && a->type != Animal_Dog) continue;
        if (dist2(a->pos, player_.pos) < 220.f * 220.f) { near_wolf = true; break; }
      }
      if (near_wolf) {
        audio_->play("howl", 0.55f);
        howl_cd_ = 14.f + rng_.next_float() * 10.f;
      }
    }

    // Infected cough — alerts nearby zeds slightly via hear (audio cue)
    if (cough_cd_ > 0.f) cough_cd_ -= dt;
    if (cough_alert_t_ > 0.f) cough_alert_t_ -= dt;
    if (player_.survival.infected && cough_cd_ <= 0.f && audio_) {
      audio_->play("cough", 0.4f);
      cough_cd_ = 8.f + rng_.next_float() * 10.f;
      cough_alert_t_ = 2.5f;
      // Cough noise: nudge nearby zeds + wolves/dogs (blood/scent)
      for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
        if (!z->alive) continue;
        if (dist2(z->pos, player_.pos) < 160.f * 160.f) {
          const float dx = player_.pos.x - z->pos.x;
          const float dy = player_.pos.y - z->pos.y;
          const float len = std::sqrt(dx * dx + dy * dy);
          if (len > 1.f) {
            z->pos.x += (dx / len) * 12.f;
            z->pos.y += (dy / len) * 12.f;
          }
        }
      }
      for (int ai = 0; ai < animals_.count(); ++ai) {
        Animal* a = animals_.at(ai);
        if (!a->alive) continue;
        if (a->type != Animal_Wolf && a->type != Animal_Dog) continue;
        if (dist2(a->pos, player_.pos) > 200.f * 200.f) continue;
        const float dx = player_.pos.x - a->pos.x;
        const float dy = player_.pos.y - a->pos.y;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len > 1.f) {
          a->pos.x += (dx / len) * 18.f;
          a->pos.y += (dy / len) * 18.f;
        }
      }
    }

    // Bleed drips on ground while bleeding
    if (bleed_drip_cd_ > 0.f) bleed_drip_cd_ -= dt;
    if (player_.survival.bleeding > 0.f && bleed_drip_cd_ <= 0.f) {
      bleed_drip_cd_ = 0.35f;
      BloodDrip* d = 0;
      for (int i = 0; i < blood_drip_count_; ++i) {
        if (blood_drips_[i].life <= 0.f) { d = &blood_drips_[i]; break; }
      }
      if (!d && blood_drip_count_ < kMaxBloodDrips) d = &blood_drips_[blood_drip_count_++];
      if (d) {
        d->pos = Vec2(player_.pos.x + rng_.range(-4.f, 4.f),
                      player_.pos.y + rng_.range(-4.f, 4.f));
        d->life = 8.f;
      }
    }
    for (int i = 0; i < blood_drip_count_; ++i) {
      if (blood_drips_[i].life > 0.f) blood_drips_[i].life -= dt;
    }

    update_traps(dt);
    update_crops(dt);
    update_fx(dt);
    dmg_hints_.update(dt);

    // Screamer also agitates nearby wolves
    if (zombies_.scream_active()) {
      const Vec2 sp = zombies_.scream_pos();
      for (int ai = 0; ai < animals_.count(); ++ai) {
        Animal* a = animals_.at(ai);
        if (!a->alive) continue;
        if (a->type != Animal_Wolf && a->type != Animal_Dog) continue;
        if (dist2(a->pos, sp) < 380.f * 380.f) {
          // Nudge toward player (aggro)
          const float dx = player_.pos.x - a->pos.x;
          const float dy = player_.pos.y - a->pos.y;
          const float len = std::sqrt(dx * dx + dy * dy);
          if (len > 1.f) {
            a->pos.x += (dx / len) * a->speed * dt * 0.5f;
            a->pos.y += (dy / len) * a->speed * dt * 0.5f;
          }
        }
      }
    }
    Vec2 prev_animal[kMaxAnimals];
    Vec2 prev_bandit[kMaxBandits];
    for (int i = 0; i < animals_.count(); ++i) prev_animal[i] = animals_.at(i)->pos;
    for (int i = 0; i < bandits_.count(); ++i) prev_bandit[i] = bandits_.at(i)->pos;
    float animal_dmg = animals_.update(dt, player_.pos, player_.survival.bleeding > 0.f);
    float bandit_dmg = bandits_.update(dt, player_.pos);
    if (!in_bunker_) {
      for (int i = 0; i < animals_.count(); ++i) {
        Animal* a = animals_.at(i);
        if (a->type == Animal_Crow) continue; // flying flock, not a Bullet mover in C2
        npc_forest_block(a->pos, prev_animal[i]);
      }
      for (int i = 0; i < bandits_.count(); ++i) npc_forest_block(bandits_.at(i)->pos, prev_bandit[i]);
    }
    apply_incoming_damage(animal_dmg + bandit_dmg,
                          animal_dmg > 0.f || bandit_dmg > 0.f);
    update_army_shooters(dt);
    update_bandit_guns(dt);

    if (timers_.consume_bot_respawn() && bandits_.alive_count() < 4) {
      Vec2 spawn = player_.pos;
      bool near_hot = false;
      float gun_bias = 0.5f;
      for (int i = 0; i < mapgen_.location_count(); ++i) {
        const MapLocation& L = mapgen_.location(i);
        if (L.type != Loc_Military && L.type != Loc_Police) continue;
        if (dist2(L.pos, player_.pos) < 700.f * 700.f) {
          spawn = Vec2(L.pos.x + rng_.range(-80.f, 80.f), L.pos.y + rng_.range(-80.f, 80.f));
          near_hot = true;
          gun_bias = (L.type == Loc_Military) ? 0.7f : 0.8f; // police bandits often armed
          break;
        }
      }
      if (!near_hot) {
        const float ang = rng_.next_float() * 6.28318f;
        const float dist = 280.f + rng_.next_float() * 200.f;
        spawn = Vec2(player_.pos.x + std::cos(ang) * dist,
                     player_.pos.y + std::sin(ang) * dist);
      }
      bandits_.spawn(spawn, rng_.next_float() < gun_bias);
    }
    const int animal_cap = 10;
    if (timers_.consume_animal_respawn() && animals_.alive_count() < animal_cap) {
      const float ang = rng_.next_float() * 6.28318f;
      const float dist = 260.f + rng_.next_float() * 220.f;
      const Vec2 base(player_.pos.x + std::cos(ang) * dist,
                      player_.pos.y + std::sin(ang) * dist);
      const float roll = rng_.next_float();
      const float wolf_chance = 0.45f;
      if (roll < wolf_chance && animals_.alive_of_type(Animal_Wolf) < 7) {
        // Wolf pack (2–3)
        const int pack = 2 + rng_.next_int(0, 1);
        for (int i = 0; i < pack; ++i) {
          animals_.spawn(Animal_Wolf, Vec2(base.x + rng_.range(-40.f, 40.f),
                                           base.y + rng_.range(-40.f, 40.f)));
        }
      } else if (roll < wolf_chance + 0.25f) {
        // Deer pair common
        animals_.spawn(Animal_Deer, base);
        if (rng_.next_float() < 0.45f)
          animals_.spawn(Animal_Deer, Vec2(base.x + rng_.range(-30.f, 30.f),
                                           base.y + rng_.range(-30.f, 30.f)));
      } else if (roll < wolf_chance + 0.37f) {
        // Rabbit warren (2–4)
        const int n = 2 + rng_.next_int(0, 2);
        for (int ri = 0; ri < n; ++ri) {
          animals_.spawn(Animal_Rabbit, Vec2(base.x + rng_.range(-25.f, 25.f),
                                             base.y + rng_.range(-25.f, 25.f)));
        }
      } else if (roll < wolf_chance + 0.48f) {
        // Crow flock
        const int n = 3 + rng_.next_int(0, 2);
        for (int ci = 0; ci < n; ++ci) {
          animals_.spawn(Animal_Crow, Vec2(base.x + rng_.range(-50.f, 50.f),
                                           base.y + rng_.range(-50.f, 50.f)));
        }
      } else {
        // Feral dogs often come in pairs
        animals_.spawn(Animal_Dog, base);
        if (rng_.next_float() < 0.55f)
          animals_.spawn(Animal_Dog, Vec2(base.x + rng_.range(-35.f, 35.f),
                                          base.y + rng_.range(-35.f, 35.f)));
      }
    }
    mark_sector_visited();
  }

  // Zeds update in overworld and bunker (bunker dens use same system)
  {
    float zed_dmg = 0.f;
    // Standing still / walking quieter than sprint; crouch quieter still
    const bool loud = player_.running && !in.crouch;
    // Storm (heavy rain) makes zeds slightly more aggressive
    const bool storm = !in_bunker_ && weather_.rain > 0.85f;
    Vec2 prev_zed[kMaxZombies];
    {
      int zi = 0;
      for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) prev_zed[zi++] = z->pos;
    }
    zombies_.update(dt, player_.pos, zed_dmg, loud || storm, in.crouch,
                    player_.survival.bleeding > 0.f,
                    cough_alert_t_ > 0.f || storm);
    if (!in_bunker_) {
      int zi = 0;
      for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z, ++zi)
        if (z->alive) npc_forest_block(z->pos, prev_zed[zi]);
    }
    // Vehicle runover (car/bike) — damages zeds/bandits/wolves + vehicle wear
    if (!in_bunker_ && vehicles_.player_driving()) {
      Vehicle* v = vehicles_.driven();
      const float hit_r = (v && v->type == Vehicle_Bike) ? 22.f : 32.f;
      const float dps = (v && v->type == Vehicle_Bike) ? 18.f : 45.f;
      for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
        if (!z->alive) continue;
        if (dist2(z->pos, player_.pos) > hit_r * hit_r) continue;
        z->hp -= dps * dt;
        if (v) { v->hp -= 4.f * dt; if (v->hp < 0.f) v->hp = 0.f; }
        if (z->hp <= 0.f) {
          on_zed_killed(*z);
          if (audio_) audio_->play("hit", 0.5f);
        }
      }
      for (Bandit* b = bandits_.begin(); b != bandits_.end(); ++b) {
        if (!b->alive) continue;
        if (dist2(b->pos, player_.pos) > hit_r * hit_r) continue;
        b->hp -= dps * dt;
        if (v) { v->hp -= 5.f * dt; if (v->hp < 0.f) v->hp = 0.f; }
        if (b->hp <= 0.f) {
          b->alive = false;
          player_.bandits_killed += 1;
          spawn_corpse(Corpse_Bandit, b->pos, 0.f, 0);
          loot_.spawn_bandit(b->pos, rng_);
          if (audio_) audio_->play("hit", 0.55f);
        }
      }
      for (int ai = 0; ai < animals_.count(); ++ai) {
        Animal* a = animals_.at(ai);
        if (!a->alive) continue;
        if (a->type != Animal_Wolf && a->type != Animal_Dog) continue;
        if (dist2(a->pos, player_.pos) > hit_r * hit_r) continue;
        a->hp -= dps * 0.8f * dt;
        if (a->hp <= 0.f) on_animal_killed(*a);
      }
    }
    if (in_bunker_) {
      for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
        if (!z->alive) continue;
        if (!bunker_.is_blocked_world(z->pos.x, z->pos.y)) continue;
        // Slide out of wall toward player / previous free cell
        const float dx = player_.pos.x - z->pos.x;
        const float dy = player_.pos.y - z->pos.y;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len > 1.f) {
          z->pos.x -= (dx / len) * 8.f;
          z->pos.y -= (dy / len) * 8.f;
        }
        if (bunker_.is_blocked_world(z->pos.x, z->pos.y)) {
          z->pos.x -= 6.f; z->pos.y -= 6.f;
        }
      }
    }
    // Buried ambush pop SFX
    for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
      if (!z->alive || z->type != Zed_Buried) continue;
      if (z->ambush_boost > 1.55f && audio_) {
        audio_->play("hit", 0.65f);
        break;
      }
    }
    if (zombies_.scream_active() && !prev_scream_ && audio_) {
      audio_->play("hit", 0.95f);
    }
    prev_scream_ = zombies_.scream_active();
    apply_incoming_damage(zed_dmg, zed_dmg > 0.f);
  }

  bullets_.update(dt);
  if (in_bunker_) {
    for (Bullet* b = bullets_.begin(); b != bullets_.end(); ++b) {
      if (!b->alive) continue;
      if (bunker_.is_blocked_world(b->pos.x, b->pos.y)) {
        if (b->explosive && b->from_player) {
          explode_at(b->pos, b->damage, b->leaves_fire);
          b->explosive = false;
          b->leaves_fire = false;
        }
        b->alive = false;
      }
    }
  }
  resolve_bullet_hits();

  if (in.attack_pressed && attack_cd_ <= 0.f) {
    const InvSlot& eq = inventory_.slot(equipped_slot_);
    const WeaponDef* w = eq.empty() ? 0 : find_weapon(eq.id);
    if (w && !w->stats.is_melee) try_ranged_attack();
    else try_melee_attack();
  }

  if (in.interact_pressed && !vehicle_interact) {
    const int loot_before = loot_.count();
    bool picked = false;
    {
      const float reach = 40.f;
      for (int i = 0; i < loot_.count(); ++i) {
        LootPile* p = loot_.at(i);
        if (!p->active) continue;
        if (dist2(p->pos, player_.pos) > reach * reach) continue;
        picked = true;
        break;
      }
    }
    try_pickup_loot();
    try_bunker_enter_exit();
    if (!picked && !in_bunker_) {
      // Fishing stub with rod
      bool has_rod = false;
      for (int i = 0; i < inventory_.slot_count(); ++i) {
        if (inventory_.slot(i).id == 87 || inventory_.slot(i).id == 86) { has_rod = true; break; }
      }
      const float fish_chance = player_.perk_farmer ? 0.45f : 0.35f;
      const float bush_chance = player_.perk_farmer ? 0.4f : 0.25f;
      // Shovel dig (prefer over axe when shovel equipped)
      const ItemId eqid = inventory_.slot(equipped_slot_).id;
      bool has_shovel = (eqid == 201);
      if (!has_shovel) {
        for (int i = 0; i < inventory_.slot_count(); ++i) {
          if (inventory_.slot(i).id == 201) { has_shovel = true; break; }
        }
      }
      if (has_shovel && eqid == 201) {
        // Uncover buried zeds nearby, else forage dirt finds
        bool dug_zed = false;
        for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
          if (!z->alive || z->type != Zed_Buried) continue;
          if (dist2(z->pos, player_.pos) > 40.f * 40.f) continue;
          if (z->ambush_boost <= 0.f) {
            z->ambush_boost = 1.f; // pop ambush
            if (z->aggro_radius < 320.f) z->aggro_radius = 320.f;
            dug_zed = true;
            if (audio_) audio_->play("hit", 0.55f);
            break;
          }
        }
        if (!dug_zed) {
          const float r = rng_.next_float();
          bool found = true;
          if (r < 0.28f) inventory_.add_item(47 + rng_.next_int(0, 1), 1, items_);
          else if (r < 0.42f) inventory_.add_item(97, 1, items_);
          else if (r < 0.6f) inventory_.add_item(38, 1, items_);
          else if (r < 0.72f) inventory_.add_item(26, 1, items_);
          else if (r < 0.82f) inventory_.add_item(27, 1, items_);
          else found = false;
          if (audio_) audio_->play("melee_swing", found ? 0.4f : 0.25f);
          if (found && !player_.survival.infected && rng_.next_float() < 0.04f)
            player_.survival.infected = true;
        }
      } else {
      // Chop wood with axe / hatchet equipped
      bool has_axe = false;
      if (eqid == 202 || eqid == 204 || eqid == 206) has_axe = true;
      if (!has_axe) {
        for (int i = 0; i < inventory_.slot_count(); ++i) {
          const ItemId id = inventory_.slot(i).id;
          if (id == 202 || id == 204) { has_axe = true; break; }
        }
      }
      if (has_axe) {
        // Prefer chopping a nearby tree stub (removes it); else ambient wood chance
        int near_tree = -1;
        float best = 55.f * 55.f;
        for (int ti = 0; ti < tree_count_; ++ti) {
          const float d2t = dist2(trees_[ti], player_.pos);
          if (d2t < best) { best = d2t; near_tree = ti; }
        }
        if (near_tree >= 0) {
          // Fell tree → sticks / piles / rare ashwood
          const float r = rng_.next_float();
          if (r < 0.2f) inventory_.add_item(37, 1, items_);
          else if (r < 0.55f) inventory_.add_item(50, 1, items_);
          else inventory_.add_item(38, 2 + (player_.perk_farmer ? 1 : 0), items_);
          trees_[near_tree] = trees_[tree_count_ - 1];
          tree_count_ -= 1;
          // Axe wear: ~6% break on chop (duct tape helps)
          // Tool break-on-chop: no C2 evidence in 1.4.1 export — not invented.
          if (audio_) audio_->play("melee_swing", 0.55f);
        } else if (rng_.next_float() < 0.35f) {
          inventory_.add_item(38, 1, items_);
          if (audio_) audio_->play("melee_swing", 0.4f);
        }
      } else if (has_rod) {
        // Fishing only near map-edge water band
        const float margin = 400.f;
        const bool near_water =
          player_.pos.x < margin || player_.pos.y < margin ||
          player_.pos.x > kMapLayoutW - margin || player_.pos.y > kMapLayoutH - margin;
        float fc = fish_chance;
        // Spinning rod (86) better than basic (87)
        bool spinning = false;
        for (int i = 0; i < inventory_.slot_count(); ++i) {
          if (inventory_.slot(i).id == 86) { spinning = true; break; }
        }
        if (spinning) fc *= 1.35f;
        bool used_bait = false;
        if (near_water) {
          // Bait: berry / apple / raw meat — spend 1 per cast near water
          for (int i = 0; i < inventory_.slot_count(); ++i) {
            const ItemId bid = inventory_.slot(i).id;
            if (bid == 6 || bid == 27 || bid == 73 || bid == 74 || bid == 60 || bid == 71) {
              if (inventory_.slot(i).count > 0 && inventory_.remove_at(i, 1)) {
                used_bait = true;
                fc *= 1.7f;
                break;
              }
            }
          }
        }
        if (weather_.rain > 0.7f) fc *= 0.55f; // rain makes fishing worse
        if (near_water && rng_.next_float() < fc) {
          // Catch mix: perch common; bait/spinning → herring/salmon/ruffe
          ItemId catch_id = 94; // perch
          const float roll = rng_.next_float();
          if (used_bait || spinning) {
            if (roll < 0.12f) catch_id = 92;      // salmon rare
            else if (roll < 0.28f) catch_id = 91; // herring
            else if (roll < 0.48f) catch_id = 93; // ruffe
            else catch_id = 94;
          } else {
            if (roll < 0.18f) catch_id = 93;
            else if (roll < 0.28f) catch_id = 91;
            else catch_id = 94;
          }
          inventory_.add_item(catch_id, 1, items_);
          player_.fish_caught += 1;
          spawn_fx(Fx_Fish,
                   Vec2(player_.pos.x + 10.f, player_.pos.y + 4.f), 0.f, 0.7f);
          if (audio_) audio_->play("pickup", 0.4f);
        } else if (!near_water && audio_) {
          audio_->play("melee_swing", 0.15f); // cast on dry land — nothing
        }
      } else if (!has_rod && !has_axe) {
        // Harvest ready crop if standing on it
        bool harvested = false;
        for (int ci = 0; ci < crop_count_; ++ci) {
          CropPlot& c = crops_[ci];
          if (!c.active || !c.ready) continue;
          if (dist2(c.pos, player_.pos) > 28.f * 28.f) continue;
          const int n = 1 + (player_.perk_farmer ? 1 : 0);
          inventory_.add_item(c.crop_id, n, items_);
          c.active = false;
          player_.forage_count += 1;
          if (player_.forage_count >= 8) player_.perk_farmer = true;
          harvested = true;
          if (audio_) audio_->play("pickup", 0.45f);
          break;
        }
        const ItemId seed_eq = inventory_.slot(equipped_slot_).id;
        if (!harvested && (seed_eq == 68 || seed_eq == 47 || seed_eq == 48)) {
          try_plant_seed(seed_eq);
        } else if (!harvested) {
        // Search building interior near POI (typed scraps + crowbar pry)
        const bool has_crowbar = (inventory_.slot(equipped_slot_).id == 205);
        bool searched = false;
        for (int li = 0; li < mapgen_.location_count(); ++li) {
          const MapLocation& L = mapgen_.location(li);
          if (L.type == Loc_BunkerEntrance || L.type == Loc_Helicrash) continue;
          if (dist2(L.pos, player_.pos) > 70.f * 70.f) continue;
          indoor_t_ = 40.f;
          searched = true;
          const float find_p = has_crowbar ? 0.72f : 0.4f;
          if (rng_.next_float() < find_p) {
            ItemId found = 1;
            if (L.type == Loc_Hospital) {
              static const ItemId kH[] = { 18, 19, 28, 29, 45, 46, 35 };
              found = kH[rng_.next_int(0, 6)];
            } else if (L.type == Loc_Police) {
              static const ItemId kP[] = { 12, 69, 18, 42, 36, 151 };
              found = kP[rng_.next_int(0, 5)];
            } else if (L.type == Loc_Firestation) {
              static const ItemId kF[] = { 204, 205, 53, 59, 24, 50 };
              found = kF[rng_.next_int(0, 5)];
            } else if (L.type == Loc_Military) {
              static const ItemId kM[] = { 11, 15, 18, 69, 43, 80 };
              found = kM[rng_.next_int(0, 5)];
            } else if (L.type == Loc_GasStation) {
              static const ItemId kG[] = { 53, 59, 8, 22, 39, 200 };
              found = kG[rng_.next_int(0, 5)];
            } else if (L.type == Loc_Church) {
              static const ItemId kC[] = { 18, 25, 49, 26, 6, 54 };
              found = kC[rng_.next_int(0, 5)];
            } else {
              static const ItemId kIndoor[] = { 1, 8, 18, 26, 30, 39, 42, 54 };
              found = kIndoor[rng_.next_int(0, 7)];
            }
            inventory_.add_item(found, 1, items_);
            if (has_crowbar && rng_.next_float() < 0.35f)
              inventory_.add_item(rng_.next_float() < 0.5f ? 59 : 36, 1, items_); // toolbox / cleaning
          }
          if (audio_) audio_->play(has_crowbar ? "hit" : "pickup", has_crowbar ? 0.45f : 0.35f);
          break;
        }
        if (!searched && weather_.rain > 0.7f && player_.survival.water < 95.f) {
          // Drink rain — dirty water
          player_.survival.water += 8.f;
          if (player_.survival.water > 100.f) player_.survival.water = 100.f;
          if (!(wearing_gasmask() && gasmask_filter_ > 5.f) &&
              !player_.survival.infected && rng_.next_float() < 0.2f) {
            player_.survival.infected = true;
          }
          if (audio_) audio_->play("drink", 0.3f);
        } else if (!searched && rng_.next_float() < bush_chance) {
          // Bush forage: sticks / berries / rags
          static const ItemId kBush[] = { 38, 27, 26, 33, 97, 73 };
          ItemId id = kBush[rng_.next_int(0, 5)];
          inventory_.add_item(id, 1 + (player_.perk_farmer ? 1 : 0), items_);
          player_.forage_count += 1;
          if (player_.forage_count >= 8) player_.perk_farmer = true;
          if (audio_) audio_->play("bush", 0.4f);
        }
        } // end non-seed interact
      }
      } // end axe/rod/forage (not shovel-equipped dig)
    }
    (void)loot_before;
  }

  {
    bool near_bunker = false;
    bool near_hospital = false;
    bool near_police = false;
    bool near_fire = false;
    bool near_church = false;
    for (int i = 0; i < mapgen_.location_count(); ++i) {
      const MapLocation& L = mapgen_.location(i);
      if (L.type == Loc_BunkerEntrance && dist2(L.pos, player_.pos) < 80.f * 80.f)
        near_bunker = true;
      if (L.type == Loc_Hospital && dist2(L.pos, player_.pos) < 90.f * 90.f)
        near_hospital = true;
      if (L.type == Loc_Police && dist2(L.pos, player_.pos) < 90.f * 90.f)
        near_police = true;
      if (L.type == Loc_Firestation && dist2(L.pos, player_.pos) < 90.f * 90.f)
        near_fire = true;
      if (L.type == Loc_Church && dist2(L.pos, player_.pos) < 90.f * 90.f)
        near_church = true;
    }
    if (quest_.tick(inventory_, items_, bandits_alive_near(quest_.q_anchor, 300.f))) {
      if (audio_) audio_->play("craft", 0.9f);
      quest_.spawn_reward_box(loot_, player_.pos);
      quest_.finish_reward();
      quest_.generate_quest(rng_, mapgen_, 1);
    }  }

  // (R used to also auto-craft the first possible recipe; crafting now goes through the
  //  craft menu only - PSP Square is reload.)
  if (in.drop_pressed) try_drop_equipped();
  if (in.mag_reload_pressed) try_mag_reload();
  if (in.throw_pressed) try_throw_projectile();
  if (in.torch_pressed) {
    // T: place campfire (kit or matches+fuel), else toggle torch/headlamp
    bool can_fire = false;
    bool has_match = false, has_fuel = false;
    int stick_n = 0;
    has_headlamp_ = false;
    for (int i = 0; i < inventory_.slot_count(); ++i) {
      const ItemId id = inventory_.slot(i).id;
      if (id == 23 && inventory_.slot(i).count > 0) can_fire = true;
      if (id == 24 && inventory_.slot(i).count > 0) has_match = true;
      if ((id == 38 || id == 50) && inventory_.slot(i).count > 0) has_fuel = true;
      if (id == 38) stick_n += inventory_.slot(i).count;
      if (id == 260 && inventory_.slot(i).count > 0) has_headlamp_ = true;
    }
    if (has_match && has_fuel) can_fire = true;
    if (stick_n >= 2) can_fire = true; // hand-drill attempt
    for (int wi = 0; wi < inventory_.worn_count(); ++wi) {
      if (inventory_.worn_at(wi) == 260) has_headlamp_ = true;
    }
    if (can_fire) try_place_campfire();
    else if (player_.torch_on) {
      player_.torch_on = false;
    } else if (torch_battery_ > 1.f) {
      player_.torch_on = true;
      (void)has_headlamp_;
    } else if (audio_) {
      audio_->play("melee_swing", 0.2f);
    }
  }
  sync_bag_overlay_from_ui();

  if (in.save_pressed) save_game_to_disk();
  if (in.load_pressed) {
    if (!load_game_from_disk()) show_toast("NO SAVE FOUND");
    return;
  }

  if (in.inventory_pressed) use_equipped_item();

  if (player_.survival.is_dead() && state_ != AppState_Dead) {
    // Drop inventory as corpse piles (visible until return to menu)
    Vec2 drop = player_.pos;
    for (int i = 0; i < inventory_.slot_count(); ++i) {
      const InvSlot& sl = inventory_.slot(i);
      if (sl.empty()) continue;
      loot_.spawn_item(Vec2(drop.x + rng_.range(-20.f, 20.f),
                            drop.y + rng_.range(-20.f, 20.f)),
                       sl.id, sl.count);
    }
    write_death_log();
    state_ = AppState_Dead;
    state_timer_ = 0.f;
    in_bunker_ = false;
    if (audio_) audio_->play("death", 0.8f);
  }
}

void Game::use_equipped_item() {
    // USE always acts on the SELECTED slot only (equipped_slot_; use_bag_slot sets it
    // temporarily for the inventory focus). Bug 2026-10-19: this used to scan the whole
    // inventory for gasoline/toolbox and fall back to the first consumable.
    if (equipped_slot_ < 0 || equipped_slot_ >= inventory_.slot_count()) return;
    // Gasoline (53) refuel / Car toolbox (59) repair
    bool vehicle_item = false;
    {
      const int i = equipped_slot_;
      const ItemId id = inventory_.slot(i).id;
      if (id == 53) {
        Vehicle* v = vehicles_.driven();
        if (!v) {
          for (int vi = 0; vi < vehicles_.count(); ++vi) {
            Vehicle* cand = vehicles_.at(vi);
            if (!cand->active) continue;
            if (dist2(cand->pos, player_.pos) < 50.f * 50.f) { v = cand; break; }
          }
        }
        if (v && v->fuel < 100.f) {
          v->fuel += 40.f;
          if (v->fuel > 100.f) v->fuel = 100.f;
          inventory_.remove_at(i, 1);
          if (audio_) audio_->play("craft", 0.5f);
          vehicle_item = true;
        }
      } else if (id == 59) {
        if (vehicles_.try_repair(player_.pos, 35.f)) {
          inventory_.remove_at(i, 1);
          if (audio_) audio_->play("craft", 0.55f);
          vehicle_item = true;
        }
      }
    }
    if (!vehicle_item) {
      bool used_special = false;
      const InvSlot& eq = inventory_.slot(equipped_slot_);
      if (!eq.empty()) {
        if (eq.id == 265) { // NVG toggle (shares torch battery)
          if (player_.nvg_on) {
            player_.nvg_on = false;
            used_special = true;
          } else if (torch_battery_ > 1.f) {
            player_.nvg_on = true;
            used_special = true;
            if (audio_) audio_->play("craft", 0.4f);
          } else if (audio_) {
            audio_->play("melee_swing", 0.2f);
            used_special = true;
          }
        } else if (eq.id == 20) { // Flare — bright short beacon (longer at night)
          if (inventory_.remove_at(equipped_slot_, 1)) {
            const float life = weather_.is_night ? 90.f : 45.f;
            campfires_.place(player_.pos, life);
            flare_t_ = life;
            flare_pos_ = player_.pos;
            // Night flare briefly lights torch without battery drain
            if (weather_.is_night) {
              player_.torch_on = true;
              torch_battery_ += 5.f;
              if (torch_battery_ > 100.f) torch_battery_ = 100.f;
            }
            used_special = true;
            if (audio_) audio_->play("craft", 0.6f);
          }
        } else if (eq.id == 22) { // Battery — refill torch/NVG (+ gasmask filter)
          if (inventory_.remove_at(equipped_slot_, 1)) {
            torch_battery_ += 50.f;
            if (torch_battery_ > 100.f) torch_battery_ = 100.f;
            gasmask_filter_ += 40.f;
            if (gasmask_filter_ > 100.f) gasmask_filter_ = 100.f;
            used_special = true;
            if (audio_) audio_->play("craft", 0.45f);
          }
        } else if (eq.id == 456) { // Civilian tent — long shelter fire marker
          if (inventory_.remove_at(equipped_slot_, 1)) {
            campfires_.place(player_.pos, 600.f);
            tent_warmth_t_ = 600.f;
            used_special = true;
            if (audio_) audio_->play("craft", 0.7f);
          }
        } else if (eq.id == 250) { // Gasmask — wear
          used_special = inventory_.wear_clothing(equipped_slot_, items_);
          if (used_special && audio_) audio_->play("craft", 0.35f);
        } else if (eq.id == 45) { // Morphine — heal + adrenaline sprint
          if (inventory_.use_consumable(equipped_slot_, player_.survival, items_, &rng_)) {
            adrenaline_t_ = 18.f;
            used_special = true;
            if (audio_) audio_->play("craft", 0.5f);
          }
        } else if (eq.id == 25) { // Whiskey — drink + brief dutch courage
          if (inventory_.use_consumable(equipped_slot_, player_.survival, items_, &rng_)) {
            adrenaline_t_ = 6.f;
            used_special = true;
            if (audio_) audio_->play("pickup", 0.45f);
          }
        } else if (eq.id == 36) { // Cleaning kit — clear jam / maintain firearm
          const WeaponDef* ew = find_weapon(eq.id);
          (void)ew;
          bool has_gun = false;
          for (int i = 0; i < inventory_.slot_count(); ++i) {
            const WeaponDef* w = find_weapon(inventory_.slot(i).id);
            if (w && !w->stats.is_melee && w->mag_capacity > 0) { has_gun = true; break; }
          }
          if ((gun_jammed_ || has_gun) && inventory_.remove_at(equipped_slot_, 1)) {
            gun_jammed_ = false;
            used_special = true;
            if (audio_) audio_->play("craft", 0.55f);
          }
        } else if (eq.id == 35) { // Sewing kit — dry/repair clothes
          if (wetness_ > 5.f || inventory_.worn_count() > 0) {
            if (inventory_.remove_at(equipped_slot_, 1)) {
              wetness_ -= 40.f;
              if (wetness_ < 0.f) wetness_ = 0.f;
              melee_break_mul_ = 0.5f; // patched gear
              used_special = true;
              if (audio_) audio_->play("craft", 0.5f);
            }
          }
        } else if (eq.id == 63) { // Energy drink — water + short sprint
          if (inventory_.use_consumable(equipped_slot_, player_.survival, items_, &rng_)) {
            adrenaline_t_ = 10.f;
            used_special = true;
            if (audio_) audio_->play("drink", 0.4f);
          }
        } else if (eq.id == 57) { // Bear trap — place
          used_special = try_place_trap(0);
        } else if (eq.id == 70) { // Barbed wire — place zone
          used_special = try_place_trap(1);
        } else if (eq.id == 50) { // Woodpile — build fence (needs 2 sticks)
          used_special = try_place_trap(2);
        } else if (eq.id == 99) { // Claymore — place proximity mine
          used_special = try_place_trap(3);
        } else if (eq.id == 53) { // Gasoline — boost nearby fire or splash puddle
          if (inventory_.remove_at(equipped_slot_, 1)) {
            if (campfires_.boost_near(player_.pos, 120.f)) {
              used_special = true;
              if (audio_) audio_->play("craft", 0.55f);
            } else {
              // Fuel puddle — short intense fire
              campfires_.place(player_.pos, 45.f);
              used_special = true;
              if (audio_) audio_->play("gunshot", 0.3f);
            }
          }
        } else if (eq.id == 75) { // Radio — airdrop ETA (needs battery)
          if (torch_battery_ > 5.f) {
            torch_battery_ -= 3.f;
            radio_t_ = 6.f;
            used_special = true;
            if (audio_) audio_->play("airdrop", 0.35f);
          } else if (audio_) {
            audio_->play("melee_swing", 0.2f);
            used_special = true;
          }
        } else if (eq.id == 39) {
          // Duct tape melee reinforce: no C2 craft/jam strings — blocked (do not invent).
        } else if (eq.id == 260) { // Headlamp — wear + enable torch
          used_special = inventory_.wear_clothing(equipped_slot_, items_);
          if (used_special) {
            has_headlamp_ = true;
            if (torch_battery_ > 1.f) player_.torch_on = true;
            if (audio_) audio_->play("craft", 0.35f);
          }
        } else if (eq.id == 83) { // Bandolier — wear for ammo stacks + slots
          used_special = inventory_.wear_clothing(equipped_slot_, items_);
          if (used_special && audio_) audio_->play("craft", 0.35f);
        } else if (eq.id == 82) { // Hacksaw — cut nearest wire/fence trap
          float best = 55.f * 55.f;
          int best_i = -1;
          for (int ti = 0; ti < trap_count_; ++ti) {
            if (!traps_[ti].active) continue;
            if (traps_[ti].kind != 1 && traps_[ti].kind != 2) continue;
            const float d2t = dist2(traps_[ti].pos, player_.pos);
            if (d2t < best) { best = d2t; best_i = ti; }
          }
          if (best_i >= 0) {
            traps_[best_i].active = false;
            // Scrap: chance for sticks or wire salvage
            if (traps_[best_i].kind == 2) inventory_.add_item(38, 1, items_);
            else if (rng_.next_float() < 0.4f) inventory_.add_item(70, 1, items_);
            used_special = true;
            if (audio_) audio_->play("melee_swing", 0.5f);
          }
        }
      }
      if (!used_special) {
        const ItemId before = inventory_.slot(equipped_slot_).id;
        const bool was_bleed = player_.survival.bleeding > 0.f;
        auto play_consume = [&](ItemId id) {
          if (!audio_) return;
          // Drinks / medical / food family tags
          if (id == 8 || id == 9 || id == 10 || id == 30 || id == 54 || id == 55 ||
              id == 62 || id == 63 || id == 25)
            audio_->play("drink", 0.45f);
          else if (id == 18 || id == 19 || id == 26 || id == 28 || id == 29 ||
                   id == 45 || id == 46)
            audio_->play("craft", 0.4f);
          else
            audio_->play("eat", 0.4f);
        };
        if (inventory_.use_consumable(equipped_slot_, player_.survival, items_, &rng_)) {
          use_anim_t_ = 0.5f;
          if (before == 45) adrenaline_t_ = 18.f;
          play_consume(before);
          (void)was_bleed;
        }
      }
    }
  }

void Game::update_dead(float dt, const InputState& in) {
  state_timer_ += dt;
  if (in.confirm_pressed || (state_timer_ > 1.5f && in.attack_pressed)) {
    state_ = AppState_Menu;
  }
}

void Game::update(float dt) {
  if (!platform_) return;

  // FPS counter — average over 0.5s windows.
  fps_frames_ += 1;
  fps_accum_ += (dt > 0.f ? dt : 0.f);
  if (fps_accum_ >= 0.5f) {
    fps_ = (fps_accum_ > 1.e-4f) ? (static_cast<float>(fps_frames_) / fps_accum_) : 0.f;
    fps_frames_ = 0;
    fps_accum_ = 0.f;
  }

  InputState in;
  if (!platform_->poll(in)) {
    quit_ = true;
    return;
  }
  compute_nav(in, dt);
  ui_t_ += dt;
  if (toast_t_ > 0.f) toast_t_ -= dt;

  switch (state_) {
  case AppState_Loading:
    state_ = AppState_Menu;
    break;
  case AppState_Menu:
    update_menu(dt, in);
    break;
  case AppState_Playing:
    update_playing(dt, in);
    break;
  case AppState_Paused:
    update_pause(dt, in);
    break;
  case AppState_Dead:
    update_dead(dt, in);
    break;
  case AppState_Tutorial:
    state_ = AppState_Playing;
    break;
  }
}

void Game::render() {
  if (!platform_) return;
  platform_->profile_mark(IPlatform::Prof_Hud); // menus / UI-only frames bill to HUD
  platform_->begin_frame();

  const float cx = static_cast<float>(platform_->screen_w()) * 0.5f;
  const float cy = static_cast<float>(platform_->screen_h()) * 0.5f;
  const float sc = world_to_screen_scale();
  // View-rect culling (2026-10-19): an object is drawn when its screen-space box
  // (centre +- r_px, in screen px) intersects the 480x272 logical screen.
  // Replaces the old fixed world-distance checks / fixed grid windows.
  struct ViewRect {
    float cx, cy;
    Vec2 cam;
    bool vis(const Vec2& p, float s, float r_px) const {
      const float x = cx + (p.x - cam.x) * s, y = cy + (p.y - cam.y) * s;
      return x + r_px >= 0.f && x - r_px <= 2.f * cx && y + r_px >= 0.f && y - r_px <= 2.f * cy;
    }
    // World-grid index range covering the view (+1 cell margin each side).
    int lo(float c, float half, float s, float step) const {
      return static_cast<int>(std::floor((c - half / s) / step)) - 1;
    }
    int hi(float c, float half, float s, float step) const {
      return static_cast<int>(std::floor((c + half / s) / step)) + 1;
    }
  };
  ViewRect view;
  view.cx = cx; view.cy = cy; view.cam = player_.pos;

  if (state_ == AppState_Menu) {
    platform_->clear(0.12f, 0.14f, 0.18f);
    // Backdrop: the original Menu layout (2500×1500 town), slowly panned.
    {
      const Vec2 cam(1250.f + 700.f * std::sin(state_timer_ * 0.03f),
                     750.f + 380.f * std::sin(state_timer_ * 0.021f));
      const float msc = 0.6f;
      menu_tiles_.draw(*platform_, "ground_lvl", cam, msc, cx * 2.f, cy * 2.f);
      menu_tiles_.draw(*platform_, "ground_env", cam, msc, cx * 2.f, cy * 2.f);
      menu_sprites_.draw(*platform_, cam, msc, cx * 2.f, cy * 2.f);
    }
    if (platform_->has_sprite("main_menu_logo-sheet0"))
      platform_->draw_sprite_frame("main_menu_logo-sheet0", cx - 100.f, 14.f, 200.f, 65.f,
                                   0, 0, 400, 130, 0.f, 1.f);
    else
      platform_->draw_text(cx - 40.f, 30.f, "MINI DAYZ", 0.95f, 0.9f, 0.75f);

    render_main_menu();
  } else if (state_ == AppState_Playing || state_ == AppState_Paused) {
    platform_->world_pass_begin();
    platform_->profile_mark(IPlatform::Prof_Ground);
    if (in_bunker_) platform_->clear(0.08f, 0.07f, 0.06f);
    else if (weather_.is_night) platform_->clear(0.05f, 0.06f, 0.08f);
    else platform_->clear(0.14f, 0.18f, 0.11f);

    if (in_bunker_ && bunker_.width > 0) {
      platform_->ground_cache_invalidate();
      // World cell 8×8 (collision); atlas tilemap_bunker 150×300 = 5×10 of 30×30
      const float tile = 8.f;
      const float bsc = 1.0f;
      const bool has_bunk = platform_->has_sprite("tilemap_bunker");
      const int view_x = static_cast<int>(player_.pos.x / tile) - 30;
      const int view_y = static_cast<int>(player_.pos.y / tile) - 17;
      for (int ty = 0; ty < 34; ++ty) {
        for (int tx = 0; tx < 60; ++tx) {
          const int bx = view_x + tx;
          const int by = view_y + ty;
          if (bx < 0 || by < 0 || bx >= bunker_.width || by >= bunker_.height) continue;
          const int tile_id = bunker_.tile_at(bx, by);
          if (tile_id <= 0) continue;
          float sx = cx + (bx * tile - player_.pos.x) * bsc;
          float sy = cy + (by * tile - player_.pos.y) * bsc;
          if (has_bunk) {
            // C2 bunker tileset: tile 0 empty; ids 1..50 → atlas cells 0..49
            int idx = tile_id - 1;
            if (idx < 0) idx = 0;
            if (idx > 49) idx = idx % 50;
            const int ux = (idx % 5) * 30;
            const int uy = (idx / 5) * 30;
            platform_->draw_sprite_frame("tilemap_bunker", sx, sy, tile, tile,
                                        ux, uy, 30, 30, 0.f, 1.f);
          } else {
            float r = 0.28f, g = 0.24f, b = 0.2f;
            if (tile_id == 6 || tile_id == 16 || tile_id == 20 || tile_id == 1) {
              r = 0.18f; g = 0.17f; b = 0.15f;
            } else {
              r = 0.42f; g = 0.36f; b = 0.3f;
            }
            platform_->draw_rect(sx, sy, tile, tile, r, g, b, 1.f);
          }
        }
      }
    } else {
      // Ground: ground_lvl1–4 (60×60) underlay + forest 150×150 + water/sand
      // PSP ground cache (2026-10-19): the platform keeps the ground layer in VRAM and only
      // redraws newly exposed strips; the ground camera is snapped to whole screen pixels.
      // Desktop: ground_snap_px() == 0 -> gcam == player_.pos, ground drawn every frame.
      Vec2 gcam = player_.pos;
      const int gsnap = platform_->ground_snap_px();
      if (gsnap > 0) {
        const float u = static_cast<float>(gsnap);
        gcam.x = std::floor(player_.pos.x * sc / u + 0.5f) * u / sc;
        gcam.y = std::floor(player_.pos.y * sc / u + 0.5f) * u / sc;
      }
      const unsigned gkey = (weather_.is_night ? 1u : 0u);
      if (platform_->ground_cache_begin(gcam.x * sc, gcam.y * sc, gkey))

      {
        const bool has_forest = platform_->has_sprite("tilemap_forest");
        const bool has_water = platform_->has_sprite("tilemap_water");
        const bool has_sand = platform_->has_sprite("tilemap_sand");
        const bool has_g1 = platform_->has_sprite("ground_lvl1_tilemap");
        const bool has_g2 = platform_->has_sprite("ground_lvl2_tilemap");
        const bool has_g3 = platform_->has_sprite("ground_lvl3_tilemap");
        const bool has_g4 = platform_->has_sprite("ground_lvl4_tilemap");
        const bool has_gnd = has_g1 || has_g2 || has_g3 || has_g4;
        const float water_w = 60.f;
        // Each ground_lvl*: 300×300 atlas → 5×5 of 60×60 (C2 tile size).
        // Pick atlas by coarse biome hash so the map isn't one repeating sheet.
        if (has_gnd) {
          const float step = 60.f;
          const float tile = step * sc;
          for (int gx = view.lo(gcam.x, cx, sc, step); gx <= view.hi(gcam.x, cx, sc, step); ++gx) {
            for (int gy = view.lo(gcam.y, cy, sc, step); gy <= view.hi(gcam.y, cy, sc, step); ++gy) {
              const float wx = (gx + 0.5f) * step;
              const float wy = (gy + 0.5f) * step;
              const float margin = 400.f;
              if (wx < margin || wy < margin ||
                  wx > kMapLayoutW - margin || wy > kMapLayoutH - margin) continue;
              const float sx = cx + (wx - gcam.x) * sc;
              const float sy = cy + (wy - gcam.y) * sc;
              // Atlas 5x5 of 60px: 3x3 dirt->grass autotile at (0..2,0..2), 2x2 hole at (3..4,0..1),
              // dirt/grass detail cells in rows 3..4, cells (3..4,2..4) are EMPTY (transparent).
              // Old code tiled gx%5/gy%5 over the whole atlas -> transparent holes + flat
              // stretched-looking patches. Use the seamless grass centre (1,1) with rare detail variants.
              // C2 (data.js Map generation): one ground tilemap per map cell, type chosen by
              // the global CurrentLevel (0 -> ground_lvl1 ... 3 -> ground_lvl4); CurrentLevel is
              // 0 after Restart_game and only Level_change adds 1 (not ported -> always lvl1).
              // The tilemap template (Map layout instance) is tile 6 = atlas cell (1,1) on 5975
              // of 5985 cells, so the whole map is one uniform ground - no per-cell biome mix.
              const int ux = 1, uy = 1;
              const int lvl = 0; // C2 CurrentLevel
              const char* gspr = "ground_lvl1_tilemap";
              if (lvl == 1 && has_g2) gspr = "ground_lvl2_tilemap";
              else if (lvl == 2 && has_g3) gspr = "ground_lvl3_tilemap";
              else if (lvl == 3 && has_g4) gspr = "ground_lvl4_tilemap";
              else if (!has_g1) {
                if (has_g2) gspr = "ground_lvl2_tilemap";
                else if (has_g3) gspr = "ground_lvl3_tilemap";
                else gspr = "ground_lvl4_tilemap";
              }
              platform_->draw_sprite_frame(gspr,
                sx - tile * 0.5f, sy - tile * 0.5f, tile, tile,
                ux * 60, uy * 60, 60, 60, 0.f, 1.f);
            }
          }
        }
        // Forest: the real C2 tilemap_forest instances of the Map layout (150x150 tiles, Solid).
        // Replaces the old procedural canopy that tinted every cell of the map with forest at
        // 55% alpha (not in the original, and it had no collision).
        if (has_forest)
          world_tiles_.draw(*platform_, "tilemap_forest", gcam, sc, cx * 2.f, cy * 2.f);
        // Water / sand fringe (finer 60×60 cells)
        {
          const float step = water_w;
          const float tile = water_w * sc;
          for (int gx = view.lo(gcam.x, cx, sc, step); gx <= view.hi(gcam.x, cx, sc, step); ++gx) {
            for (int gy = view.lo(gcam.y, cy, sc, step); gy <= view.hi(gcam.y, cy, sc, step); ++gy) {
              const float wx = (gx + 0.5f) * step;
              const float wy = (gy + 0.5f) * step;
              const float margin = 400.f;
              const bool deep = wx < margin - 80.f || wy < margin - 80.f ||
                wx > kMapLayoutW - margin + 80.f || wy > kMapLayoutH - margin + 80.f;
              const bool shore = !deep && (wx < margin || wy < margin ||
                wx > kMapLayoutW - margin || wy > kMapLayoutH - margin);
              if (!deep && !shore) continue;
              const float sx = cx + (wx - gcam.x) * sc;
              const float sy = cy + (wy - gcam.y) * sc;
              const int ux = ((gx % 3) + 3) % 3;
              const int uy = ((gy % 5) + 5) % 5;
              if (deep && has_water) {
                platform_->draw_sprite_frame("tilemap_water",
                  sx - tile * 0.5f, sy - tile * 0.5f, tile, tile,
                  ux * 60, uy * 60, 60, 60, 0.f, 1.f);
              } else if (shore && has_sand) {
                platform_->draw_sprite_frame("tilemap_sand",
                  sx - tile * 0.5f, sy - tile * 0.5f, tile, tile,
                  ux * 60, uy * 60, 60, 60, 0.f, 1.f);
              } else if (deep) {
                platform_->draw_rect(sx - tile * 0.5f, sy - tile * 0.5f, tile, tile,
                                     0.12f, 0.22f, 0.35f, 1.f);
              }
            }
          }
        }
        // ground_enviroment_tilemap: 30×30 cells in 150×510 (5×17) — scatter props
        if (platform_->has_sprite("ground_enviroment_tilemap")) {
          const float step = 30.f;
          const float tile = step * sc;
          for (int gx = view.lo(gcam.x, cx, sc, step); gx <= view.hi(gcam.x, cx, sc, step); ++gx) {
            for (int gy = view.lo(gcam.y, cy, sc, step); gy <= view.hi(gcam.y, cy, sc, step); ++gy) {
              // Sparse placement: ~1/7 cells get a prop
              const unsigned h = static_cast<unsigned>(gx * 73856093u) ^
                                 static_cast<unsigned>(gy * 19349663u);
              if ((h % 7u) != 0u) continue;
              const float wx = (gx + 0.5f) * step;
              const float wy = (gy + 0.5f) * step;
              const float margin = 420.f;
              if (wx < margin || wy < margin ||
                  wx > kMapLayoutW - margin || wy > kMapLayoutH - margin) continue;
              const float sx = cx + (wx - gcam.x) * sc;
              const float sy = cy + (wy - gcam.y) * sc;
              const int idx = static_cast<int>(h % 85u); // 5*17
              const int ux = (idx % 5) * 30;
              const int uy = (idx / 5) * 30;
              platform_->draw_sprite_frame("ground_enviroment_tilemap",
                sx - tile * 0.5f, sy - tile * 0.5f, tile, tile,
                ux, uy, 30, 30, 0.f, 1.f);
            }
          }
        }
      }
      platform_->ground_cache_end();

      platform_->profile_mark(IPlatform::Prof_World);
      // Blood drips
      for (int i = 0; i < blood_drip_count_; ++i) {
        if (blood_drips_[i].life <= 0.f) continue;
        if (!view.vis(blood_drips_[i].pos, sc, 4.f)) continue;
        float sx = cx + (blood_drips_[i].pos.x - player_.pos.x) * sc;
        float sy = cy + (blood_drips_[i].pos.y - player_.pos.y) * sc;
        const float a = blood_drips_[i].life / 8.f;
        if (platform_->has_sprite("blood_drop-sheet0"))
          platform_->draw_sprite("blood_drop-sheet0", sx - 3, sy - 3, 6, 6, 0.f, 0.35f + 0.5f * a);
        else
          platform_->draw_rect(sx - 2, sy - 2, 4, 3, 0.55f, 0.05f, 0.08f, 0.25f + 0.45f * a);
      }

      // Corpses (under living actors)
      for (int i = 0; i < corpse_count_; ++i) {
        if (!corpses_[i].active) continue;
        if (!view.vis(corpses_[i].pos, sc, 24.f)) continue;
        float sx = cx + (corpses_[i].pos.x - player_.pos.x) * sc;
        float sy = cy + (corpses_[i].pos.y - player_.pos.y) * sc;
        const float fade = corpses_[i].life < 10.f ? corpses_[i].life / 10.f : 1.f;
        if (corpses_[i].die_anim_t > 0.f && corpses_[i].anim_base[0]) {
          const CharAnim* da = char_anims_.find(corpses_[i].anim_base,
                                                corpses_[i].die_style == 2 ? "die2" : "die1");
          if (da && da->speed > 0) {
            const float total = static_cast<float>(da->frame_count) / da->speed;
            const float t = total - corpses_[i].die_anim_t;
            const CharAnimFrame* fr = char_anims_.sample(da, t < 0.f ? 0.f : t);
            char sheet[64];
            if (fr) {
              std::snprintf(sheet, sizeof(sheet), "%s-sheet%d", corpses_[i].anim_base, fr->sheet);
              if (platform_->has_sprite(sheet)) {
                const float dw = static_cast<float>(fr->sw) * sc, dh = static_cast<float>(fr->sh) * sc;
                platform_->draw_sprite_frame(sheet, sx - dw * 0.5f, sy - dh * 0.5f, dw, dh,
                                             fr->sx, fr->sy, fr->sw, fr->sh, 0.f, 1.f);
                continue;
              }
            }
          }
        }
        const char* spr = 0;
        int sw = 30, sh = 30;
        if (corpses_[i].kind == Corpse_Zed) {
          if (corpses_[i].variant == 1) spr = "zed_fast_skin1_dead-sheet0";
          else if (corpses_[i].variant == 2) spr = "zed_army_dead_skin1-sheet0";
          else if (corpses_[i].variant == 3) spr = "zed_buried_skin_dead-sheet0";
          else spr = "zed_normal_skin1_dead-sheet0";
          sw = 40; sh = 30;
        } else if (corpses_[i].kind == Corpse_Bandit) {
          spr = "bandit_dead-sheet0"; sw = 20; sh = 30;
        } else if (corpses_[i].kind == Corpse_Deer) {
          spr = "deer_dead-sheet0"; sw = 38; sh = 38;
        } else if (corpses_[i].kind == Corpse_Dog) {
          spr = "dog_dead-sheet0"; sw = 30; sh = 30;
        } else if (corpses_[i].kind == Corpse_Wolf) {
          spr = platform_->has_sprite("wolf_dead-sheet0") ? "wolf_dead-sheet0" : "dog_dead-sheet0";
          sw = 30; sh = 30;
        }
        if (spr && platform_->has_sprite(spr)) {
          const float dw = static_cast<float>(sw) * sc * 0.9f;
          const float dh = static_cast<float>(sh) * sc * 0.9f;
          platform_->draw_sprite_frame(spr, sx - dw * 0.5f, sy - dh * 0.5f, dw, dh,
                                       1, 1, sw, sh, corpses_[i].angle, fade);
        } else {
          platform_->draw_rect(sx - 6, sy - 3, 12, 6, 0.35f, 0.12f, 0.1f, 0.55f * fade);
        }
      }

      // Berry bushes (original berry_bush frame)
      if (platform_->has_sprite("berry_bush-sheet0")) {
        const float step = 90.f;
        for (int gx = view.lo(player_.pos.x, cx, sc, step); gx <= view.hi(player_.pos.x, cx, sc, step); ++gx) {
          for (int gy = view.lo(player_.pos.y, cy, sc, step); gy <= view.hi(player_.pos.y, cy, sc, step); ++gy) {
            const unsigned h = static_cast<unsigned>(gx * 374761u) ^
                               static_cast<unsigned>(gy * 6682651u);
            if ((h % 5u) != 0u) continue;
            const float wx = (gx + 0.5f) * step;
            const float wy = (gy + 0.5f) * step;
            if (!view.vis(Vec2(wx, wy), sc, 16.f)) continue;
            float sx = cx + (wx - player_.pos.x) * sc;
            float sy = cy + (wy - player_.pos.y) * sc;
            platform_->draw_sprite_frame("berry_bush-sheet0",
              sx - 14.f, sy - 9.f, 28.f, 18.f, 35, 1, 28, 18, 0.f, 1.f);
          }
        }
      }

      // Forest trees — crop first pine frame (47×92), not whole sheet
      for (int i = 0; i < tree_count_; ++i) {
        if (!view.vis(trees_[i], sc, 72.f)) continue; // pine 47x92 frame drawn 70px tall
        float sx = cx + (trees_[i].x - player_.pos.x) * sc;
        float sy = cy + (trees_[i].y - player_.pos.y) * sc;
        draw_pine(platform_, sx, sy, 70.f);
      }

      // Map POIs — interiors when standing inside, else exterior shells
      for (int i = 0; i < mapgen_.location_count(); ++i) {
        const MapLocation& L = mapgen_.location(i);
        float sx = cx + (L.pos.x - player_.pos.x) * sc;
        float sy = cy + (L.pos.y - player_.pos.y) * sc;
        // Buildings are drawn at their native data.js frame size in world units, scaled by
        // the same world->screen factor as positions/terrain. (Bug fix 2026-10-19: this used
        // to pass a 48..140 "max_dim" as the scale, so one building sheet was blown up
        // ~100x and covered the screen with flat colour squares.)
        const char* ext_spr = poi_building_sprite(L.type, i);
        int nat_w = 100, nat_h = 100;
        poi_native_px(ext_spr, nat_w, nat_h);
        const float max_dim = static_cast<float>(nat_w > nat_h ? nat_w : nat_h) * sc;
        if (!view.vis(L.pos, sc, max_dim * 0.5f + 12.f)) continue; // whole building + nameplate
        const float enter_r = static_cast<float>(nat_w < nat_h ? nat_w : nat_h) * 0.35f;
        const bool inside = (L.type != Loc_Helicrash) &&
                            dist2(L.pos, player_.pos) < enter_r * enter_r;
        if (inside) {
          if (indoor_t_ < 8.f) indoor_t_ = 8.f; // passive shelter while standing in
          const char* ispr = poi_interior_sprite(L.type, i);
          if (platform_->has_sprite(ispr))
            draw_poi_sprite(platform_, sx, sy, ispr, sc);
          else
            draw_poi_sprite(platform_, sx, sy, ext_spr, sc);
        } else {
          draw_poi_sprite(platform_, sx, sy, ext_spr, sc);
        }
        // Nameplate when close
        if (dist2(L.pos, player_.pos) < 160.f * 160.f) {
          const char* lab = location_label(L.type);
          if (lab)
            platform_->draw_text(sx - 18.f, sy - max_dim * 0.55f - 8.f, lab,
                                 0.9f, 0.85f, 0.55f);
        }
        if (L.type == Loc_BunkerEntrance &&
            platform_->has_sprite("alife_marker-sheet0"))
          platform_->draw_sprite("alife_marker-sheet0",
            sx - 6.f, sy - max_dim * 0.55f - 14.f, 12.f, 12.f, 0.f, 1.f);
      }
    }

    // Loot — near piles show first item icon (original ground pickup look)
    {
      const float lsc = in_bunker_ ? 1.f : sc;
      for (int i = 0; i < loot_.count(); ++i) {
        const LootPile* p = loot_.at(i);
        if (!p->active) continue;
        float sx = cx + (p->pos.x - player_.pos.x) * lsc;
        float sy = cy + (p->pos.y - player_.pos.y) * lsc;
        if (p->is_airdrop) {
          if (platform_->has_sprite("ee_pubg_drop_smoke"))
            platform_->draw_sprite("ee_pubg_drop_smoke", sx - 18, sy - 28, 36, 36, 0.f, 0.55f);
          if (platform_->has_sprite("airdrop_box-sheet0"))
            platform_->draw_sprite("airdrop_box-sheet0", sx - 10, sy - 8, 20, 16, 0.f, 1.f);
          else
            platform_->draw_rect(sx - 6, sy - 6, 12, 12, 0.95f, 0.45f, 0.1f, 1.f);
          continue;
        }
        if (!view.vis(p->pos, sc, 24.f)) continue;
        // C2 layer items_on_ground: each pile shows its item sprites at native size.
        // spawn_loot_point is an invisible editor marker in C2 (16x16 colour swatch) and is
        // never drawn in game (it was the "colored stacks" placeholder).
        int drawn = 0;
        for (int j = 0; j < p->item_count && drawn < 3; ++j) {
          if (p->counts[j] <= 0) continue;
          const ItemIcon* ic = find_item_ground_icon(p->items[j]);
          if (!ic || !platform_->has_sprite(ic->sprite)) continue;
          // C2 on_ground frame at native size (same 1:1 scale as characters: 30px sheet -> 30px).
          const float iw = static_cast<float>(ic->sw);
          const float ih = static_cast<float>(ic->sh);
          const float ox = (drawn == 1) ? 5.f : (drawn == 2 ? -5.f : 0.f);
          const float oy = (drawn == 0) ? 0.f : 3.f;
          platform_->draw_sprite_frame(ic->sprite, sx + ox - iw * 0.5f, sy + oy - ih * 0.5f, iw, ih,
                                       ic->sx, ic->sy, ic->sw, ic->sh, 0.f, 1.f);
          ++drawn;
        }
        if (drawn == 0 && p->item_count > 0)
          platform_->draw_rect(sx - 3, sy - 2, 6, 4, 0.55f, 0.5f, 0.35f, 0.9f);
      }
    }

    // Zombies — C2 idle/walk/run/attack sheets
    {
      const float zsc = in_bunker_ ? 1.f : sc;
      for (const Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
        if (!z->alive) continue;
        if (!view.vis(z->pos, zsc, 32.f)) continue;
        float sx = cx + (z->pos.x - player_.pos.x) * zsc;
        float sy = cy + (z->pos.y - player_.pos.y) * zsc;
        float zsz = 30.f;
        if (z->type == Zed_Buried && z->ambush_boost <= 0.f) {
          if (platform_->has_sprite("zed_buried_hidden-sheet0"))
            platform_->draw_sprite_frame("zed_buried_hidden-sheet0",
                                        sx - 15, sy - 10, 30, 20, 1, 1, 30, 30, 0.f, 1.f);
          else
            platform_->draw_rect(sx - 7, sy - 3, 14, 8, 0.32f, 0.28f, 0.18f, 0.9f);
          continue;
        }
        const char* base = zed_skin_base(z->type);
        if (z->type == Zed_Tank) zsz = 42.f;
        const float fdeg = std::atan2(player_.pos.y - z->pos.y, player_.pos.x - z->pos.x) * 57.2957795f;
        const float d2z = dist2(z->pos, player_.pos);
        const char* zprefix = "idle";
        float zt = state_timer_ + static_cast<float>(z->id) * 0.17f;
        // attack_cooldown high right after a swing in C2 AI
        if (z->attack_cooldown > 0.45f) zprefix = "attack";
        else if (d2z < z->aggro_radius * z->aggro_radius) {
          zprefix = (z->type == Zed_Fast || z->speed > 55.f) ? "run" : "walk";
        }
        char probe[64];
        sprite_sheet_name(probe, (int)sizeof(probe), base, 0);
        platform_->draw_rect(sx - zsz * 0.35f, sy + zsz * 0.28f, zsz * 0.7f, 4.f,
                             0.f, 0.f, 0.f, 0.28f);
        if (platform_->has_sprite(probe))
          draw_c2_char(platform_, &char_anims_, sx, sy, zsz, base, fdeg, zt, zprefix);
        else
          platform_->draw_rect(sx - 5, sy - 5, 10, 10, 0.7f, 0.2f, 0.15f, 1.f);
        if (!in_bunker_ && weather_.is_night && !player_.torch_on) {
          const char* eyes = "zed_normal_eyes";
          if (z->type == Zed_Fast) eyes = "zed_fast_eyes";
          else if (z->type == Zed_Army || z->type == Zed_ArmyShooter) eyes = "zed_army_eyes";
          else if (z->type == Zed_Buried) eyes = "zed_buried_eyes";
          else if (z->type == Zed_Tank) eyes = "zed_tank_eyes";
          else if (z->type == Zed_Screamer) eyes = "zed_screamer_eyes";
          char eprobe[64];
          sprite_sheet_name(eprobe, (int)sizeof(eprobe), eyes, 0);
          if (platform_->has_sprite(eprobe))
            draw_c2_char(platform_, &char_anims_, sx, sy, zsz, eyes, fdeg, 0.f, "idle");
        }
        if (z->hp < z->max_hp * 0.99f)
          platform_->draw_rect(sx - 5, sy - zsz * 0.5f - 3.f, 10.f * (z->hp / z->max_hp), 2, 0.9f, 0.2f, 0.15f, 1.f);
      }
    }

    if (!in_bunker_) {
      // Airdrop marker while alert active
      if (airdrop_alert_ > 0.f) {
        float sx = cx + (last_airdrop_pos_.x - player_.pos.x) * sc;
        float sy = cy + (last_airdrop_pos_.y - player_.pos.y) * sc;
        if (platform_->has_sprite("airdrop_parachute-sheet0"))
          platform_->draw_sprite("airdrop_parachute-sheet0", sx - 16, sy - 28, 32, 32, 0.f, 0.9f);
        else if (platform_->has_sprite("gui_airdrop_icon-sheet0"))
          platform_->draw_sprite("gui_airdrop_icon-sheet0", sx - 10, sy - 10, 20, 20, 0.f, 0.9f);
        else
          platform_->draw_rect(sx - 8, sy - 8, 16, 16, 1.f, 0.8f, 0.1f, 0.35f);
      }

      // Screamer — subtle pulse only (no debug text)
      if (zombies_.scream_active()) {
        const Vec2 sp = zombies_.scream_pos();
        float sx = cx + (sp.x - player_.pos.x) * sc;
        float sy = cy + (sp.y - player_.pos.y) * sc;
        const float pulse = 18.f + 8.f * std::fabs(std::sin(state_timer_ * 6.f));
        platform_->draw_rect(sx - pulse, sy - pulse, pulse * 2.f, pulse * 2.f,
                             0.7f, 0.2f, 0.65f, 0.08f);
      }

      // Animals — C2 run/idle/attack
      {
        for (int i = 0; i < animals_.count(); ++i) {
          const Animal* a = animals_.at(i);
          if (!a->alive) continue;
          if (!view.vis(a->pos, sc, 32.f)) continue;
          float sx = cx + (a->pos.x - player_.pos.x) * sc;
          float sy = cy + (a->pos.y - player_.pos.y) * sc;
          const float fdeg = std::atan2(player_.pos.y - a->pos.y, player_.pos.x - a->pos.x) * 57.2957795f;
          const float d2a = dist2(a->pos, player_.pos);
          const char* aprefix = (d2a < 140.f * 140.f) ? "run" : "idle";
          const float at = state_timer_ + static_cast<float>(i) * 0.23f;
          platform_->draw_rect(sx - 10.f, sy + 10.f, 20.f, 3.f, 0.f, 0.f, 0.f, 0.25f);
          if (a->type == Animal_Wolf || a->type == Animal_Dog) {
            draw_c2_char(platform_, &char_anims_, sx, sy, 30.f, "wolf_skin", fdeg, at, aprefix);
            if (a->type == Animal_Dog && platform_->has_sprite("dog_marker-sheet0"))
              platform_->draw_sprite("dog_marker-sheet0", sx - 5.f, sy - 22.f, 10.f, 8.f, 0.f, 0.85f);
            if (!in_bunker_ && weather_.is_night) {
              const char* ae = (a->type == Animal_Dog) ? "dog_eyes" : "wolf_eyes";
              char ap[64];
              sprite_sheet_name(ap, (int)sizeof(ap), ae, 0);
              if (platform_->has_sprite(ap))
                draw_c2_char(platform_, &char_anims_, sx, sy, 30.f, ae, fdeg, 0.f, "idle");
            }
          } else if (a->type == Animal_Deer) {
            draw_c2_char(platform_, &char_anims_, sx, sy, 38.f, "deer_skin", fdeg, at, aprefix, 38, 40);
            if (!in_bunker_ && weather_.is_night && platform_->has_sprite("deer_eyes-sheet0"))
              draw_c2_char(platform_, &char_anims_, sx, sy, 38.f, "deer_eyes", fdeg, 0.f, "idle", 38, 40);
          } else if (a->type == Animal_Rabbit) {
            draw_c2_char(platform_, &char_anims_, sx, sy, 30.f, "rabbit_skin", fdeg, at, aprefix);
            if (!in_bunker_ && weather_.is_night && platform_->has_sprite("rabbit_eyes-sheet0"))
              draw_c2_char(platform_, &char_anims_, sx, sy, 30.f, "rabbit_eyes", fdeg, 0.f, "idle");
          } else {
            platform_->draw_rect(sx - 3, sy - 3, 6, 6, 0.2f, 0.2f, 0.22f, 1.f);
          }
        }
      }

      // Bandits — C2 run/idle
      {
        for (const Bandit* bd = bandits_.begin(); bd != bandits_.end(); ++bd) {
          if (!bd->alive) continue;
          if (!view.vis(bd->pos, sc, 32.f)) continue;
          float sx = cx + (bd->pos.x - player_.pos.x) * sc;
          float sy = cy + (bd->pos.y - player_.pos.y) * sc;
          const float fdeg = std::atan2(player_.pos.y - bd->pos.y, player_.pos.x - bd->pos.x) * 57.2957795f;
          const float d2b = dist2(bd->pos, player_.pos);
          const char* bprefix = (d2b < 220.f * 220.f) ? "run" : "idle";
          const float bt = state_timer_ + static_cast<float>(bd->id) * 0.13f;
          platform_->draw_rect(sx - 10.f, sy + 10.f, 20.f, 3.f, 0.f, 0.f, 0.f, 0.25f);
          if (platform_->has_sprite("bandit_skin-sheet0"))
            draw_c2_char(platform_, &char_anims_, sx, sy, 30.f, "bandit_skin", fdeg, bt, bprefix);
          else
            platform_->draw_rect(sx - 5, sy - 5, 10, 10, 0.25f, 0.4f, 0.75f, 1.f);
          if (platform_->has_sprite("bot_bandit_melee_marker-sheet0"))
            platform_->draw_sprite("bot_bandit_melee_marker-sheet0",
              sx - 5.f, sy - 28.f, 10.f, 8.f, 0.f, 0.9f);
          if (bd->hp < bd->max_hp * 0.99f)
            platform_->draw_rect(sx - 5, sy - 24, 10.f * (bd->hp / bd->max_hp), 2, 0.9f, 0.2f, 0.15f, 1.f);
        }
      }

      // Vehicles (UAZ / boat sprites)
      for (int i = 0; i < vehicles_.count(); ++i) {
        const Vehicle* v = vehicles_.at(i);
        if (!v->active) continue;
        if (!view.vis(v->pos, sc, 48.f)) continue;
        float sx = cx + (v->pos.x - player_.pos.x) * sc;
        float sy = cy + (v->pos.y - player_.pos.y) * sc;
        if (v->type == Vehicle_Boat) {
          if (platform_->has_sprite("boat_arrive-sheet0"))
            platform_->draw_sprite("boat_arrive-sheet0", sx - 36, sy - 22, 72, 44, v->facing_deg, 1.f);
          else
            platform_->draw_rect(sx - 10, sy - 4, 20, 8, 0.25f, 0.45f, 0.55f, 1.f);
        } else if (v->type == Vehicle_Bike) {
          if (platform_->has_sprite("car_hatch_red-sheet0"))
            platform_->draw_sprite("car_hatch_red-sheet0", sx - 22, sy - 12, 44, 24, v->facing_deg, 1.f);
          else
            platform_->draw_rect(sx - 6, sy - 3, 12, 6, 0.55f, 0.35f, 0.2f, 1.f);
        } else {
          if (platform_->has_sprite("car_uaz_base-sheet0"))
            platform_->draw_sprite("car_uaz_base-sheet0", sx - 36, sy - 16, 72, 32, v->facing_deg, 1.f);
          else
            platform_->draw_rect(sx - 8, sy - 5, 16, 10, 0.4f, 0.4f, 0.45f, 1.f);
        }
      }

      // Campfires (flicker between fireplace sheets)
      for (int i = 0; i < campfires_.count(); ++i) {
        const Campfire* f = campfires_.at(i);
        if (!f->active) continue;
        if (!view.vis(f->pos, sc, 40.f)) continue; // flame + glow
        float sx = cx + (f->pos.x - player_.pos.x) * sc;
        float sy = cy + (f->pos.y - player_.pos.y) * sc;
        const int flick = (static_cast<int>(state_timer_ * 8.f) + i) & 1;
        const char* fire =
          (flick && platform_->has_sprite("fireplace-sheet1")) ? "fireplace-sheet1" :
          platform_->has_sprite("fireplace-sheet0") ? "fireplace-sheet0" :
          platform_->has_sprite("fire_place_warm-sheet0") ? "fire_place_warm-sheet0" : 0;
        if (fire)
          platform_->draw_sprite(fire, sx - 20, sy - 20, 40, 40, 0.f, 1.f);
        else {
          platform_->draw_rect(sx - 6, sy - 6, 12, 12, 0.9f, 0.35f, 0.05f, 0.9f);
          platform_->draw_rect(sx - 3, sy - 10, 6, 6, 1.f, 0.7f, 0.15f, 0.7f);
        }
      }

      // Placed traps / barbed wire — original sprites
      for (int i = 0; i < trap_count_; ++i) {
        if (!traps_[i].active) continue;
        if (!view.vis(traps_[i].pos, sc, 16.f)) continue;
        float sx = cx + (traps_[i].pos.x - player_.pos.x) * sc;
        float sy = cy + (traps_[i].pos.y - player_.pos.y) * sc;
        if (traps_[i].kind == 0) {
          const char* ts = traps_[i].sprung ? "beartrap_deployed-sheet0" : "beartrap-sheet0";
          if (platform_->has_sprite(ts))
            platform_->draw_sprite(ts, sx - 8, sy - 5, 16, 10, 0.f, 1.f);
          else
            platform_->draw_rect(sx - 5, sy - 5, 10, 10, 0.45f, 0.45f, 0.25f, 0.95f);
        } else if (traps_[i].kind == 2) {
          platform_->draw_rect(sx - 3, sy - 10, 6, 20, 0.45f, 0.32f, 0.18f, 0.95f);
          platform_->draw_rect(sx - 10, sy - 4, 20, 3, 0.5f, 0.36f, 0.2f, 0.9f);
        } else if (traps_[i].kind == 3) {
          platform_->draw_rect(sx - 4, sy - 3, 8, 6, 0.35f, 0.35f, 0.2f, 0.95f);
          platform_->draw_rect(sx - 2, sy - 5, 4, 3, 0.7f, 0.2f, 0.15f, 0.9f);
        } else if (traps_[i].kind == 4) {
          if (platform_->has_sprite("landmine-sheet0"))
            platform_->draw_sprite("landmine-sheet0", sx - 6, sy - 6, 12, 12, 0.f, 1.f);
          else {
            platform_->draw_rect(sx - 6, sy - 6, 12, 12, 0.25f, 0.28f, 0.2f, 0.95f);
            platform_->draw_rect(sx - 2, sy - 2, 4, 4, 0.85f, 0.25f, 0.1f, 0.9f);
          }
        } else {
          if (platform_->has_sprite("barbedwire-sheet0"))
            platform_->draw_sprite("barbedwire-sheet0", sx - 14, sy - 10, 28, 20, 0.f, 0.95f);
          else if (platform_->has_sprite("barberwire_deployed-sheet0"))
            platform_->draw_sprite("barberwire_deployed-sheet0", sx - 14, sy - 10, 28, 20, 0.f, 0.95f);
          else {
            platform_->draw_rect(sx - 14, sy - 14, 28, 28, 0.35f, 0.3f, 0.28f, 0.35f);
            platform_->draw_rect(sx - 8, sy - 1, 16, 2, 0.6f, 0.55f, 0.5f, 0.8f);
          }
        }
      }
      // Crop plots — dirt mound while growing, green when ready
      for (int i = 0; i < crop_count_; ++i) {
        if (!crops_[i].active) continue;
        if (!view.vis(crops_[i].pos, sc, 16.f)) continue;
        float sx = cx + (crops_[i].pos.x - player_.pos.x) * sc;
        float sy = cy + (crops_[i].pos.y - player_.pos.y) * sc;
        if (crops_[i].ready) {
          platform_->draw_rect(sx - 5, sy - 5, 10, 10, 0.25f, 0.65f, 0.2f, 0.95f);
          if (dist2(crops_[i].pos, player_.pos) < 40.f * 40.f)
            platform_->draw_text(sx - 14.f, sy - 14.f, "E HARVEST", 0.55f, 0.9f, 0.45f);
        } else {
          platform_->draw_rect(sx - 4, sy - 3, 8, 6, 0.35f, 0.28f, 0.18f, 0.9f);
          platform_->draw_rect(sx - 2, sy - 6, 4, 4, 0.3f, 0.5f, 0.22f, 0.85f);
        }
      }
    }

    // Bullets — grenade / molotov fly sprites, else zed_bullet strip
    {
      const float bsc = in_bunker_ ? 1.f : sc;
      const bool has_b = platform_->has_sprite("zed_bullet-sheet0");
      for (const Bullet* b = bullets_.begin(); b != bullets_.end(); ++b) {
        if (!b->alive) continue;
        if (!view.vis(b->pos, bsc, 8.f)) continue;
        float sx = cx + (b->pos.x - player_.pos.x) * bsc;
        float sy = cy + (b->pos.y - player_.pos.y) * bsc;
        const float ang = std::atan2(b->vel.y, b->vel.x) * 57.2957795f;
        if (b->leaves_fire && platform_->has_sprite("molotov_fly-sheet0")) {
          platform_->draw_sprite("molotov_fly-sheet0", sx - 6, sy - 6, 12, 12, ang, 1.f);
        } else if (b->explosive && platform_->has_sprite("f1_grenade_active-sheet0")) {
          platform_->draw_sprite("f1_grenade_active-sheet0", sx - 6, sy - 6, 12, 12, ang, 1.f);
        } else if (b->is_arrow && platform_->has_sprite("arrow_bullet-sheet0")) {
          platform_->draw_sprite("arrow_bullet-sheet0", sx - 10, sy - 2, 20, 4, ang, 1.f);
        } else if (has_b) {
          platform_->draw_sprite("zed_bullet-sheet0", sx - 8, sy - 1.5f, 16, 3, ang, 1.f);
        } else {
          platform_->draw_rect(sx - 1, sy - 1, 3, 3, 1.f, 0.95f, 0.4f, 1.f);
        }
      }
    }

    // Muzzle / hit / explode FX
    {
      const float bsc = in_bunker_ ? 1.f : sc;
      for (int i = 0; i < fx_count_; ++i) {
        if (!fx_[i].active) continue;
        if (!view.vis(fx_[i].pos, bsc, 64.f)) continue; // explosions are the largest fx
        float sx = cx + (fx_[i].pos.x - player_.pos.x) * bsc;
        float sy = cy + (fx_[i].pos.y - player_.pos.y) * bsc;
        const float a = fx_[i].life > 0.05f ? 1.f : fx_[i].life / 0.05f;
        if (fx_[i].kind == Fx_Muzzle) {
          const int frame = static_cast<int>(fx_[i].life * 40.f) % 3;
          const char* mz = (frame == 1 && platform_->has_sprite("muzzle_sprite-sheet1"))
                            ? "muzzle_sprite-sheet1"
                          : (frame == 2 && platform_->has_sprite("muzzle_sprite-sheet2"))
                            ? "muzzle_sprite-sheet2"
                          : platform_->has_sprite("muzzle_sprite-sheet0")
                            ? "muzzle_sprite-sheet0"
                          : platform_->has_sprite("muzzle_flash-sheet0")
                            ? "muzzle_flash-sheet0" : 0;
          if (mz)
            platform_->draw_sprite(mz, sx - 14, sy - 14, 28, 28, fx_[i].angle, a);
          else
            platform_->draw_rect(sx - 4, sy - 4, 8, 8, 1.f, 0.9f, 0.4f, a);
        } else if (fx_[i].kind == Fx_Hit) {
          if (platform_->has_sprite("test_bullet_hit-sheet0"))
            platform_->draw_sprite_frame("test_bullet_hit-sheet0",
              sx - 10, sy - 10, 20, 20, 1, 1, 20, 20, 0.f, a);
          else if (platform_->has_sprite("test_bodyhit-sheet0"))
            platform_->draw_sprite_frame("test_bodyhit-sheet0",
              sx - 10, sy - 10, 20, 20, 1, 1, 20, 20, 0.f, a);
          else
            platform_->draw_rect(sx - 3, sy - 3, 6, 6, 0.9f, 0.2f, 0.15f, a);
        } else if (fx_[i].kind == Fx_Shell) {
          const char* sh = platform_->has_sprite("test_shell-sheet0")
                             ? "test_shell-sheet0" : 0;
          if (sh)
            platform_->draw_sprite(sh, sx - 3, sy - 2, 6, 4, fx_[i].angle, a);
          else
            platform_->draw_rect(sx - 2, sy - 1, 4, 2, 0.75f, 0.65f, 0.25f, a);
        } else if (fx_[i].kind == Fx_Blood) {
          if (platform_->has_sprite("blood_drop-sheet0"))
            platform_->draw_sprite("blood_drop-sheet0", sx - 6, sy - 6, 12, 12, 0.f, a * 0.85f);
          else
            platform_->draw_rect(sx - 4, sy - 3, 8, 6, 0.55f, 0.05f, 0.08f, a * 0.7f);
        } else if (fx_[i].kind == Fx_Fish) {
          const int fr = (static_cast<int>(fx_[i].life * 8.f) % 3);
          const char* fs = (fr == 1 && platform_->has_sprite("fish_sprite-sheet1"))
                             ? "fish_sprite-sheet1"
                           : (fr == 2 && platform_->has_sprite("fish_sprite-sheet2"))
                             ? "fish_sprite-sheet2"
                           : platform_->has_sprite("fish_sprite-sheet0")
                             ? "fish_sprite-sheet0" : 0;
          if (fs)
            platform_->draw_sprite(fs, sx - 10, sy - 6, 20, 12, 0.f, a);
          else
            platform_->draw_rect(sx - 6, sy - 3, 12, 6, 0.4f, 0.55f, 0.7f, a);
        } else if (fx_[i].kind == Fx_Explode) {
          const bool molotov = fx_[i].angle > 0.5f;
          if (molotov && platform_->has_sprite("molotov_flame"))
            platform_->draw_sprite("molotov_flame", sx - 24, sy - 24, 48, 48, 0.f, a);
          else if (platform_->has_sprite("grenadeparticles"))
            platform_->draw_sprite("grenadeparticles", sx - 30, sy - 30, 60, 60, 0.f, a);
          else if (platform_->has_sprite("car_explode-sheet0"))
            platform_->draw_sprite("car_explode-sheet0", sx - 28, sy - 28, 56, 56, 0.f, a);
          else
            platform_->draw_rect(sx - 20, sy - 20, 40, 40, 1.f, 0.45f, 0.1f, a * 0.7f);
        }
      }
    }

    // Cloud shadows drifting over the map (daytime)
    if (!in_bunker_ && !weather_.is_night && platform_->has_sprite("cloud_shadow-sheet0")) {
      const float drift = state_timer_ * 12.f;
      for (int i = 0; i < 5; ++i) {
        const float bx = std::fmod(drift * (0.6f + 0.15f * i) + i * 97.f, 560.f) - 40.f;
        const float by = 20.f + (i * 47) % 200;
        platform_->draw_sprite("cloud_shadow-sheet0", bx, by, 140.f, 70.f, 0.f, 0.18f);
      }
    }

    // Light snow dust when cold / night fringe
    if (!in_bunker_ && weather_.ambient_temp < 5.f &&
        platform_->has_sprite("snow_ground-sheet0")) {
      for (int i = 0; i < 8; ++i) {
        const float gx = std::fmod(state_timer_ * 8.f + i * 61.f, 500.f) - 10.f;
        const float gy = std::fmod(state_timer_ * 3.f + i * 37.f, 290.f);
        platform_->draw_sprite("snow_ground-sheet0", gx, gy, 18, 6, 0.f, 0.2f);
      }
    }

    // Dawn/dusk fog (day_phase near 0 or 1)
    if (!in_bunker_ && !weather_.is_night) {
      const float phase = weather_.day_phase;
      float fog = 0.f;
      if (phase < 0.12f) fog = (0.12f - phase) / 0.12f;
      else if (phase > 0.88f) fog = (phase - 0.88f) / 0.12f;
      if (fog > 0.02f) {
        platform_->draw_rect(0, 0, 480, 272, 0.55f, 0.58f, 0.62f, 0.12f + 0.22f * fog);
      }
    }

    // Night darkness vignette + torch/NVG/headlights + stars
    const bool headlights = vehicles_.player_driving() && weather_.is_night &&
                            vehicles_.driven() &&
                            (vehicles_.driven()->type == Vehicle_Car ||
                             vehicles_.driven()->type == Vehicle_Bike);
    if (!in_bunker_ && weather_.is_night && !player_.torch_on && !headlights) {
      if (player_.nvg_on) {
        if (platform_->has_sprite("light_nvg_color_tiled"))
          platform_->draw_sprite("light_nvg_color_tiled", 0, 0, 480, 272, 0.f, 0.35f);
        else {
          platform_->draw_rect(0, 0, 480, 272, 0.02f, 0.18f, 0.05f, 0.18f);
          platform_->draw_rect(cx - 90, cy - 70, 180, 140, 0.08f, 0.35f, 0.12f, 0.12f);
        }
      } else if (platform_->has_sprite("night_overlay-sheet0")) {
        platform_->draw_sprite("night_overlay-sheet0", 0, 0, 480, 272, 0.f, 0.72f);
        for (int si = 0; si < 18; ++si) {
          const float sx = static_cast<float>((si * 97 + 13) % 480);
          const float sy = static_cast<float>((si * 53 + 7) % 120);
          platform_->draw_rect(sx, sy, 1, 1, 0.85f, 0.9f, 1.f, 0.45f);
        }
      } else {
        platform_->draw_rect(0, 0, 480, 272, 0.f, 0.f, 0.08f, 0.48f);
        for (int si = 0; si < 24; ++si) {
          const float sx = static_cast<float>((si * 97 + 13) % 480);
          const float sy = static_cast<float>((si * 53 + 7) % 120);
          platform_->draw_rect(sx, sy, 1, 1, 0.85f, 0.9f, 1.f, 0.55f);
        }
      }
    }
    if (!in_bunker_ && weather_.rain > 0.55f) {
      if (platform_->has_sprite("rain_fade-sheet0"))
        platform_->draw_sprite("rain_fade-sheet0", 0, 0, 480, 272, 0.f,
                               0.12f + 0.2f * (weather_.rain > 1.f ? 1.f : weather_.rain));
      // Falling rain — crop small streaks from rain_particle atlas
      const bool has_rp = platform_->has_sprite("rain_particle-sheet0");
      for (int i = 0; i < 28; ++i) {
        const float rx = static_cast<float>((i * 37 + static_cast<int>(state_timer_ * 180.f)) % 480);
        const float ry = static_cast<float>((i * 53 + static_cast<int>(state_timer_ * 220.f)) % 272);
        if (has_rp)
          platform_->draw_sprite_frame("rain_particle-sheet0", rx, ry, 3, 10,
                                       2 + (i % 4) * 8, 2 + (i % 3) * 12, 4, 12, 0.f, 0.45f);
        else
          platform_->draw_rect(rx, ry, 1, 6, 0.55f, 0.6f, 0.7f, 0.35f);
      }
      if (platform_->has_sprite("rain_ground-sheet0")) {
        for (int i = 0; i < 6; ++i) {
          const float gx = static_cast<float>((i * 79 + 20) % 460);
          const float gy = 200.f + static_cast<float>((i * 17) % 50);
          platform_->draw_sprite("rain_ground-sheet0", gx, gy, 24, 8, 0.f, 0.25f);
        }
      }
    }
    if (player_.torch_on || headlights) {
      float face = player_.facing_deg;
      if (headlights && vehicles_.driven()) face = vehicles_.driven()->facing_deg;
      if (headlights && platform_->has_sprite("car_light-sheet0")) {
        const float rad = face * 0.01745329252f;
        const float bx = cx + std::cos(rad) * 36.f;
        const float by = cy + std::sin(rad) * 36.f;
        platform_->draw_sprite("car_light-sheet0", bx - 40, by - 24, 80, 48, face, 0.55f);
      } else if (player_.torch_on && platform_->has_sprite("light_sprite-sheet0")) {
        platform_->draw_sprite("light_sprite-sheet0", cx - 48, cy - 48, 96, 96, face, 0.5f);
      } else {
        const float glow = headlights ? 40.f : 28.f;
        platform_->draw_rect(cx - glow, cy - glow, glow * 2.f, glow * 2.f,
                             0.55f, 0.4f, 0.1f, headlights ? 0.2f : 0.25f);
        const float rad = face * 0.01745329252f;
        const int steps = headlights ? 10 : 8;
        for (int s = 2; s <= steps; ++s) {
          const float d = (headlights ? 14.f : 10.f) * static_cast<float>(s);
          const float w = 4.f + static_cast<float>(s) * (headlights ? 3.f : 2.2f);
          const float bx = cx + std::cos(rad) * d;
          const float by = cy + std::sin(rad) * d;
          platform_->draw_rect(bx - w * 0.5f, by - w * 0.5f, w, w,
                               0.9f, 0.8f, 0.4f, 0.1f + 0.02f * s);
        }
      }
    }
    if (lightning_t_ > 0.f) {
      platform_->draw_rect(0, 0, 480, 272, 0.95f, 0.95f, 1.f, 0.35f * (lightning_t_ / 0.12f));
    }
    // Critical blood / infection screen tint
    if (player_.survival.blood < 30.f) {
      const float a = 0.15f + (30.f - player_.survival.blood) * 0.01f;
      platform_->draw_rect(0, 0, 480, 272, 0.55f, 0.05f, 0.05f, a > 0.45f ? 0.45f : a);
    } else if (player_.survival.infected) {
      platform_->draw_rect(0, 0, 480, 272, 0.15f, 0.35f, 0.1f, 0.08f);
    }

    // Player — C2 idle/run/axe/pistol/twohand/using_item
    {
      const float spd2 = player_.vel.x * player_.vel.x + player_.vel.y * player_.vel.y;
      const bool moving = spd2 > 4.f;
      const float psz = crouching_ ? 26.f : 30.f;
      const InvSlot& eq = inventory_.slot(equipped_slot_);
      const WeaponDef* w = eq.empty() ? 0 : find_weapon(eq.id);
      const HoldPose hold = hold_pose_for(w);
      platform_->draw_rect(cx - 11.f, cy + 12.f, 22.f, 4.f, 0.f, 0.f, 0.f, 0.3f);
      const char* body = player_body_skin(inventory_);
      char body_probe[64];
      sprite_sheet_name(body_probe, (int)sizeof(body_probe), body, 0);
      if (platform_->has_sprite(body_probe) ||
          platform_->has_sprite("player_skin_def_1-sheet0"))
        draw_player_c2(platform_, &char_anims_, cx, cy, psz, player_.facing_deg,
                       player_anim_t_, melee_anim_t_, use_anim_t_,
                       moving, player_.running, hold, body);
      else
        platform_->draw_rect(cx - 6, cy - 6, 12, 12, 0.25f, 0.75f, 0.35f, 1.f);
      // Gasmask / helmet badge overlay (inventory icon cropped small on head)
      if (wearing_gasmask() && platform_->has_sprite("gasmask-sheet0"))
        platform_->draw_sprite_frame("gasmask-sheet0",
          cx - 6.f, cy - psz * 0.45f, 12.f, 12.f, 1, 1, 30, 30, 0.f, 0.95f);

      // Original crosshair when holding a firearm (best scope in bag)
      if (w && !w->stats.is_melee && !vehicles_.player_driving()) {
        const char* ch = "crosshair_rds-sheet0";
        int cw = 8, chh = 7;
        int best = 0; // 0 none, 1 rds, 2 pu, 3 pso, 4 acog, 5 long
        for (int i = 0; i < inventory_.slot_count(); ++i) {
          const ItemId sid = inventory_.slot(i).id;
          int rank = 0;
          if (sid == 76) rank = 1;
          else if (sid == 77) rank = 2;
          else if (sid == 78) rank = 3;
          else if (sid == 79) rank = 4;
          else if (sid == 98) rank = 5;
          if (rank > best) best = rank;
        }
        if (best == 5) { ch = "crosshair_long-sheet0"; cw = 11; chh = 5; }
        else if (best == 4) { ch = "crosshair_acog-sheet0"; cw = 10; chh = 7; }
        else if (best == 3) { ch = "crosshair_pso-sheet0"; cw = 11; chh = 8; }
        else if (best == 2) { ch = "crosshair_pu-sheet0"; cw = 11; chh = 6; }
        else if (best == 1) { ch = "crosshair_rds-sheet0"; cw = 8; chh = 7; }
        const float rad = player_.facing_deg * 0.01745329252f;
        const float reach = crouching_ ? 48.f : 40.f;
        const float hx = cx + std::cos(rad) * reach;
        const float hy = cy + std::sin(rad) * reach;
        if (platform_->has_sprite(ch))
          platform_->draw_sprite(ch, hx - cw * 0.5f, hy - chh * 0.5f,
                                 static_cast<float>(cw), static_cast<float>(chh), 0.f, 0.95f);
        else
          platform_->draw_rect(hx - 1, hy - 1, 2, 2, 0.95f, 0.9f, 0.5f, 0.9f);
      }
    }

    // C2 dmg_hint: "-N" Text on the Rain layer, orange (255,196,68) or red when crit, fading.
    for (const DmgHint* h = dmg_hints_.begin(); h != dmg_hints_.end(); ++h) {
      if (!h->active) continue;
      char t[16];
      std::snprintf(t, sizeof(t), "-%d", h->amount);
      const float hx = cx + (h->pos.x - player_.pos.x) * sc;
      const float hy = cy + (h->pos.y - player_.pos.y) * sc - 14.f;
      if (h->crit) platform_->draw_text(hx - 6.f, hy, t, 1.f, 0.f, 0.f);
      else platform_->draw_text(hx - 6.f, hy, t, 1.f, 196.f / 255.f, 68.f / 255.f);
    }
    platform_->world_pass_end();
    platform_->profile_mark(IPlatform::Prof_Hud);
    draw_debug_hud(*platform_, *this);
    if (state_ == AppState_Paused) render_pause_menu();
  } else if (state_ == AppState_Dead) {
    render_dead_screen();
  }

  platform_->end_frame();
}

} // namespace mdaz

namespace mdaz {
int Game::bandits_alive_near(Vec2 pos, float radius) const {
  int n = 0;
  for (const Bandit* b = bandits_.begin(); b != bandits_.end(); ++b)
    if (b->alive && dist2(b->pos, pos) < radius * radius) ++n;
  return n;
}
} // namespace mdaz

