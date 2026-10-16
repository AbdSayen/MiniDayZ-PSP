#include "mdaz/game.hpp"
#include "mdaz/combat.hpp"
#include "mdaz/craft.hpp"
#include "mdaz/hud.hpp"
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

float world_to_screen_scale() { return 0.35f; }

} // namespace

Game::Game()
  : platform_(0), audio_(0), state_(AppState_Loading), quit_(false),
    state_timer_(0.f), attack_cd_(0.f), reload_cd_(0.f), airdrop_alert_(0.f),
    torch_battery_(100.f), inv_full_flash_(0.f), tent_warmth_t_(0.f),
    equipped_slot_(0), in_bunker_(false), bunker_looted_(false),
    show_bag_(false), menu_difficulty_(Difficulty_Novice) {
  data_dir_[0] = 0;
  last_airdrop_pos_ = Vec2();
  for (int i = 0; i < kMapSectorsX * kMapSectorsY; ++i) visited_sectors_[i] = false;
  prev_scream_ = false;
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
  equipped_slot_ = 3; // fire axe slot if layout matches add order
  // Find axe slot
  for (int i = 0; i < inventory_.slot_count(); ++i) {
    if (inventory_.slot(i).id == 204) { equipped_slot_ = i; break; }
  }

  world_.reset();
  weather_.reset();
  zombies_.clear();
  animals_.clear();
  bandits_.clear();
  vehicles_.clear();
  campfires_.clear();
  loot_.clear();
  bullets_.clear();
  quest_.reset();
  quest_.start(Quest_KillZeds10);
  timers_.reset();
  dispersion_.reset_from_globals();
  attack_cd_ = 0.f;
  airdrop_alert_ = 0.f;
  torch_battery_ = 100.f;
  inv_full_flash_ = 0.f;
  tent_warmth_t_ = 0.f;
  for (int i = 0; i < kMapSectorsX * kMapSectorsY; ++i) visited_sectors_[i] = false;
  rng_.seed(140u);
  mapgen_.generate_new_game(140);
  mapgen_.apply_collision(world_);
  vehicles_.spawn(Vehicle_Car, Vec2(player_.pos.x + 90.f, player_.pos.y - 50.f));
  vehicles_.spawn(Vehicle_Boat, Vec2(player_.pos.x - 120.f, player_.pos.y + 160.f));
  vehicles_.spawn(Vehicle_Bike, Vec2(player_.pos.x + 40.f, player_.pos.y + 70.f));
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
  inventory_.add_item(38, 3, items_); // wood sticks
  inventory_.add_item(37, 1, items_); // ashwood
  inventory_.add_item(41, 1, items_); // rope
  inventory_.add_item(34, 1, items_); // heatpack
  inventory_.add_item(46, 1, items_); // tetracycline
  inventory_.add_item(354, 1, items_); // Jacket — equip with C for insulation
  inventory_.add_item(300, 1, items_); // Jeans
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
    else if (L.type == Loc_Hospital || L.type == Loc_City) piles = 2;
    for (int n = 0; n < piles; ++n) {
      const float ox = rng_.range(-40.f, 40.f);
      const float oy = rng_.range(-40.f, 40.f);
      const Vec2 p(L.pos.x + ox, L.pos.y + oy);
      switch (L.type) {
      case Loc_Military:
      case Loc_Helicrash:
        loot_.spawn_airdrop(p, rng_);
        break;
      case Loc_Hospital:
        loot_.spawn_hospital(p, rng_);
        break;
      case Loc_GasStation:
        loot_.spawn_gas_station(p, rng_);
        break;
      case Loc_City:
      case Loc_Village:
      case Loc_Firestation:
        loot_.spawn_civilian(p, rng_);
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
  // Infection chance stub (full C2 disease group not ported); gasmask helps
  if (can_bleed && !player_.survival.infected) {
    bool gasmask = false;
    for (int wi = 0; wi < inventory_.worn_count(); ++wi) {
      if (inventory_.worn_at(wi) == 250) { gasmask = true; break; }
    }
    const float chance = gasmask ? 0.02f : 0.08f;
    if (rng_.next_float() < chance) player_.survival.infected = true;
  }
  if (audio_ && amount > 0.f) audio_->play("hit", 0.55f);
}

void Game::try_melee_attack() {
  WeaponStats stats = default_melee_stats();
  const InvSlot& eq = inventory_.slot(equipped_slot_);
  if (!eq.empty()) {
    const WeaponDef* w = find_weapon(eq.id);
    if (w && w->stats.is_melee) stats = w->stats;
  }
  attack_cd_ = stats.fire_interval;
  if (audio_) audio_->play_at("melee_swing", player_.pos.x, player_.pos.y, 0.6f);

  for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
    if (!z->alive) continue;
    if (dist2(z->pos, player_.pos) <= stats.range * stats.range) {
      z->hp -= stats.damage;
      if (z->hp <= 0.f) {
        z->alive = false;
        player_.zeds_killed += 1;
        // Drop loot on kill
        loot_.spawn_random(z->pos, rng_);
      }
    }
  }
  for (int i = 0; i < animals_.count(); ++i) {
    Animal* a = animals_.at(i);
    if (!a->alive) continue;
    if (dist2(a->pos, player_.pos) <= stats.range * stats.range) {
      a->hp -= stats.damage;
      if (a->hp <= 0.f) on_animal_killed(*a);
    }
  }
  for (Bandit* b = bandits_.begin(); b != bandits_.end(); ++b) {
    if (!b->alive) continue;
    if (dist2(b->pos, player_.pos) <= stats.range * stats.range) {
      b->hp -= stats.damage;
      if (b->hp <= 0.f) {
        b->alive = false;
        player_.bandits_killed += 1;
        loot_.spawn_airdrop(b->pos, rng_);
      }
    }
  }
}

void Game::try_ranged_attack() {
  if (reload_cd_ > 0.f) return;
  const InvSlot& eq = inventory_.slot(equipped_slot_);
  if (eq.empty()) return;
  const WeaponDef* w = find_weapon(eq.id);
  if (!w || w->stats.is_melee) return;

  // Consume one ammo if ammo_id set
  if (w->ammo_id > 0) {
    bool spent = false;
    for (int i = 0; i < inventory_.slot_count(); ++i) {
      if (inventory_.slot(i).id == w->ammo_id && inventory_.slot(i).count > 0) {
        inventory_.remove_at(i, 1);
        spent = true;
        break;
      }
    }
    if (!spent) {
      if (audio_) audio_->play("melee_swing", 0.25f); // dry click
      attack_cd_ = 0.2f;
      return;
    }
  }

  attack_cd_ = w->stats.fire_interval;
  if (audio_) audio_->play_at("gunshot", player_.pos.x, player_.pos.y, 0.8f);
  dispersion_.on_shot(player_.running);
  const float ang = player_.facing_deg + dispersion_.sample_offset_deg(rng_);
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
  ItemId meat = 60;
  if (a.type == Animal_Rabbit || a.type == Animal_Crow) meat = 71;
  const int n = player_.perk_hunter ? 2 : 1;
  loot_.spawn_item(a.pos, meat, n);
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
    if (z->hp <= 0.f) {
      z->alive = false;
      player_.zeds_killed += 1;
      loot_.spawn_random(z->pos, rng_);
    }
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
      loot_.spawn_airdrop(bd->pos, rng_);
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
        if (z->hp <= 0.f) {
          z->alive = false;
          player_.zeds_killed += 1;
          loot_.spawn_random(z->pos, rng_);
        }
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
          loot_.spawn_airdrop(bd->pos, rng_);
        }
        break;
      }
    }
  }
}

void Game::try_craft() {
  const bool at_fire = campfires_.near_fire(player_.pos);
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
  const InvSlot& eq = inventory_.slot(equipped_slot_);
  if (eq.empty()) return;
  const WeaponDef* w = find_weapon(eq.id);
  if (!w || w->stats.is_melee || w->ammo_id <= 0) return;
  // Magazines not modeled yet — Tap_reload stub: block fire briefly if reserve ammo exists
  bool has_ammo = false;
  for (int i = 0; i < inventory_.slot_count(); ++i) {
    if (inventory_.slot(i).id == w->ammo_id && inventory_.slot(i).count > 0) {
      has_ammo = true;
      break;
    }
  }
  if (!has_ammo) {
    if (audio_) audio_->play("melee_swing", 0.2f);
    return;
  }
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
  // Consume one Campfire kit (id 23)
  for (int i = 0; i < inventory_.slot_count(); ++i) {
    if (inventory_.slot(i).id != 23 || inventory_.slot(i).count <= 0) continue;
    if (!campfires_.place(player_.pos, 240.f)) return;
    inventory_.remove_at(i, 1);
    if (audio_) audio_->play("craft", 0.5f);
    return;
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
    if (d2 > 380.f * 380.f || d2 < 40.f * 40.f) continue;
    const float ang = std::atan2(player_.pos.y - z->pos.y, player_.pos.x - z->pos.x) * 57.2957795f;
    const float dmg = roll_bullet_damage(18.f, rng_);
    bullets_.spawn(z->pos, ang, 480.f, dmg, 0.9f, false);
    z->attack_cooldown = 1.4f;
  }
}

void Game::update_bandit_guns(float dt) {
  (void)dt;
  for (Bandit* b = bandits_.begin(); b != bandits_.end(); ++b) {
    if (!b->alive || !b->has_gun) continue;
    if (b->attack_cooldown > 0.f) continue;
    const float d2 = dist2(b->pos, player_.pos);
    if (d2 > 360.f * 360.f || d2 < 50.f * 50.f) continue;
    const float ang = std::atan2(player_.pos.y - b->pos.y, player_.pos.x - b->pos.x) * 57.2957795f;
    const float dmg = roll_bullet_damage(20.f, rng_);
    bullets_.spawn(b->pos, ang, 500.f, dmg, 0.85f, false);
    b->attack_cooldown = 1.2f;
  }
}

void Game::update_menu(float dt, const InputState& in) {
  (void)dt;
  if (in.slot_digit == 1) menu_difficulty_ = Difficulty_Novice;
  if (in.slot_digit == 2) menu_difficulty_ = Difficulty_Regular;
  if (in.slot_digit == 3) menu_difficulty_ = Difficulty_Veteran;
  if (in.load_pressed) {
    if (SaveSystem::load_file(*platform_, "save.mdaz", player_, inventory_, weather_,
                              &equipped_slot_, visited_sectors_,
                              kMapSectorsX * kMapSectorsY)) {
      in_bunker_ = false;
      bullets_.clear();
      zombies_.clear();
      animals_.clear();
      bandits_.clear();
      campfires_.clear();
      loot_.clear();
      mark_sector_visited();
      // Respawn a few threats near loaded position
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
  }
  if (in.equip_delta != 0) {
    equipped_slot_ = (equipped_slot_ + in.equip_delta) % 8;
    if (equipped_slot_ < 0) equipped_slot_ += 8;
  }

  const bool was_driving = vehicles_.player_driving();
  const bool drove = (!in_bunker_) &&
    vehicles_.update(dt, player_.pos, in.move_x, in.move_y, in.interact_pressed);
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
          bool gasmask = false;
          for (int wi = 0; wi < inventory_.worn_count(); ++wi) {
            if (inventory_.worn_at(wi) == 250) { gasmask = true; break; }
          }
          if (!gasmask && !player_.survival.infected && rng_.next_float() < 0.002f) {
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
    }
    player_.apply_input(mx, my, in.run, dt);
    if (!in_bunker_ && world_.is_blocked(player_.pos.x, player_.pos.y)) {
      player_.pos.x -= player_.vel.x * dt;
      player_.pos.y -= player_.vel.y * dt;
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
  }
  const bool vehicle_interact = was_driving || vehicles_.player_driving();

  if (!in_bunker_) {
    const float prev_phase = weather_.day_phase;
    weather_.update(dt);
    // Day rolled over
    if (weather_.day_phase < prev_phase) {
      player_.days_survived += 1;
    }
  }
  {
    // Clothing insulation stub: count clothing items in bag (full equip slots TBD)
    float insulation = in_bunker_ ? 8.f : 0.f;
    if (player_.torch_on) insulation += 3.f;
    if (tent_warmth_t_ > 0.f && campfires_.near_fire(player_.pos, 70.f)) insulation += 5.f;
    insulation += inventory_.clothing_insulation();
    insulation += campfires_.warmth_near(player_.pos) * 6.f; // fire warmth
    if (insulation > 16.f) insulation = 16.f;
    campfires_.update(dt);
    player_.update_survival(dt, weather_.ambient_temp, weather_.is_night && !in_bunker_,
                            !in_bunker_ && weather_.rain > 0.7f, insulation);
  }
  timers_.update(dt);
  dispersion_.update(dt);
  if (attack_cd_ > 0.f) attack_cd_ -= dt;
  if (reload_cd_ > 0.f) reload_cd_ -= dt;
  if (inv_full_flash_ > 0.f) inv_full_flash_ -= dt;
  if (tent_warmth_t_ > 0.f) tent_warmth_t_ -= dt;
  {
    float drain = 0.f;
    if (player_.torch_on) drain += 1.5f;
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
    if (timers_.consume_zed_respawn() && zombies_.alive_count() < 8) {
      const float ang = rng_.next_float() * 6.28318f;
      const float dist = 220.f + rng_.next_float() * 180.f;
      const ZedType zt = static_cast<ZedType>(rng_.next_int(0, Zed_ArmyShooter));
      zombies_.spawn(zt, Vec2(player_.pos.x + std::cos(ang) * dist,
                              player_.pos.y + std::sin(ang) * dist));
    }
    if (timers_.consume_loot_respawn()) {
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
      if (audio_) audio_->play("bunker", 0.7f);
    }
    if (airdrop_alert_ > 0.f) airdrop_alert_ -= dt;

    float animal_dmg = animals_.update(dt, player_.pos);
    float bandit_dmg = bandits_.update(dt, player_.pos);
    apply_incoming_damage(animal_dmg + bandit_dmg,
                          animal_dmg > 0.f || bandit_dmg > 0.f);
    update_army_shooters(dt);
    update_bandit_guns(dt);

    if (timers_.consume_bot_respawn() && bandits_.alive_count() < 4) {
      const float ang = rng_.next_float() * 6.28318f;
      const float dist = 280.f + rng_.next_float() * 200.f;
      bandits_.spawn(Vec2(player_.pos.x + std::cos(ang) * dist,
                          player_.pos.y + std::sin(ang) * dist),
                     rng_.next_float() < 0.5f);
    }
    if (timers_.consume_animal_respawn() && animals_.alive_count() < 10) {
      const float ang = rng_.next_float() * 6.28318f;
      const float dist = 260.f + rng_.next_float() * 220.f;
      const Vec2 base(player_.pos.x + std::cos(ang) * dist,
                      player_.pos.y + std::sin(ang) * dist);
      const float roll = rng_.next_float();
      if (roll < 0.45f && animals_.alive_of_type(Animal_Wolf) < 5) {
        // Wolf pack (2–3)
        const int pack = 2 + rng_.next_int(0, 1);
        for (int i = 0; i < pack; ++i) {
          animals_.spawn(Animal_Wolf, Vec2(base.x + rng_.range(-40.f, 40.f),
                                           base.y + rng_.range(-40.f, 40.f)));
        }
      } else if (roll < 0.7f) {
        animals_.spawn(Animal_Deer, base);
      } else if (roll < 0.82f) {
        animals_.spawn(Animal_Rabbit, base);
      } else if (roll < 0.93f) {
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
    zombies_.update(dt, player_.pos, zed_dmg);
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
      if (has_rod && rng_.next_float() < fish_chance) {
        inventory_.add_item(94, 1, items_); // Perch
        if (audio_) audio_->play("pickup", 0.4f);
      } else if (!has_rod && rng_.next_float() < bush_chance) {
        // Bush forage: sticks / berries / rags
        static const ItemId kBush[] = { 38, 27, 26, 33 };
        ItemId id = kBush[rng_.next_int(0, 3)];
        inventory_.add_item(id, 1 + (player_.perk_farmer ? 1 : 0), items_);
        if (audio_) audio_->play("pickup", 0.35f);
      }
    }
    (void)loot_before;
  }

  {
    bool near_bunker = false;
    for (int i = 0; i < mapgen_.location_count(); ++i) {
      const MapLocation& L = mapgen_.location(i);
      if (L.type == Loc_BunkerEntrance && dist2(L.pos, player_.pos) < 80.f * 80.f) {
        near_bunker = true;
        break;
      }
    }
    if (quest_.tick(player_.days_survived, player_.zeds_killed, near_bunker || in_bunker_)) {
      if (audio_) audio_->play("craft", 0.9f);
      // Chain next objective
      if (quest_.completed[Quest_KillZeds10] && !quest_.completed[Quest_FindBunker]) {
        quest_.start(Quest_FindBunker);
      } else if (quest_.completed[Quest_FindBunker] && !quest_.completed[Quest_SurviveDay1]) {
        quest_.start(Quest_SurviveDay1);
      }
    }
  }

  if (in.reload_pressed) try_craft();
  if (in.drop_pressed) try_drop_equipped();
  if (in.mag_reload_pressed) try_mag_reload();
  if (in.throw_pressed) try_throw_projectile();
  if (in.torch_pressed) {
    // T: place campfire if kit available, else toggle torch (needs battery)
    bool had_kit = false;
    for (int i = 0; i < inventory_.slot_count(); ++i) {
      if (inventory_.slot(i).id == 23 && inventory_.slot(i).count > 0) { had_kit = true; break; }
    }
    if (had_kit) try_place_campfire();
    else if (player_.torch_on) {
      player_.torch_on = false;
    } else if (torch_battery_ > 1.f) {
      player_.torch_on = true;
    } else if (audio_) {
      audio_->play("melee_swing", 0.2f);
    }
  }
  show_bag_ = in.bag;

  if (in.save_pressed) {
    SaveSystem::save_file(*platform_, "save.mdaz", player_, inventory_, weather_,
                          equipped_slot_, visited_sectors_,
                          kMapSectorsX * kMapSectorsY);
  }
  if (in.load_pressed) {
    if (SaveSystem::load_file(*platform_, "save.mdaz", player_, inventory_, weather_,
                              &equipped_slot_, visited_sectors_,
                              kMapSectorsX * kMapSectorsY)) {
      in_bunker_ = false;
      bullets_.clear();
      campfires_.clear();
      mark_sector_visited();
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
        } else if (eq.id == 20) { // Flare — short light/warmth
          if (inventory_.remove_at(equipped_slot_, 1)) {
            campfires_.place(player_.pos, 45.f);
            used_special = true;
            if (audio_) audio_->play("craft", 0.6f);
          }
        } else if (eq.id == 22) { // Battery — refill torch/NVG
          if (inventory_.remove_at(equipped_slot_, 1)) {
            torch_battery_ += 50.f;
            if (torch_battery_ > 100.f) torch_battery_ = 100.f;
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
        }
      }
      if (!used_special) {
        if (!inventory_.use_consumable(equipped_slot_, player_.survival, items_, &rng_)) {
          for (int i = 0; i < inventory_.slot_count(); ++i) {
            if (inventory_.use_consumable(i, player_.survival, items_, &rng_)) break;
          }
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
    platform_->draw_rect(0, 0, 480, 272, 0.08f, 0.1f, 0.07f, 1.f);
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
      // Ground grid relative to player
      for (int gx = -8; gx <= 8; ++gx) {
        for (int gy = -5; gy <= 5; ++gy) {
          float wx = player_.pos.x + gx * 64.f;
          float wy = player_.pos.y + gy * 64.f;
          float sx = cx + (wx - player_.pos.x) * sc;
          float sy = cy + (wy - player_.pos.y) * sc;
          const bool alt = ((gx + gy) & 1) != 0;
          platform_->draw_rect(sx - 30, sy - 30, 60, 60,
                               alt ? 0.16f : 0.13f, alt ? 0.20f : 0.17f, alt ? 0.12f : 0.10f, 1.f);
        }
      }

      // Map POIs
      for (int i = 0; i < mapgen_.location_count(); ++i) {
        const MapLocation& L = mapgen_.location(i);
        float sx = cx + (L.pos.x - player_.pos.x) * sc;
        float sy = cy + (L.pos.y - player_.pos.y) * sc;
        float r = 0.5f, g = 0.5f, b = 0.35f;
        if (L.type == Loc_City) { r = 0.55f; g = 0.45f; b = 0.25f; }
        if (L.type == Loc_Military) { r = 0.35f; g = 0.45f; b = 0.3f; }
        if (L.type == Loc_Hospital) { r = 0.7f; g = 0.7f; b = 0.75f; }
        if (L.type == Loc_BunkerEntrance) { r = 0.3f; g = 0.25f; b = 0.2f; }
        platform_->draw_rect(sx - 4, sy - 4, 8, 8, r, g, b, 0.9f);
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
          platform_->draw_rect(sx - 6, sy - 6, 12, 12, 0.95f, 0.45f, 0.1f, 1.f);
          platform_->draw_rect(sx - 2, sy - 10, 4, 4, 0.9f, 0.9f, 0.9f, 0.8f);
        } else {
          platform_->draw_rect(sx - 3, sy - 3, 6, 6, 0.9f, 0.85f, 0.2f, 1.f);
        }
      }
    }

    // Zombies (also in bunker)
    {
      const float zsc = in_bunker_ ? 1.f : sc;
      for (const Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
        if (!z->alive) continue;
        float sx = cx + (z->pos.x - player_.pos.x) * zsc;
        float sy = cy + (z->pos.y - player_.pos.y) * zsc;
        float r = 0.7f, g = 0.2f, b = 0.15f;
        if (z->type == Zed_Fast) { r = 0.85f; g = 0.35f; b = 0.1f; }
        if (z->type == Zed_Army || z->type == Zed_ArmyShooter) { r = 0.35f; g = 0.45f; b = 0.25f; }
        if (z->type == Zed_Buried) { r = 0.4f; g = 0.35f; b = 0.2f; }
        if (z->type == Zed_Tank) { r = 0.55f; g = 0.15f; b = 0.1f; }
        platform_->draw_rect(sx - 5, sy - 5, 10, 10, r, g, b, 1.f);
        platform_->draw_rect(sx - 5, sy - 8, 10.f * (z->hp / z->max_hp), 2, 0.9f, 0.2f, 0.15f, 1.f);
      }
    }

    if (!in_bunker_) {
      // Airdrop compass while alert active
      if (airdrop_alert_ > 0.f) {
        float sx = cx + (last_airdrop_pos_.x - player_.pos.x) * sc;
        float sy = cy + (last_airdrop_pos_.y - player_.pos.y) * sc;
        platform_->draw_rect(sx - 8, sy - 8, 16, 16, 1.f, 0.8f, 0.1f, 0.35f);
      }

      // Animals
      for (int i = 0; i < animals_.count(); ++i) {
        const Animal* a = animals_.at(i);
        if (!a->alive) continue;
        float sx = cx + (a->pos.x - player_.pos.x) * sc;
        float sy = cy + (a->pos.y - player_.pos.y) * sc;
        if (a->type == Animal_Wolf || a->type == Animal_Dog)
          platform_->draw_rect(sx - 5, sy - 4, 10, 8, 0.45f, 0.4f, 0.35f, 1.f);
        else if (a->type == Animal_Crow)
          platform_->draw_rect(sx - 2, sy - 2, 4, 4, 0.15f, 0.15f, 0.18f, 1.f);
        else
          platform_->draw_rect(sx - 4, sy - 4, 8, 8, 0.55f, 0.5f, 0.3f, 1.f);
      }

      // Bandits (blue-ish)
      for (const Bandit* bd = bandits_.begin(); bd != bandits_.end(); ++bd) {
        if (!bd->alive) continue;
        float sx = cx + (bd->pos.x - player_.pos.x) * sc;
        float sy = cy + (bd->pos.y - player_.pos.y) * sc;
        platform_->draw_rect(sx - 5, sy - 5, 10, 10, 0.25f, 0.4f, 0.75f, 1.f);
        platform_->draw_rect(sx - 5, sy - 8, 10.f * (bd->hp / bd->max_hp), 2, 0.9f, 0.2f, 0.15f, 1.f);
      }

      // Vehicles
      for (int i = 0; i < vehicles_.count(); ++i) {
        const Vehicle* v = vehicles_.at(i);
        if (!v->active) continue;
        float sx = cx + (v->pos.x - player_.pos.x) * sc;
        float sy = cy + (v->pos.y - player_.pos.y) * sc;
        if (v->type == Vehicle_Boat) {
          platform_->draw_rect(sx - 10, sy - 4, 20, 8, 0.25f, 0.45f, 0.55f, 1.f);
        } else if (v->type == Vehicle_Bike) {
          platform_->draw_rect(sx - 6, sy - 3, 12, 6, 0.55f, 0.35f, 0.2f, 1.f);
        } else {
          platform_->draw_rect(sx - 8, sy - 5, 16, 10, 0.4f, 0.4f, 0.45f, 1.f);
        }
      }

      // Campfires
      for (int i = 0; i < campfires_.count(); ++i) {
        const Campfire* f = campfires_.at(i);
        if (!f->active) continue;
        float sx = cx + (f->pos.x - player_.pos.x) * sc;
        float sy = cy + (f->pos.y - player_.pos.y) * sc;
        platform_->draw_rect(sx - 6, sy - 6, 12, 12, 0.9f, 0.35f, 0.05f, 0.9f);
        platform_->draw_rect(sx - 3, sy - 10, 6, 6, 1.f, 0.7f, 0.15f, 0.7f);
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

    // Night darkness vignette + torch/NVG
    if (!in_bunker_ && weather_.is_night && !player_.torch_on) {
      if (player_.nvg_on) {
        platform_->draw_rect(0, 0, 480, 272, 0.05f, 0.25f, 0.08f, 0.22f);
      } else {
        platform_->draw_rect(0, 0, 480, 272, 0.f, 0.f, 0.05f, 0.35f);
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
    if (player_.torch_on) {
      platform_->draw_rect(cx - 28, cy - 28, 56, 56, 0.55f, 0.4f, 0.1f, 0.25f);
      platform_->draw_rect(cx - 14, cy - 14, 28, 28, 0.75f, 0.55f, 0.15f, 0.35f);
    }

    // Player
    platform_->draw_rect(cx - 6, cy - 6, 12, 12, 0.25f, 0.75f, 0.35f, 1.f);
    // facing / aim line
    {
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
    char buf[80];
    std::snprintf(buf, sizeof(buf), "DAY %d  ZEDS %d  BOTS %d",
                  player_.days_survived, player_.zeds_killed, player_.bandits_killed);
    platform_->draw_text(120, 160, buf, 0.8f, 0.6f, 0.55f);
    platform_->draw_text(120, 200, "ENTER - MENU", 0.75f, 0.55f, 0.5f);
  }

  platform_->end_frame();
}

} // namespace mdaz
