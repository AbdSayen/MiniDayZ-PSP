#include "mdaz/game.hpp"
#include "mdaz/combat.hpp"
#include "mdaz/craft.hpp"
#include "mdaz/hud.hpp"
#include "mdaz/map_labels.hpp"
#include "mdaz/save_game.hpp"
#include "mdaz/weapons.hpp"

#include <cmath>
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

// C2 packs many 30×30 frames into 128×128 sheets at (1+col*32, 1+row*32).
// data.js: idle_down/up/right/left = cols 0..3 on row 0; walk rows 1..3.
int facing_to_col(float deg) {
  while (deg > 180.f) deg -= 360.f;
  while (deg < -180.f) deg += 360.f;
  if (deg >= -45.f && deg < 45.f) return 2;   // right
  if (deg >= 45.f && deg < 135.f) return 0;   // down
  if (deg >= -135.f && deg < -45.f) return 1; // up
  return 3; // left
}

void sprite_sheet_name(char* out, int out_n, const char* base, int sheet) {
  if (sheet < 0) sheet = 0;
  if (sheet > 2) sheet = sheet % 3;
  std::snprintf(out, out_n, "%s-sheet%d", base, sheet);
}

// Draw one C2 character/animal frame (default 30×30 cell). No rotation —
// direction is already encoded in the frame column (as in the original).
void draw_c2_char(IPlatform* p, float sx, float sy, float size,
                  const char* base, float facing_deg, int walk_frame,
                  int cell = 30, int stride = 32) {
  if (!p || !base) return;
  char name[64];
  // Prefer sheet0; walk uses rows. sheet1/2 are weapon poses etc.
  sprite_sheet_name(name, (int)sizeof(name), base, 0);
  if (!p->has_sprite(name)) return;
  const int col = facing_to_col(facing_deg);
  int row = 0;
  if (walk_frame > 0) row = 1 + ((walk_frame - 1) % 3);
  const int srcx = 1 + col * stride;
  const int srcy = 1 + row * stride;
  p->draw_sprite_frame(name, sx - size * 0.5f, sy - size * 0.5f, size, size,
                       srcx, srcy, cell, cell, 0.f, 1.f);
}

void draw_world_sprite(IPlatform* p, float cx, float cy, float sc, Vec2 cam, Vec2 world,
                       const char* name, float size, float angle_deg = 0.f, float alpha = 1.f) {
  if (!p || !name) return;
  const float sx = cx + (world.x - cam.x) * sc;
  const float sy = cy + (world.y - cam.y) * sc;
  if (p->has_sprite(name))
    p->draw_sprite(name, sx - size * 0.5f, sy - size * 0.5f, size, size, angle_deg, alpha);
}

// Pine tree: first frame is 47×92 at 1,1 (not the whole 128 sheet).
void draw_pine(IPlatform* p, float sx, float sy, float h) {
  if (!p) return;
  const char* name = "tree_pine-sheet0";
  if (!p->has_sprite(name)) name = "tree_pine2-sheet0";
  if (!p->has_sprite(name)) return;
  const float w = h * (47.f / 92.f);
  p->draw_sprite_frame(name, sx - w * 0.5f, sy - h * 0.75f, w, h, 1, 1, 47, 92, 0.f, 1.f);
}

const char* poi_building_sprite(LocationType t, int variant) {
  switch (t) {
  case Loc_Hospital: return "b_hospital-sheet0";
  case Loc_GasStation: return "b_gas_station-sheet0";
  case Loc_Police: return "b_police_station-sheet0";
  case Loc_Firestation: return "b_fire_station-sheet0";
  case Loc_Church: return "b_church-sheet0";
  case Loc_Military: return (variant & 1) ? "b_military_barrack-sheet0" : "b_military_shtab-sheet0";
  case Loc_Helicrash: return "car_explode-sheet0";
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

} // namespace

Game::Game()
  : platform_(0), audio_(0), state_(AppState_Loading), quit_(false),
    state_timer_(0.f), attack_cd_(0.f), reload_cd_(0.f), airdrop_alert_(0.f), radio_t_(0.f),
    torch_battery_(100.f), inv_full_flash_(0.f), tent_warmth_t_(0.f), indoor_t_(0.f),
    wetness_(0.f), gasmask_filter_(100.f), adrenaline_t_(0.f), limp_t_(0.f), melee_break_mul_(1.f), has_headlamp_(false),
    gun_jammed_(false), mag_rounds_(0), mag_weapon_id_(0),
    equipped_slot_(0), in_bunker_(false), bunker_looted_(false),
    show_bag_(false), menu_difficulty_(Difficulty_Novice),
    trap_count_(0), howl_cd_(0.f), cough_cd_(0.f), tree_count_(0),
    blood_drip_count_(0), bleed_drip_cd_(0.f), crouching_(false), fuel_warn_cd_(0.f),
    rain_sfx_cd_(0.f), lightning_t_(0.f), thunder_cd_(0.f), cough_alert_t_(0.f),
    wind_sfx_cd_(0.f), flare_t_(0.f) {
  flare_pos_ = Vec2();
  data_dir_[0] = 0;
  last_airdrop_pos_ = Vec2();
  for (int i = 0; i < kMapSectorsX * kMapSectorsY; ++i) visited_sectors_[i] = false;
  prev_scream_ = false;
  for (int i = 0; i < kMaxTraps; ++i) traps_[i].active = false;
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
  return w->mag_capacity;
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

void Game::mark_sector_visited() {
  const SectorCoord c = world_.sector_at(player_.pos);
  visited_sectors_[c.sy * kMapSectorsX + c.sx] = true;
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

  // Original 1.4.1 Construct sprites (original_docs/images)
  {
    // Absolute-ish candidates relative to common launch cwds (repo / cpp / cpp/build)
    static const char* kImgCandidates[] = {
      "C:/Users/abdur/Desktop/MiniDayZ/original_docs/images",
      "../../original_docs/images",
      "../original_docs/images",
      "original_docs/images",
      "../../../original_docs/images",
      0
    };
    bool loaded = false;
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
    if (!loaded)
      std::printf("[mdaz] WARNING: original sprites not found — colored rects fallback\n");
  }

  state_ = AppState_Menu;
  state_timer_ = 0.f;
  return true;
}

void Game::shutdown() {}

void Game::start_new_game(Difficulty d) {
  player_.reset_new_game(d);
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
  inventory_.add_item(1, 1, items_);
  inventory_.add_item(18, 2, items_);
  inventory_.add_item(54, 1, items_);
  inventory_.add_item(204, 1, items_); // Fire axe — playable melee
  inventory_.add_item(151, 1, items_); // FNX
  inventory_.add_item(12, 24, items_); // .45 ACP
  inventory_.add_item(251, 1, items_); // Army helmet
  inventory_.add_item(400, 1, items_); // Bulletproof vest
  inventory_.add_item(450, 1, items_); // Taloon backpack — wear for capacity
  equipped_slot_ = 3; // fire axe slot if layout matches add order
  // Find axe slot
  for (int i = 0; i < inventory_.slot_count(); ++i) {
    if (inventory_.slot(i).id == 204) { equipped_slot_ = i; break; }
  }
  for (int i = 0; i < inventory_.slot_count(); ++i) {
    if (inventory_.slot(i).id == 450) {
      inventory_.wear_clothing(i, items_);
      break;
    }
  }
  for (int i = 0; i < inventory_.slot_count(); ++i) {
    if (inventory_.slot(i).id == 354) { inventory_.wear_clothing(i, items_); break; }
  }
  for (int i = 0; i < inventory_.slot_count(); ++i) {
    if (inventory_.slot(i).id == 353) { inventory_.wear_clothing(i, items_); break; }
  }
  mag_weapon_id_ = 0;
  mag_rounds_ = 0;
  adrenaline_t_ = 0.f;
  limp_t_ = 0.f;
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
  howl_cd_ = 0.f;
  cough_cd_ = 0.f;
  blood_drip_count_ = 0;
  bleed_drip_cd_ = 0.f;
  for (int i = 0; i < kMaxTraps; ++i) traps_[i].active = false;
  quest_.reset();
  quest_.start(Quest_KillZeds10);
  timers_.reset();
  dispersion_.reset_from_globals();
  attack_cd_ = 0.f;
  airdrop_alert_ = 0.f;
  radio_t_ = 0.f;
  torch_battery_ = 100.f;
  inv_full_flash_ = 0.f;
  tent_warmth_t_ = 0.f;
  indoor_t_ = 0.f;
  wetness_ = 0.f;
  gasmask_filter_ = 100.f;
  for (int i = 0; i < kMapSectorsX * kMapSectorsY; ++i) visited_sectors_[i] = false;
  rng_.seed(140u);
  mapgen_.generate_new_game(140);
  mapgen_.apply_collision(world_);
  // Scatter forest tree stubs around villages / cities
  tree_count_ = 0;
  for (int i = 0; i < mapgen_.location_count() && tree_count_ < kMaxTrees; ++i) {
    const MapLocation& L = mapgen_.location(i);
    if (L.type != Loc_Village && L.type != Loc_City && L.type != Loc_Secret) continue;
    const int n = (L.type == Loc_City) ? 6 : 10;
    for (int t = 0; t < n && tree_count_ < kMaxTrees; ++t) {
      trees_[tree_count_++] = Vec2(L.pos.x + rng_.range(-180.f, 180.f),
                                   L.pos.y + rng_.range(-180.f, 180.f));
    }
  }
  vehicles_.spawn(Vehicle_Car, Vec2(player_.pos.x + 90.f, player_.pos.y - 50.f));
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

  zombies_.spawn(Zed_Normal, Vec2(player_.pos.x + 160.f, player_.pos.y));
  zombies_.spawn(Zed_Fast, Vec2(player_.pos.x - 200.f, player_.pos.y + 40.f));
  zombies_.spawn(Zed_Buried, Vec2(player_.pos.x + 80.f, player_.pos.y + 140.f));
  zombies_.spawn(Zed_Army, Vec2(player_.pos.x + 280.f, player_.pos.y - 60.f));
  zombies_.spawn(Zed_ArmyShooter, Vec2(player_.pos.x + 340.f, player_.pos.y + 40.f));
  zombies_.spawn(Zed_Screamer, Vec2(player_.pos.x - 120.f, player_.pos.y - 180.f));
  zombies_.spawn(Zed_Tank, Vec2(player_.pos.x + 420.f, player_.pos.y - 140.f));
  bandits_.spawn(Vec2(player_.pos.x + 400.f, player_.pos.y + 80.f), true);
  bandits_.spawn(Vec2(player_.pos.x - 280.f, player_.pos.y + 120.f), false);
  animals_.spawn(Animal_Wolf, Vec2(player_.pos.x + 320.f, player_.pos.y - 100.f));
  animals_.spawn(Animal_Deer, Vec2(player_.pos.x - 360.f, player_.pos.y));
  loot_.spawn_random(Vec2(player_.pos.x + 50.f, player_.pos.y - 35.f), rng_);
  loot_.spawn_random(Vec2(player_.pos.x - 70.f, player_.pos.y + 55.f), rng_);
  spawn_poi_loot();
  // Extra hostiles around military / city POIs
  for (int i = 0; i < mapgen_.location_count(); ++i) {
    const MapLocation& L = mapgen_.location(i);
    if (L.type == Loc_Military) {
      zombies_.spawn(Zed_Army, Vec2(L.pos.x + 30.f, L.pos.y));
      zombies_.spawn(Zed_ArmyShooter, Vec2(L.pos.x - 40.f, L.pos.y + 20.f));
      bandits_.spawn(Vec2(L.pos.x + 60.f, L.pos.y - 30.f), true);
    } else if (L.type == Loc_City || L.type == Loc_Village) {
      zombies_.spawn(Zed_Normal, Vec2(L.pos.x + rng_.range(-50.f, 50.f),
                                      L.pos.y + rng_.range(-50.f, 50.f)));
    } else if (L.type == Loc_Hospital) {
      zombies_.spawn(Zed_Fast, Vec2(L.pos.x + 20.f, L.pos.y - 15.f));
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
}

void Game::spawn_poi_loot() {
  for (int i = 0; i < mapgen_.location_count(); ++i) {
    const MapLocation& L = mapgen_.location(i);
    int piles = 1;
    if (L.type == Loc_Military || L.type == Loc_Helicrash) piles = 3;
    else if (L.type == Loc_Hospital || L.type == Loc_City || L.type == Loc_Secret ||
             L.type == Loc_Police || L.type == Loc_Church) piles = 2;
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
        loot_.spawn_gas_station(p, rng_); // tools/fuel-ish; axe bias below
        if (rng_.next_float() < 0.55f)
          loot_.spawn_item(Vec2(p.x + 12.f, p.y), 204, 1); // fire axe
        if (rng_.next_float() < 0.4f)
          loot_.spawn_item(Vec2(p.x - 10.f, p.y), 205, 1); // crowbar
        if (rng_.next_float() < 0.3f)
          loot_.spawn_item(Vec2(p.x, p.y + 10.f), 82, 1); // hacksaw
        break;
      case Loc_Police:
        loot_.spawn_police(p, rng_);
        break;
      case Loc_Church:
        loot_.spawn_civilian(p, rng_);
        if (rng_.next_float() < 0.55f) loot_.spawn_item(Vec2(p.x + 8.f, p.y), 18, 1); // bandage
        if (rng_.next_float() < 0.4f) loot_.spawn_item(Vec2(p.x - 8.f, p.y), 25, 1);  // whiskey
        if (rng_.next_float() < 0.45f) loot_.spawn_item(Vec2(p.x, p.y + 8.f), 49, 1); // papers
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

void Game::apply_incoming_damage(float amount, bool can_bleed) {
  if (amount <= 0.f) return;
  // Worn helmet/vest stub mitigation (full C2 armor curves not extracted)
  amount *= inventory_.armor_damage_mul();
  if (amount < 0.5f) amount = 0.5f;
  // Hard hit → chance of limp (broken leg stub)
  if (amount >= 12.f && limp_t_ <= 0.f && rng_.next_float() < 0.22f) {
    limp_t_ = 45.f;
  }
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
  if (audio_) audio_->play_at("melee_swing", player_.pos.x, player_.pos.y, 0.6f);

  bool hit_something = false;
  for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
    if (!z->alive) continue;
    if (dist2(z->pos, player_.pos) <= stats.range * stats.range) {
      z->hp -= stats.damage;
      hit_something = true;
      if (z->hp <= 0.f) on_zed_killed(*z);
    }
  }
  for (int i = 0; i < animals_.count(); ++i) {
    Animal* a = animals_.at(i);
    if (!a->alive) continue;
    if (dist2(a->pos, player_.pos) <= stats.range * stats.range) {
      a->hp -= stats.damage;
      hit_something = true;
      if (a->hp <= 0.f) on_animal_killed(*a);
    }
  }
  for (Bandit* b = bandits_.begin(); b != bandits_.end(); ++b) {
    if (!b->alive) continue;
    if (dist2(b->pos, player_.pos) <= stats.range * stats.range) {
      b->hp -= stats.damage;
      hit_something = true;
      if (b->hp <= 0.f) {
        b->alive = false;
        player_.bandits_killed += 1;
        loot_.spawn_bandit(b->pos, rng_);
      }
    }
  }
  // Melee durability stub: ~4% chance weapon breaks on connecting hit (duct tape reinforces)
  if (hit_something && using_melee_item &&
      rng_.next_float() < 0.04f * melee_break_mul_) {
    inventory_.remove_at(equipped_slot_, 1);
    melee_break_mul_ = 1.f;
    if (audio_) audio_->play("hit", 0.4f);
  }
}

void Game::try_ranged_attack() {
  if (reload_cd_ > 0.f) return;
  sync_mag_for_equipped();
  const InvSlot& eq = inventory_.slot(equipped_slot_);
  if (eq.empty()) return;
  const WeaponDef* w = find_weapon(eq.id);
  if (!w || w->stats.is_melee) return;
  if (gun_jammed_) {
    if (audio_) audio_->play("melee_swing", 0.3f);
    attack_cd_ = 0.25f;
    return;
  }

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

  // Jam stub: ~2% when wet or mag almost empty — clear with V
  if (w->mag_capacity > 0 &&
      (wetness_ > 70.f || mag_rounds_ <= 1) &&
      rng_.next_float() < 0.02f) {
    gun_jammed_ = true;
    attack_cd_ = 0.4f;
    if (audio_) audio_->play("melee_swing", 0.35f);
    return;
  }

  attack_cd_ = w->stats.fire_interval;
  // Silencer 5.45/5.56 (80/81) quiets report + reduces hear pull
  bool silenced = false;
  for (int i = 0; i < inventory_.slot_count(); ++i) {
    const ItemId sid = inventory_.slot(i).id;
    if (sid == 80 || sid == 81) { silenced = true; break; }
  }
  if (audio_) audio_->play_at("gunshot", player_.pos.x, player_.pos.y, silenced ? 0.28f : 0.8f);
  {
    const float hear_r = silenced ? 90.f : 420.f;
    const float boost = silenced ? 260.f : 450.f;
    for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
      if (!z->alive) continue;
      if (dist2(z->pos, player_.pos) > hear_r * hear_r) continue;
      if (z->aggro_radius < boost) z->aggro_radius = boost;
    }
    // Loud shots stampede deer/rabbits; silenced barely scares
    const float scare_r = silenced ? 120.f : 480.f;
    for (int ai = 0; ai < animals_.count(); ++ai) {
      Animal* a = animals_.at(ai);
      if (!a->alive) continue;
      if (a->type != Animal_Deer && a->type != Animal_Rabbit && a->type != Animal_Crow) continue;
      if (dist2(a->pos, player_.pos) > scare_r * scare_r) continue;
      const float dx = a->pos.x - player_.pos.x;
      const float dy = a->pos.y - player_.pos.y;
      const float len = std::sqrt(dx * dx + dy * dy);
      if (len > 1.f) {
        const float push = silenced ? 40.f : 110.f;
        a->pos.x += (dx / len) * push;
        a->pos.y += (dy / len) * push;
      }
    }
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
  const float ang = player_.facing_deg + ang_off;
  const float dmg = roll_bullet_damage(w->stats.damage, rng_);
  bullets_.spawn(player_.pos, ang, 520.f, dmg, w->stats.range / 520.f + 0.15f, true);
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

void Game::on_animal_killed(Animal& a) {
  a.alive = false;
  player_.animals_killed += 1;
  if (player_.animals_killed >= 5) player_.perk_hunter = true;
  ItemId meat = 60; // Raw steak default (deer / large)
  int n = player_.perk_hunter ? 2 : 1;
  if (a.type == Animal_Rabbit || a.type == Animal_Crow) {
    meat = 71; // Rabbit meat
  } else if (a.type == Animal_Deer) {
    n = player_.perk_hunter ? 3 : 2; // deer yields more
  } else if (a.type == Animal_Wolf || a.type == Animal_Dog) {
    meat = 60;
    n = player_.perk_hunter ? 2 : 1;
  }
  loot_.spawn_item(a.pos, meat, n);
  // Crow: also drop sticks (feather substitute for fletching)
  if (a.type == Animal_Crow && rng_.next_float() < 0.6f) {
    loot_.spawn_item(Vec2(a.pos.x + 8.f, a.pos.y), 38, 1);
  }
}

void Game::on_zed_killed(Zombie& z) {
  z.alive = false;
  player_.zeds_killed += 1;
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
  if (audio_) audio_->play_at("gunshot", pos.x, pos.y, 1.f);
  if (leave_fire) campfires_.place(pos, 25.f);
  for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
    if (!z->alive) continue;
    if (dist2(z->pos, pos) > r2) continue;
    z->hp -= damage;
    if (z->hp <= 0.f) on_zed_killed(*z);
  }
  for (int i = 0; i < animals_.count(); ++i) {
    Animal* a = animals_.at(i);
    if (!a->alive) continue;
    if (dist2(a->pos, pos) > r2) continue;
    a->hp -= damage * 0.8f;
    if (a->hp <= 0.f) on_animal_killed(*a);
  }
  for (Bandit* bd = bandits_.begin(); bd != bandits_.end(); ++bd) {
    if (!bd->alive) continue;
    if (dist2(bd->pos, pos) > r2) continue;
    bd->hp -= damage;
    if (bd->hp <= 0.f) {
      bd->alive = false;
      player_.bandits_killed += 1;
      loot_.spawn_bandit(bd->pos, rng_);
    }
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
        z->hp -= b->damage;
        b->alive = false;
        if (z->hp <= 0.f) on_zed_killed(*z);
        break;
      }
    }
    if (!b->alive) continue;
    for (int i = 0; i < animals_.count(); ++i) {
      Animal* a = animals_.at(i);
      if (!a->alive) continue;
      if (dist2(b->pos, a->pos) < 16.f * 16.f) {
        a->hp -= b->damage;
        b->alive = false;
        if (a->hp <= 0.f) on_animal_killed(*a);
        break;
      }
    }
    if (!b->alive) continue;
    for (Bandit* bd = bandits_.begin(); bd != bandits_.end(); ++bd) {
      if (!bd->alive) continue;
      if (dist2(b->pos, bd->pos) < 16.f * 16.f) {
        bd->hp -= b->damage;
        b->alive = false;
        if (bd->hp <= 0.f) {
          bd->alive = false;
          player_.bandits_killed += 1;
          loot_.spawn_bandit(bd->pos, rng_);
        }
        break;
      }
    }
  }
}

void Game::try_craft() {
  const bool at_fire = campfires_.near_fire(player_.pos);
  // Prefer improvised splint while limping (sticks+rags → bandage + clear limp)
  if (limp_t_ > 0.f) {
    for (int i = 0; i < craft_recipe_count(); ++i) {
      const CraftRecipe* r = craft_recipe_at(i);
      if (!r || std::strcmp(r->name, "Craft_improvised_splint") != 0) continue;
      if (craft_do(inventory_, *r, items_)) {
        limp_t_ = 0.f;
        if (audio_) audio_->play("craft", 0.7f);
        return;
      }
    }
  }
  // Prefer cook recipes when at fire and holding pan (210)
  bool has_pan = false;
  for (int i = 0; i < inventory_.slot_count(); ++i) {
    if (inventory_.slot(i).id == 210) { has_pan = true; break; }
  }
  if (at_fire && has_pan) {
    for (int i = 0; i < craft_recipe_count(); ++i) {
      const CraftRecipe* r = craft_recipe_at(i);
      if (!r || !r->requires_fire) continue;
      if (craft_do(inventory_, *r, items_)) {
        if (audio_) audio_->play("craft", 0.75f);
        return;
      }
    }
  }
  for (int i = 0; i < craft_recipe_count(); ++i) {
    const CraftRecipe* r = craft_recipe_at(i);
    if (!r) continue;
    if (r->requires_fire && !at_fire) continue;
    if (craft_do(inventory_, *r, items_)) {
      if (r->name && std::strcmp(r->name, "Craft_improvised_splint") == 0) limp_t_ = 0.f;
      if (audio_) audio_->play("craft", 0.7f);
      return;
    }
  }
}

void Game::try_drop_equipped() {
  InvSlot& s = inventory_.slot(equipped_slot_);
  if (s.empty()) return;
  const ItemId id = s.id;
  if (!inventory_.remove_at(equipped_slot_, 1)) return;
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
  if (audio_) audio_->play("craft", 0.55f);
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
  // Hand-drill stub: need ≥2 sticks, ~25% success, short fire
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
  if (rng_.next_float() < 0.25f) {
    campfires_.place(player_.pos, 50.f);
    if (audio_) audio_->play("craft", 0.4f);
  } else if (audio_) {
    audio_->play("melee_swing", 0.25f);
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
              loot_.spawn_bandit(b->pos, rng_);
            }
            break;
          }
        }
      }
      if (!t.sprung && dist2(player_.pos, t.pos) < 18.f * 18.f) {
        apply_incoming_damage(18.f, true);
        limp_t_ = 35.f;
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
        loot_.spawn_hospital(bunker_spot(40.f, 0.f), rng_);
        loot_.spawn_airdrop(bunker_spot(-50.f, 30.f), rng_);
        loot_.spawn_civilian(bunker_spot(20.f, -40.f), rng_);
        zombies_.spawn(Zed_Normal, bunker_spot(90.f, 0.f));
        zombies_.spawn(Zed_Fast, bunker_spot(-70.f, 40.f));
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
    const float dmg = roll_bullet_damage(18.f, rng_);
    bullets_.spawn(z->pos, ang, 480.f, dmg, 0.95f, false);
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
    const float dmg = roll_bullet_damage(20.f, rng_);
    bullets_.spawn(b->pos, ang, 500.f, dmg, 0.9f, false);
    b->attack_cooldown = 1.15f;
    if (audio_) audio_->play("gunshot", 0.35f);
  }
}

void Game::update_menu(float dt, const InputState& in) {
  (void)dt;
  if (in.slot_digit == 1) menu_difficulty_ = Difficulty_Novice;
  if (in.slot_digit == 2) menu_difficulty_ = Difficulty_Regular;
  if (in.slot_digit == 3) menu_difficulty_ = Difficulty_Veteran;
  if (in.load_pressed) {
    int qact = 0, qprog = 0;
    int mag = 0;
    float wet = 0.f;
    float gfilt = 100.f;
    int fish = 0;
    if (SaveSystem::load_file(*platform_, "save.mdaz", player_, inventory_, weather_,
                              &equipped_slot_, visited_sectors_,
                              kMapSectorsX * kMapSectorsY,
                              &torch_battery_, &qact, &qprog, &mag, &wet,
                              &gfilt, &fish)) {
      in_bunker_ = false;
      bullets_.clear();
      zombies_.clear();
      animals_.clear();
      bandits_.clear();
      campfires_.clear();
      loot_.clear();
      mag_rounds_ = mag;
      wetness_ = wet;
      gasmask_filter_ = gfilt;
      player_.fish_caught = fish;
      mag_weapon_id_ = inventory_.slot(equipped_slot_).id;
      mark_sector_visited();
      quest_.reset();
      if (qact > Quest_None && qact < Quest_Count) {
        quest_.start(static_cast<QuestId>(qact));
        quest_.progress = qprog;
      }
      zombies_.spawn(Zed_Normal, Vec2(player_.pos.x + 140.f, player_.pos.y));
      bandits_.spawn(Vec2(player_.pos.x - 160.f, player_.pos.y + 40.f), true);
      state_ = AppState_Playing;
      return;
    }
  }
  if (in.confirm_pressed || in.attack_pressed) {
    start_new_game(menu_difficulty_);
  }
  if (in.cancel) quit_ = true;
}

void Game::update_playing(float dt, const InputState& in) {
  if (in.pause_pressed) {
    state_ = AppState_Paused;
    return;
  }

  if (in.slot_digit >= 1 && in.slot_digit <= 8) {
    equipped_slot_ = in.slot_digit - 1;
    sync_mag_for_equipped();
  }
  if (in.equip_delta != 0) {
    equipped_slot_ = (equipped_slot_ + in.equip_delta) % 8;
    if (equipped_slot_ < 0) equipped_slot_ += 8;
    sync_mag_for_equipped();
  }

  if (adrenaline_t_ > 0.f) {
    adrenaline_t_ -= dt;
    if (adrenaline_t_ < 0.f) adrenaline_t_ = 0.f;
  }
  if (limp_t_ > 0.f) {
    limp_t_ -= dt;
    if (limp_t_ < 0.f) limp_t_ = 0.f;
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
      if (player_.pos.x < margin || player_.pos.y < margin ||
          player_.pos.x > kMapLayoutW - margin || player_.pos.y > kMapLayoutH - margin) {
        mx *= 0.55f;
        my *= 0.55f;
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
    if (limp_t_ > 0.f) {
      mx *= 0.55f;
      my *= 0.55f;
    }
    crouching_ = in.crouch;
    if (crouching_) {
      mx *= 0.45f;
      my *= 0.45f;
    }
    const bool can_sprint = limp_t_ <= 0.f && !crouching_ &&
                            player_.survival.food > 8.f &&
                            player_.survival.water > 8.f;
    player_.apply_input(mx, my, in.run && can_sprint, dt);
    if (!in_bunker_ && world_.is_blocked(player_.pos.x, player_.pos.y)) {
      player_.pos.x -= player_.vel.x * dt;
      player_.pos.y -= player_.vel.y * dt;
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
    // Crash into blocked cells while driving
    if (world_.is_blocked(player_.pos.x, player_.pos.y)) {
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
        loot_.spawn_item(trees_[ti], 38, 2 + rng_.next_int(0, 2));
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
    if (!in_bunker_ && weather_.rain > 0.7f && indoor_t_ <= 0.f) {
      wetness_ += (raincoat ? 4.f : 14.f) * dt;
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
    if (wetness_ > 40.f) insulation -= 3.f; // wet clothes chill
    if (wetness_ > 75.f) insulation -= 2.f; // soaked — worse
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
          if (audio_) audio_->play("craft", 0.35f);
          break;
        }
      }
    }
  }
  timers_.update(dt);
  dispersion_.update(dt);
  if (attack_cd_ > 0.f) attack_cd_ -= dt;
  if (reload_cd_ > 0.f) reload_cd_ -= dt;
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
    if (timers_.consume_zed_respawn() && zombies_.alive_count() < 12) {
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
      // Cough noise briefly expands hear radius effect: nudge nearby zeds
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
    float animal_dmg = animals_.update(dt, player_.pos, player_.survival.bleeding > 0.f);
    float bandit_dmg = bandits_.update(dt, player_.pos);
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
    const int animal_cap = weather_.is_night ? 14 : 10;
    if (timers_.consume_animal_respawn() && animals_.alive_count() < animal_cap) {
      const float ang = rng_.next_float() * 6.28318f;
      const float dist = 260.f + rng_.next_float() * 220.f;
      const Vec2 base(player_.pos.x + std::cos(ang) * dist,
                      player_.pos.y + std::sin(ang) * dist);
      const float roll = rng_.next_float();
      const float wolf_chance = weather_.is_night ? 0.62f : 0.45f;
      if (roll < wolf_chance && animals_.alive_of_type(Animal_Wolf) < 7) {
        // Wolf pack (2–3, +1 at night)
        const int pack = 2 + rng_.next_int(0, 1) + (weather_.is_night ? 1 : 0);
        for (int i = 0; i < pack; ++i) {
          animals_.spawn(Animal_Wolf, Vec2(base.x + rng_.range(-40.f, 40.f),
                                           base.y + rng_.range(-40.f, 40.f)));
        }
      } else if (roll < wolf_chance + 0.25f) {
        animals_.spawn(Animal_Deer, base);
      } else if (roll < wolf_chance + 0.37f) {
        animals_.spawn(Animal_Rabbit, base);
      } else if (roll < wolf_chance + 0.48f) {
        // Crow flock
        const int n = 3 + rng_.next_int(0, 2);
        for (int ci = 0; ci < n; ++ci) {
          animals_.spawn(Animal_Crow, Vec2(base.x + rng_.range(-50.f, 50.f),
                                           base.y + rng_.range(-50.f, 50.f)));
        }
      } else {
        animals_.spawn(Animal_Dog, base);
      }
    }
    mark_sector_visited();
  }

  // Zeds update in overworld and bunker (bunker dens use same system)
  {
    float zed_dmg = 0.f;
    // Standing still / walking quieter than sprint; crouch quieter still
    const bool loud = player_.running && !in.crouch;
    zombies_.update(dt, player_.pos, zed_dmg, loud, in.crouch,
                    player_.survival.bleeding > 0.f,
                    cough_alert_t_ > 0.f);
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
          if (r < 0.2f) inventory_.add_item(47 + rng_.next_int(0, 1), 1, items_); // seeds
          else if (r < 0.35f) inventory_.add_item(97, 1, items_); // mushroom
          else if (r < 0.55f) inventory_.add_item(38, 1, items_); // stick
          else if (r < 0.7f) inventory_.add_item(26, 1, items_);  // rags scrap
          else if (r < 0.8f) inventory_.add_item(27, 1, items_);  // berry
          if (audio_) audio_->play("melee_swing", 0.35f);
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
          if (rng_.next_float() < 0.06f * melee_break_mul_) {
            const ItemId aid = inventory_.slot(equipped_slot_).id;
            if (aid == 202 || aid == 204 || aid == 201 || aid == 206) {
              inventory_.remove_at(equipped_slot_, 1);
              if (audio_) audio_->play("hit", 0.45f);
            }
            melee_break_mul_ = 1.f;
          }
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
          // Mostly perch; rare ruffe for big fillet cook (bait → more ruffe)
          const float ruffe_p = used_bait ? 0.38f : 0.22f;
          inventory_.add_item(rng_.next_float() < ruffe_p ? 93 : 94, 1, items_);
          player_.fish_caught += 1;
          if (audio_) audio_->play("pickup", 0.4f);
        } else if (!near_water && audio_) {
          audio_->play("melee_swing", 0.15f); // cast on dry land — nothing
        }
      } else if (!has_rod && !has_axe) {
        // Plant seeds when equipped (instant harvest stub; farmer yields +1)
        const ItemId seed_eq = inventory_.slot(equipped_slot_).id;
        ItemId crop = 0;
        if (seed_eq == 68) crop = 31;      // Zucchini seeds → zucchini
        else if (seed_eq == 47) crop = 5;  // Tomato seeds → tomato
        else if (seed_eq == 48) crop = 32; // Pepper seeds → bell pepper
        if (crop != 0) {
          if (inventory_.remove_at(equipped_slot_, 1)) {
            inventory_.add_item(crop, 1 + (player_.perk_farmer ? 1 : 0), items_);
            player_.forage_count += 1;
            if (player_.forage_count >= 8) player_.perk_farmer = true;
            if (audio_) audio_->play("craft", 0.45f);
          }
        } else {
        // Search building interior near POI (warmth + scrap loot)
        // Crowbar (205) pry: better find chance + toolbox/lock scrap
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
            static const ItemId kIndoor[] = { 1, 8, 18, 26, 30, 39, 42, 54 };
            inventory_.add_item(kIndoor[rng_.next_int(0, 7)], 1, items_);
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
          if (audio_) audio_->play("pickup", 0.3f);
        } else if (!searched && rng_.next_float() < bush_chance) {
          // Bush forage: sticks / berries / rags
          static const ItemId kBush[] = { 38, 27, 26, 33, 97, 73 };
          ItemId id = kBush[rng_.next_int(0, 5)];
          inventory_.add_item(id, 1 + (player_.perk_farmer ? 1 : 0), items_);
          player_.forage_count += 1;
          if (player_.forage_count >= 8) player_.perk_farmer = true;
          if (audio_) audio_->play("pickup", 0.35f);
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
    for (int i = 0; i < mapgen_.location_count(); ++i) {
      const MapLocation& L = mapgen_.location(i);
      if (L.type == Loc_BunkerEntrance && dist2(L.pos, player_.pos) < 80.f * 80.f)
        near_bunker = true;
      if (L.type == Loc_Hospital && dist2(L.pos, player_.pos) < 90.f * 90.f)
        near_hospital = true;
      if (L.type == Loc_Police && dist2(L.pos, player_.pos) < 90.f * 90.f)
        near_police = true;
    }
    const QuestId was = quest_.active;
    if (quest_.tick(player_.days_survived, player_.zeds_killed, near_bunker || in_bunker_,
                    near_hospital, player_.bandits_killed, player_.fish_caught, near_police)) {
      if (audio_) audio_->play("craft", 0.9f);
      const QuestId nxt = quest_.next_after(was);
      if (nxt != Quest_None && !quest_.completed[nxt]) quest_.start(nxt);
    }
  }

  if (in.reload_pressed) try_craft();
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
  show_bag_ = in.bag;

  if (in.save_pressed) {
    SaveSystem::save_file(*platform_, "save.mdaz", player_, inventory_, weather_,
                          equipped_slot_, visited_sectors_,
                          kMapSectorsX * kMapSectorsY,
                          torch_battery_, static_cast<int>(quest_.active), quest_.progress,
                          mag_rounds_, wetness_, gasmask_filter_, player_.fish_caught);
  }
  if (in.load_pressed) {
    int qact = 0, qprog = 0;
    int mag = 0;
    float wet = 0.f;
    float gfilt = 100.f;
    int fish = 0;
    if (SaveSystem::load_file(*platform_, "save.mdaz", player_, inventory_, weather_,
                              &equipped_slot_, visited_sectors_,
                              kMapSectorsX * kMapSectorsY,
                              &torch_battery_, &qact, &qprog, &mag, &wet,
                              &gfilt, &fish)) {
      in_bunker_ = false;
      bullets_.clear();
      campfires_.clear();
      mag_rounds_ = mag;
      wetness_ = wet;
      gasmask_filter_ = gfilt;
      player_.fish_caught = fish;
      mag_weapon_id_ = inventory_.slot(equipped_slot_).id;
      mark_sector_visited();
      quest_.reset();
      if (qact > Quest_None && qact < Quest_Count) {
        quest_.start(static_cast<QuestId>(qact));
        quest_.progress = qprog;
      }
    }
  }

  if (in.inventory_pressed) {
    // Gasoline (53) refuel / Car toolbox (59) repair
    bool vehicle_item = false;
    for (int i = 0; i < inventory_.slot_count() && !vehicle_item; ++i) {
      const ItemId id = inventory_.slot(i).id;
      if (inventory_.slot(i).count <= 0) continue;
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
            limp_t_ = 0.f;
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
            if (audio_) audio_->play("pickup", 0.4f);
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
        } else if (eq.id == 75) { // Radio — airdrop ETA (needs battery charge)
          if (torch_battery_ > 5.f) {
            torch_battery_ -= 3.f;
            radio_t_ = 6.f;
            used_special = true;
            if (audio_) audio_->play("airdrop", 0.35f);
          } else if (audio_) {
            audio_->play("melee_swing", 0.2f);
            used_special = true;
          }
        } else if (eq.id == 39) { // Duct tape — reinforce equipped melee
          const WeaponDef* mw = find_weapon(eq.id);
          (void)mw;
          bool has_melee = false;
          for (int i = 0; i < inventory_.slot_count(); ++i) {
            const WeaponDef* w = find_weapon(inventory_.slot(i).id);
            if (w && w->stats.is_melee) { has_melee = true; break; }
          }
          if (has_melee && inventory_.remove_at(equipped_slot_, 1)) {
            melee_break_mul_ = 0.25f;
            used_special = true;
            if (audio_) audio_->play("craft", 0.5f);
          }
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
        if (!inventory_.use_consumable(equipped_slot_, player_.survival, items_, &rng_)) {
          for (int i = 0; i < inventory_.slot_count(); ++i) {
            const ItemId id = inventory_.slot(i).id;
            if (inventory_.use_consumable(i, player_.survival, items_, &rng_)) {
              if (id == 45) { adrenaline_t_ = 18.f; limp_t_ = 0.f; }
              if (id == 18 || id == 26) limp_t_ = 0.f; // bandage/rags as splint stub
              break;
            }
          }
        } else {
          if (before == 45) { adrenaline_t_ = 18.f; limp_t_ = 0.f; }
          if (before == 18 || before == 26) limp_t_ = 0.f;
          (void)was_bleed;
        }
      }
    }
  }

  if (player_.survival.is_dead()) {
    write_death_log();
    state_ = AppState_Dead;
    state_timer_ = 0.f;
    in_bunker_ = false;
    if (audio_) audio_->play("death", 0.8f);
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
  InputState in;
  if (!platform_->poll(in)) {
    quit_ = true;
    return;
  }

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
    if (in.pause_pressed || in.confirm_pressed) state_ = AppState_Playing;
    if (in.cancel) state_ = AppState_Menu;
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
  platform_->begin_frame();

  const float cx = static_cast<float>(platform_->screen_w()) * 0.5f;
  const float cy = static_cast<float>(platform_->screen_h()) * 0.5f;
  const float sc = world_to_screen_scale();

  if (state_ == AppState_Menu) {
    platform_->clear(0.06f, 0.07f, 0.05f);
    if (platform_->has_sprite("tilemap_forest.png") || platform_->has_sprite("tilemap_forest")) {
      // Tiled original forest backdrop
      for (int ty = 0; ty < 3; ++ty)
        for (int tx = 0; tx < 3; ++tx)
          platform_->draw_sprite("tilemap_forest", (float)(tx * 160 - 20), (float)(ty * 120 - 20),
                                 180.f, 140.f, 0.f, 0.55f);
    } else {
      platform_->draw_rect(0, 0, 480, 272, 0.08f, 0.1f, 0.07f, 1.f);
    }
    if (platform_->has_sprite("player_skin_def_1-sheet0"))
      platform_->draw_sprite("player_skin_def_1-sheet0", 360, 140, 72, 72, 0.f, 1.f);
    platform_->draw_text(40, 50, "MINI DAYZ C++", 0.95f, 0.9f, 0.75f);
    platform_->draw_text(40, 75, "BASELINE 1.4.1", 0.7f, 0.75f, 0.6f);
    platform_->draw_text(40, 110, "1 NOVICE  2 REGULAR  3 VETERAN", 0.75f, 0.8f, 0.65f);
    const char* diff = "NOVICE";
    if (menu_difficulty_ == Difficulty_Regular) diff = "REGULAR";
    if (menu_difficulty_ == Difficulty_Veteran) diff = "VETERAN";
    char dbuf[64];
    std::snprintf(dbuf, sizeof(dbuf), "DIFFICULTY: %s", diff);
    platform_->draw_text(40, 130, dbuf, 0.9f, 0.85f, 0.55f);
    platform_->draw_text(40, 160, "ENTER - NEW GAME", 0.8f, 0.85f, 0.7f);
    platform_->draw_text(40, 180, "F9 - CONTINUE SAVE", 0.75f, 0.8f, 0.65f);
    platform_->draw_text(40, 200, "ESC - QUIT", 0.65f, 0.7f, 0.6f);
  } else if (state_ == AppState_Playing || state_ == AppState_Paused) {
    if (in_bunker_) platform_->clear(0.08f, 0.07f, 0.06f);
    else if (weather_.is_night) platform_->clear(0.05f, 0.06f, 0.08f);
    else platform_->clear(0.14f, 0.18f, 0.11f);

    if (in_bunker_ && bunker_.width > 0) {
      const float tile = 8.f;
      const float bsc = 1.0f;
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
          float r = 0.28f, g = 0.24f, b = 0.2f;
          // Floors vs walls (heuristic matching is_blocked_world walkables)
          if (tile_id == 6 || tile_id == 16 || tile_id == 20 || tile_id == 1) {
            r = 0.18f; g = 0.17f; b = 0.15f;
          } else {
            r = 0.42f; g = 0.36f; b = 0.3f;
          }
          platform_->draw_rect(sx, sy, tile, tile, r, g, b, 1.f);
        }
      }
      platform_->draw_text(8, 220, "BUNKER  E - EXIT", 0.75f, 0.7f, 0.55f);
    } else {
      // Ground: original forest tilemap (tiled), fallback checker
      {
        const float tile = 96.f;
        const bool has_forest = platform_->has_sprite("tilemap_forest");
        for (int gx = -4; gx <= 4; ++gx) {
          for (int gy = -3; gy <= 3; ++gy) {
            const float wx = player_.pos.x + gx * (tile / sc);
            const float wy = player_.pos.y + gy * (tile / sc);
            const float sx = cx + (wx - player_.pos.x) * sc;
            const float sy = cy + (wy - player_.pos.y) * sc;
            const float margin = 400.f;
            const bool water = wx < margin || wy < margin ||
              wx > kMapLayoutW - margin || wy > kMapLayoutH - margin;
            if (water) {
              platform_->draw_rect(sx - tile * 0.5f, sy - tile * 0.5f, tile, tile,
                                   0.12f, 0.22f, 0.35f, 1.f);
            } else if (has_forest) {
              platform_->draw_sprite("tilemap_forest",
                                     sx - tile * 0.5f, sy - tile * 0.5f, tile, tile, 0.f, 1.f);
            } else {
              const bool alt = ((gx + gy) & 1) != 0;
              platform_->draw_rect(sx - tile * 0.5f, sy - tile * 0.5f, tile, tile,
                                   alt ? 0.16f : 0.13f, alt ? 0.20f : 0.17f, alt ? 0.12f : 0.10f, 1.f);
            }
          }
        }
      }

      // Blood drips
      for (int i = 0; i < blood_drip_count_; ++i) {
        if (blood_drips_[i].life <= 0.f) continue;
        if (dist2(blood_drips_[i].pos, player_.pos) > 350.f * 350.f) continue;
        float sx = cx + (blood_drips_[i].pos.x - player_.pos.x) * sc;
        float sy = cy + (blood_drips_[i].pos.y - player_.pos.y) * sc;
        const float a = blood_drips_[i].life / 8.f;
        if (platform_->has_sprite("blood_drop-sheet0"))
          platform_->draw_sprite("blood_drop-sheet0", sx - 3, sy - 3, 6, 6, 0.f, 0.35f + 0.5f * a);
        else
          platform_->draw_rect(sx - 2, sy - 2, 4, 3, 0.55f, 0.05f, 0.08f, 0.25f + 0.45f * a);
      }

      // Forest trees — crop first pine frame (47×92), not whole sheet
      for (int i = 0; i < tree_count_; ++i) {
        if (dist2(trees_[i], player_.pos) > 380.f * 380.f) continue;
        float sx = cx + (trees_[i].x - player_.pos.x) * sc;
        float sy = cy + (trees_[i].y - player_.pos.y) * sc;
        draw_pine(platform_, sx, sy, 70.f);
      }

      // Dirt roads between nearby POIs (scaffold until tile roads ported)
      for (int i = 0; i < mapgen_.location_count(); ++i) {
        const MapLocation& A = mapgen_.location(i);
        for (int j = i + 1; j < mapgen_.location_count(); ++j) {
          const MapLocation& B = mapgen_.location(j);
          const float dx = B.pos.x - A.pos.x;
          const float dy = B.pos.y - A.pos.y;
          const float d2ab = dx * dx + dy * dy;
          if (d2ab < 200.f * 200.f || d2ab > 900.f * 900.f) continue;
          const float len = std::sqrt(d2ab);
          const int steps = static_cast<int>(len / 28.f);
          if (steps < 2) continue;
          for (int s = 1; s < steps; ++s) {
            const float t = static_cast<float>(s) / static_cast<float>(steps);
            const float wx = A.pos.x + dx * t;
            const float wy = A.pos.y + dy * t;
            if (dist2(Vec2(wx, wy), player_.pos) > 420.f * 420.f) continue;
            float sx = cx + (wx - player_.pos.x) * sc;
            float sy = cy + (wy - player_.pos.y) * sc;
            platform_->draw_rect(sx - 4, sy - 4, 8, 8, 0.28f, 0.24f, 0.16f, 0.55f);
          }
        }
      }

      // Map POIs — original building sprites + labels when close
      for (int i = 0; i < mapgen_.location_count(); ++i) {
        const MapLocation& L = mapgen_.location(i);
        float sx = cx + (L.pos.x - player_.pos.x) * sc;
        float sy = cy + (L.pos.y - player_.pos.y) * sc;
        const float d2p = dist2(L.pos, player_.pos);
        if (d2p < 520.f * 520.f) {
          const char* bspr = poi_building_sprite(L.type, i);
          float bsz = 90.f;
          if (L.type == Loc_Hospital || L.type == Loc_City) bsz = 110.f;
          if (L.type == Loc_Military) bsz = 100.f;
          if (L.type == Loc_Helicrash) bsz = 64.f;
          if (platform_->has_sprite(bspr))
            platform_->draw_sprite(bspr, sx - bsz * 0.5f, sy - bsz * 0.5f, bsz, bsz, 0.f, 1.f);
          else
            platform_->draw_rect(sx - 8, sy - 8, 16, 16, 0.5f, 0.45f, 0.3f, 0.9f);
        }
        if (d2p < 450.f * 450.f && sx > 20.f && sx < 440.f && sy > 20.f && sy < 220.f) {
          platform_->draw_text(sx - 10.f, sy - 14.f, location_label(L.type), 0.85f, 0.85f, 0.7f);
        }
      }
    }

    // Loot (overworld + bunker dens) — bunker uses 1:1 scale so use matching sc
    {
      const float lsc = in_bunker_ ? 1.f : sc;
      for (int i = 0; i < loot_.count(); ++i) {
        const LootPile* p = loot_.at(i);
        if (!p->active) continue;
        float sx = cx + (p->pos.x - player_.pos.x) * lsc;
        float sy = cy + (p->pos.y - player_.pos.y) * lsc;
        if (p->is_airdrop) {
          if (platform_->has_sprite("airdrop_box-sheet0"))
            platform_->draw_sprite("airdrop_box-sheet0", sx - 10, sy - 8, 20, 16, 0.f, 1.f);
          else
            platform_->draw_rect(sx - 6, sy - 6, 12, 12, 0.95f, 0.45f, 0.1f, 1.f);
        } else if (platform_->has_sprite("spawn_loot_point-sheet0")) {
          platform_->draw_sprite("spawn_loot_point-sheet0", sx - 6, sy - 6, 12, 12, 0.f, 1.f);
        } else {
          platform_->draw_rect(sx - 3, sy - 3, 6, 6, 0.9f, 0.85f, 0.2f, 1.f);
        }
      }
    }

    // Zombies — one 30×30 C2 frame (facing col + walk row), not whole sheet
    {
      const float zsc = in_bunker_ ? 1.f : sc;
      const int zanim = 1 + (static_cast<int>(state_timer_ * 6.f) % 3);
      for (const Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
        if (!z->alive) continue;
        float sx = cx + (z->pos.x - player_.pos.x) * zsc;
        float sy = cy + (z->pos.y - player_.pos.y) * zsc;
        float zsz = 40.f;
        if (z->type == Zed_Buried && z->ambush_boost <= 0.f) {
          if (platform_->has_sprite("zed_buried_hidden-sheet0"))
            platform_->draw_sprite_frame("zed_buried_hidden-sheet0",
                                        sx - 16, sy - 10, 32, 20, 1, 1, 30, 30, 0.f, 1.f);
          else
            platform_->draw_rect(sx - 7, sy - 3, 14, 8, 0.32f, 0.28f, 0.18f, 0.9f);
          continue;
        }
        const char* base = "zed_normal_skin1";
        if (z->type == Zed_Fast) base = "zed_fast_skin1";
        else if (z->type == Zed_Army || z->type == Zed_ArmyShooter) base = "zed_army_skin1";
        else if (z->type == Zed_Buried) base = "zed_buried_skin";
        else if (z->type == Zed_Tank) { base = "zed_tank_skin"; zsz = 56.f; }
        else if (z->type == Zed_Screamer) base = "zed_screamer_skin";
        const float fdeg = std::atan2(player_.pos.y - z->pos.y, player_.pos.x - z->pos.x) * 57.2957795f;
        char probe[64];
        sprite_sheet_name(probe, (int)sizeof(probe), base, 0);
        if (platform_->has_sprite(probe))
          draw_c2_char(platform_, sx, sy, zsz, base, fdeg, zanim);
        else
          platform_->draw_rect(sx - 5, sy - 5, 10, 10, 0.7f, 0.2f, 0.15f, 1.f);
        platform_->draw_rect(sx - 5, sy - zsz * 0.5f - 3.f, 10.f * (z->hp / z->max_hp), 2, 0.9f, 0.2f, 0.15f, 1.f);
      }
    }

    if (!in_bunker_) {
      // Airdrop compass while alert active
      if (airdrop_alert_ > 0.f) {
        float sx = cx + (last_airdrop_pos_.x - player_.pos.x) * sc;
        float sy = cy + (last_airdrop_pos_.y - player_.pos.y) * sc;
        platform_->draw_rect(sx - 8, sy - 8, 16, 16, 1.f, 0.8f, 0.1f, 0.35f);
      }

      // Screamer pulse ring
      if (zombies_.scream_active()) {
        const Vec2 sp = zombies_.scream_pos();
        float sx = cx + (sp.x - player_.pos.x) * sc;
        float sy = cy + (sp.y - player_.pos.y) * sc;
        const float pulse = 20.f + 12.f * std::fabs(std::sin(state_timer_ * 8.f));
        platform_->draw_rect(sx - pulse, sy - pulse, pulse * 2.f, pulse * 2.f,
                             0.9f, 0.3f, 0.85f, 0.18f);
        platform_->draw_rect(sx - pulse * 0.5f, sy - pulse * 0.5f, pulse, pulse,
                             0.95f, 0.5f, 0.9f, 0.22f);
      }

      // Animals — cropped frames (wolf 30×30, deer 38×38)
      {
        const int aanim = 1 + (static_cast<int>(state_timer_ * 5.f) % 3);
        for (int i = 0; i < animals_.count(); ++i) {
          const Animal* a = animals_.at(i);
          if (!a->alive) continue;
          float sx = cx + (a->pos.x - player_.pos.x) * sc;
          float sy = cy + (a->pos.y - player_.pos.y) * sc;
          const float fdeg = std::atan2(player_.pos.y - a->pos.y, player_.pos.x - a->pos.x) * 57.2957795f;
          if (a->type == Animal_Wolf || a->type == Animal_Dog) {
            draw_c2_char(platform_, sx, sy, 42.f, "wolf_skin", fdeg, aanim);
          } else if (a->type == Animal_Deer) {
            // deer cells are 38×38 with stride 40
            draw_c2_char(platform_, sx, sy, 46.f, "deer_skin", fdeg, aanim, 38, 40);
          } else if (a->type == Animal_Rabbit) {
            draw_c2_char(platform_, sx, sy, 28.f, "rabbit_skin", fdeg, aanim);
          } else {
            platform_->draw_rect(sx - 3, sy - 3, 6, 6, 0.2f, 0.2f, 0.22f, 1.f);
          }
        }
      }

      // Bandits — 30×30 frames
      {
        const int banim = 1 + (static_cast<int>(state_timer_ * 5.f) % 3);
        for (const Bandit* bd = bandits_.begin(); bd != bandits_.end(); ++bd) {
          if (!bd->alive) continue;
          float sx = cx + (bd->pos.x - player_.pos.x) * sc;
          float sy = cy + (bd->pos.y - player_.pos.y) * sc;
          const float fdeg = std::atan2(player_.pos.y - bd->pos.y, player_.pos.x - bd->pos.x) * 57.2957795f;
          if (platform_->has_sprite("bandit_skin-sheet0"))
            draw_c2_char(platform_, sx, sy, 40.f, "bandit_skin", fdeg, banim);
          else
            platform_->draw_rect(sx - 5, sy - 5, 10, 10, 0.25f, 0.4f, 0.75f, 1.f);
          platform_->draw_rect(sx - 5, sy - 24, 10.f * (bd->hp / bd->max_hp), 2, 0.9f, 0.2f, 0.15f, 1.f);
        }
      }

      // Vehicles (UAZ / boat sprites)
      for (int i = 0; i < vehicles_.count(); ++i) {
        const Vehicle* v = vehicles_.at(i);
        if (!v->active) continue;
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

      // Campfires
      for (int i = 0; i < campfires_.count(); ++i) {
        const Campfire* f = campfires_.at(i);
        if (!f->active) continue;
        float sx = cx + (f->pos.x - player_.pos.x) * sc;
        float sy = cy + (f->pos.y - player_.pos.y) * sc;
        if (platform_->has_sprite("fire_place_warm-sheet0"))
          platform_->draw_sprite("fire_place_warm-sheet0", sx - 22, sy - 22, 44, 44, 0.f, 1.f);
        else {
          platform_->draw_rect(sx - 6, sy - 6, 12, 12, 0.9f, 0.35f, 0.05f, 0.9f);
          platform_->draw_rect(sx - 3, sy - 10, 6, 6, 1.f, 0.7f, 0.15f, 0.7f);
        }
      }

      // Placed traps / barbed wire
      for (int i = 0; i < trap_count_; ++i) {
        if (!traps_[i].active) continue;
        float sx = cx + (traps_[i].pos.x - player_.pos.x) * sc;
        float sy = cy + (traps_[i].pos.y - player_.pos.y) * sc;
        if (traps_[i].kind == 0) {
          const float g = traps_[i].sprung ? 0.35f : 0.55f;
          platform_->draw_rect(sx - 5, sy - 5, 10, 10, 0.45f, g, 0.25f, 0.95f);
        } else if (traps_[i].kind == 2) {
          // Wood fence post
          platform_->draw_rect(sx - 3, sy - 10, 6, 20, 0.45f, 0.32f, 0.18f, 0.95f);
          platform_->draw_rect(sx - 10, sy - 4, 20, 3, 0.5f, 0.36f, 0.2f, 0.9f);
        } else if (traps_[i].kind == 3) {
          platform_->draw_rect(sx - 4, sy - 3, 8, 6, 0.35f, 0.35f, 0.2f, 0.95f);
          platform_->draw_rect(sx - 2, sy - 5, 4, 3, 0.7f, 0.2f, 0.15f, 0.9f);
        } else if (traps_[i].kind == 4) {
          // Landmine disc
          platform_->draw_rect(sx - 6, sy - 6, 12, 12, 0.25f, 0.28f, 0.2f, 0.95f);
          platform_->draw_rect(sx - 2, sy - 2, 4, 4, 0.85f, 0.25f, 0.1f, 0.9f);
        } else {
          platform_->draw_rect(sx - 14, sy - 14, 28, 28, 0.35f, 0.3f, 0.28f, 0.35f);
          platform_->draw_rect(sx - 8, sy - 1, 16, 2, 0.6f, 0.55f, 0.5f, 0.8f);
        }
      }
    }

    // Bullets
    {
      const float bsc = in_bunker_ ? 1.f : sc;
      for (const Bullet* b = bullets_.begin(); b != bullets_.end(); ++b) {
        if (!b->alive) continue;
        float sx = cx + (b->pos.x - player_.pos.x) * bsc;
        float sy = cy + (b->pos.y - player_.pos.y) * bsc;
        platform_->draw_rect(sx - 1, sy - 1, 3, 3, 1.f, 0.95f, 0.4f, 1.f);
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
        // Green phosphor NVG: mild vignette + brighter mid
        platform_->draw_rect(0, 0, 480, 272, 0.02f, 0.18f, 0.05f, 0.18f);
        platform_->draw_rect(cx - 90, cy - 70, 180, 140, 0.08f, 0.35f, 0.12f, 0.12f);
        platform_->draw_text(8, 8, "NVG", 0.35f, 0.9f, 0.4f);
      } else {
        platform_->draw_rect(0, 0, 480, 272, 0.f, 0.f, 0.08f, 0.48f);
        for (int si = 0; si < 24; ++si) {
          const float sx = static_cast<float>((si * 97 + 13) % 480);
          const float sy = static_cast<float>((si * 53 + 7) % 120);
          platform_->draw_rect(sx, sy, 1, 1, 0.85f, 0.9f, 1.f, 0.55f);
        }
      }
    }
    if (!in_bunker_ && weather_.rain > 0.7f && weather_.rain < 1.2f) {
      // Rain streaks (cheap)
      for (int i = 0; i < 18; ++i) {
        const float rx = static_cast<float>((i * 37 + static_cast<int>(weather_.rain * 40.f)) % 480);
        const float ry = static_cast<float>((i * 53 + static_cast<int>(weather_.day_phase * 200.f)) % 272);
        platform_->draw_rect(rx, ry, 1, 6, 0.55f, 0.6f, 0.7f, 0.35f);
      }
    }
    if (player_.torch_on || headlights) {
      const float glow = headlights ? 40.f : 28.f;
      platform_->draw_rect(cx - glow, cy - glow, glow * 2.f, glow * 2.f,
                           0.55f, 0.4f, 0.1f, headlights ? 0.2f : 0.25f);
      platform_->draw_rect(cx - glow * 0.5f, cy - glow * 0.5f, glow, glow,
                           0.75f, 0.55f, 0.15f, headlights ? 0.28f : 0.35f);
      // Facing cone (torch beam / vehicle headlights)
      if (player_.torch_on || headlights) {
        float face = player_.facing_deg;
        if (headlights && vehicles_.driven()) face = vehicles_.driven()->facing_deg;
        const float rad = face * 0.01745329252f;
        const int steps = headlights ? 10 : 8;
        for (int s = 2; s <= steps; ++s) {
          const float d = (headlights ? 14.f : 10.f) * static_cast<float>(s);
          const float w = 4.f + static_cast<float>(s) * (headlights ? 3.f : 2.2f);
          const float bx = cx + std::cos(rad) * d;
          const float by = cy + std::sin(rad) * d;
          platform_->draw_rect(bx - w * 0.5f, by - w * 0.5f, w, w,
                               headlights ? 0.9f : 0.85f,
                               headlights ? 0.85f : 0.7f,
                               headlights ? 0.55f : 0.25f,
                               0.1f + 0.02f * s);
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

    // Player — one 30×30 C2 frame (facing=column, walk=row); hands layered
    {
      const int panim = player_.running ? (1 + static_cast<int>(state_timer_ * 10.f) % 3) : 0;
      const float psz = crouching_ ? 36.f : 44.f;
      if (platform_->has_sprite("player_skin_def_1-sheet0")) {
        draw_c2_char(platform_, cx, cy, psz, "player_skin_def_1", player_.facing_deg, panim);
        draw_c2_char(platform_, cx, cy, psz, "player_hands_white", player_.facing_deg, panim);
      } else {
        platform_->draw_rect(cx - 6, cy - 6, 12, 12, 0.25f, 0.75f, 0.35f, 1.f);
      }
      const float rad = player_.facing_deg * 0.01745329252f;
      const InvSlot& eq = inventory_.slot(equipped_slot_);
      const WeaponDef* w = eq.empty() ? 0 : find_weapon(eq.id);
      const float aim_len = (w && !w->stats.is_melee) ? 36.f : 10.f;
      for (int s = 1; s <= 4; ++s) {
        const float d = aim_len * (static_cast<float>(s) / 4.f);
        platform_->draw_rect(cx + std::cos(rad) * d - 1.f,
                             cy + std::sin(rad) * d - 1.f, 2, 2,
                             0.9f, 0.95f, 0.8f, 0.35f + 0.15f * s);
      }
    }

    draw_debug_hud(*platform_, *this);
    if (state_ == AppState_Paused) {
      platform_->draw_rect(140, 110, 200, 40, 0.f, 0.f, 0.f, 0.55f);
      platform_->draw_text(180, 122, "PAUSED", 1.f, 1.f, 0.55f);
    }
  } else if (state_ == AppState_Dead) {
    platform_->clear(0.12f, 0.02f, 0.02f);
    platform_->draw_text(160, 100, "YOU DIED", 1.f, 0.3f, 0.25f);
    if (player_.survival.death_reason[0]) {
      platform_->draw_text(140, 130, player_.survival.death_reason, 0.85f, 0.5f, 0.45f);
    }
    char buf[96];
    std::snprintf(buf, sizeof(buf), "DAY %d  ZEDS %d  BOTS %d  ANIMALS %d",
                  player_.days_survived, player_.zeds_killed, player_.bandits_killed,
                  player_.animals_killed);
    platform_->draw_text(80, 160, buf, 0.8f, 0.6f, 0.55f);
    platform_->draw_text(120, 200, "ENTER - MENU", 0.75f, 0.55f, 0.5f);
  }

  platform_->end_frame();
}

} // namespace mdaz
