#include "mdaz/game.hpp"
#include "mdaz/combat.hpp"
#include "mdaz/hud.hpp"
#include "mdaz/save_game.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace mdaz {

Game::Game()
  : platform_(0), state_(AppState_Loading), quit_(false), state_timer_(0.f) {
  data_dir_[0] = 0;
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
    // Fallback: minimal baked items so tests can run without cwd issues
    const char* fallback =
      "{\"items\":[{\"id\":1,\"name\":\"Canned beans\"},{\"id\":18,\"name\":\"Bandage\"},"
      "{\"id\":8,\"name\":\"Pipsi\"},{\"id\":151,\"name\":\"FNX pistol\"}]}";
    items_.load_from_json_string(fallback);
  }

  state_ = AppState_Menu;
  state_timer_ = 0.f;
  return true;
}

void Game::shutdown() {}

void Game::start_new_game(Difficulty d) {
  player_.reset_new_game(d);
  inventory_.clear();
  // Starter items (IDs from l_eng_items.xml) — matches typical new-game feel; exact
  // starter loadout still being extracted from "Start events".
  inventory_.add_item(1, 1, items_);   // Canned beans
  inventory_.add_item(18, 2, items_);  // Bandage
  inventory_.add_item(54, 1, items_);  // Water bottle
  world_.reset();
  weather_.reset();
  zombies_.clear();
  animals_.clear();
  loot_.clear();
  timers_.reset();
  rng_.seed(140u);
  mapgen_.generate_new_game(140);

  // Spawn a few nearby threats for playable loop
  zombies_.spawn(Zed_Normal, Vec2(player_.pos.x + 180.f, player_.pos.y));
  zombies_.spawn(Zed_Fast, Vec2(player_.pos.x - 220.f, player_.pos.y + 40.f));
  zombies_.spawn(Zed_Buried, Vec2(player_.pos.x + 90.f, player_.pos.y + 160.f));
  animals_.spawn(Animal_Wolf, Vec2(player_.pos.x + 300.f, player_.pos.y - 100.f));
  animals_.spawn(Animal_Deer, Vec2(player_.pos.x - 400.f, player_.pos.y));
  loot_.spawn_random(Vec2(player_.pos.x + 60.f, player_.pos.y - 40.f), rng_);

  state_ = AppState_Playing;
}

void Game::update_menu(float dt, const InputState& in) {
  (void)dt;
  if (in.confirm || in.attack) {
    start_new_game(Difficulty_Novice);
  }
  if (in.cancel) quit_ = true;
}

void Game::update_playing(float dt, const InputState& in) {
  if (in.pause) {
    state_ = AppState_Paused;
    return;
  }

  player_.apply_input(in.move_x, in.move_y, in.run, dt);

  // Block movement into solid cells (simple resolve)
  if (world_.is_blocked(player_.pos.x, player_.pos.y)) {
    player_.pos.x -= player_.vel.x * dt;
    player_.pos.y -= player_.vel.y * dt;
  }

  weather_.update(dt);
  player_.update_survival(dt, weather_.ambient_temp, weather_.is_night, weather_.rain > 0.7f);
  timers_.update(dt);

  if (timers_.consume_zed_respawn() && zombies_.alive_count() < 6) {
    const float ang = rng_.next_float() * 6.28318f;
    const float dist = 200.f + rng_.next_float() * 150.f;
    const ZedType zt = static_cast<ZedType>(rng_.next_int(0, Zed_Fast));
    zombies_.spawn(zt, Vec2(player_.pos.x + std::cos(ang) * dist,
                            player_.pos.y + std::sin(ang) * dist));
  }
  if (timers_.consume_loot_respawn()) {
    const float ang = rng_.next_float() * 6.28318f;
    const float dist = 120.f + rng_.next_float() * 200.f;
    loot_.spawn_random(Vec2(player_.pos.x + std::cos(ang) * dist,
                            player_.pos.y + std::sin(ang) * dist), rng_);
  }
  if (timers_.consume_airdrop()) {
    loot_.spawn_airdrop(Vec2(player_.pos.x + rng_.range(-400.f, 400.f),
                             player_.pos.y + rng_.range(-400.f, 400.f)), rng_);
  }

  float zed_dmg = 0.f;
  zombies_.update(dt, player_.pos, zed_dmg);
  float animal_dmg = animals_.update(dt, player_.pos);
  const float total = zed_dmg + animal_dmg;
  if (total > 0.f) {
    player_.survival.blood -= total;
    if (zed_dmg > 0.f) player_.survival.bleeding += 1.5f;
    if (player_.survival.blood < 0.f) player_.survival.blood = 0.f;
  }

  if (in.attack) {
    // Melee all zombies in range
    WeaponStats melee = default_melee_stats();
    for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
      if (!z->alive) continue;
      const float dx = z->pos.x - player_.pos.x;
      const float dy = z->pos.y - player_.pos.y;
      if (dx * dx + dy * dy <= melee.range * melee.range) {
        z->hp -= melee.damage;
        if (z->hp <= 0.f) {
          z->alive = false;
          player_.zeds_killed += 1;
        }
      }
    }
  }

  if (in.inventory) {
    // Use first consumable slot
    for (int i = 0; i < inventory_.slot_count(); ++i) {
      if (inventory_.use_consumable(i, player_.survival, items_)) break;
    }
  }

  if (player_.survival.is_dead()) {
    state_ = AppState_Dead;
    state_timer_ = 0.f;
  }
}

void Game::update_dead(float dt, const InputState& in) {
  state_timer_ += dt;
  if (in.confirm || (state_timer_ > 2.f && in.attack)) {
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
    if (in.pause || in.confirm) state_ = AppState_Playing;
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

  if (state_ == AppState_Menu) {
    platform_->clear(0.08f, 0.09f, 0.07f);
    platform_->draw_text(40, 40, "MINI DAYZ C++ (1.4.1 baseline)", 0.9f, 0.9f, 0.8f);
    platform_->draw_text(40, 70, "Enter/Z: New game (Novice)   Esc: Quit", 0.7f, 0.8f, 0.7f);
  } else if (state_ == AppState_Playing || state_ == AppState_Paused) {
    platform_->clear(0.12f, 0.16f, 0.10f);
    // Player at screen center
    const float cx = 240.f;
    const float cy = 136.f;
    platform_->draw_rect(cx - 6, cy - 6, 12, 12, 0.2f, 0.7f, 0.3f, 1.f);

    for (const Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
      if (!z->alive) continue;
      float sx = cx + (z->pos.x - player_.pos.x) * 0.15f;
      float sy = cy + (z->pos.y - player_.pos.y) * 0.15f;
      platform_->draw_rect(sx - 5, sy - 5, 10, 10, 0.7f, 0.2f, 0.15f, 1.f);
    }
    draw_debug_hud(*platform_, *this);
    if (state_ == AppState_Paused) {
      platform_->draw_text(180, 120, "PAUSED", 1.f, 1.f, 0.5f);
    }
  } else if (state_ == AppState_Dead) {
    platform_->clear(0.15f, 0.02f, 0.02f);
    platform_->draw_text(160, 120, "YOU DIED", 1.f, 0.3f, 0.3f);
    platform_->draw_text(100, 150, "Enter: Menu", 0.8f, 0.6f, 0.6f);
  }

  platform_->end_frame();
}

} // namespace mdaz
