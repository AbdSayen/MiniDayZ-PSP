// C2 Audio events (data.js t178 actions) driven from the game state. Audit with per-event sids:
// docs/AUDIO_AUDIT.md. Everything here is a 1:1 Audio action of the original; where the port
// lacks the source object/state the event is listed in KNOWN_ISSUES.md instead of approximated.
#include "mdaz/game.hpp"
#include "mdaz/c2_sfx.hpp"
#include "mdaz/building_solid.hpp"
#include "mdaz/balance_globals.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace mdaz {

namespace {
const int kC2CurrentLevel = static_cast<int>(balance::CurrentLevel);
const float kHearR = static_cast<float>(balance::Player_Hear_radius);   // 1000
const BuildingShape* loc_shape(const MapLocation& L) {   // same as game.cpp
  return L.shape >= 0 ? building_shape(L.shape) : 0;
}
float dd2(Vec2 a, Vec2 b) { const float x = a.x - b.x, y = a.y - b.y; return x * x + y * y; }
} // namespace


void Game::sfx2d(const char* name, float db, const char* tag) {
  if (audio_ && name) audio_->play(name, false, db, tag ? tag : "");
}

void Game::sfx_at(const char* name, float db, Vec2 at, const char* tag) {
  if (audio_ && name) audio_->play_at(name, false, db, at.x, at.y, tag ? tag : "");
}

void Game::sfx_play(const C2Sfx& s, Vec2 at) {
  if (!audio_ || !s.name) return;
  if (s.at_object) audio_->play_at(s.name, false, s.db, at.x, at.y, s.tag);
  else audio_->play(s.name, false, s.db, s.tag);
}

// System Wait inside an event: the port queues the follow-up action.
void Game::sfx_later(float delay, const char* name, float db, bool positioned, Vec2 at,
                     int follow, bool loop, const char* tag) {
  for (int i = 0; i < kMaxPendingSfx; ++i) {
    PendingSfx& p = pending_sfx_[i];
    if (p.name) continue;
    p.t = delay; p.name = name; p.db = db; p.positioned = positioned; p.pos = at;
    p.follow = follow; p.loop = loop;
    std::snprintf(p.tag, sizeof(p.tag), "%s", tag ? tag : "");
    return;
  }
}

// Game_events "Start events" OnLayoutStart (sid8520771380755414): StopAll; Sounds_on == 0 ->
// SetSilent + SetMuted day/night (sid2793916812797973). Day&Night TriggerOnce runs on the
// layout's first tick (runtime.layout_first_tick).
void Game::audio_layout_start() {
  for (int i = 0; i < kMaxPendingSfx; ++i) pending_sfx_[i].name = 0;
  if (!audio_) return;
  audio_->stop_all();
  audio_->set_silent(balance::Sounds_on == 0);
  snd_day_trig_ = false;
  snd_night_trig_ = false;
  snd_first_tick_ = true;
  snd_rain_on_ = weather_.raining;   // the loop only starts on the Rain timer (sid609331584746158)
  for (int i = 0; i < kMaxCampfires; ++i) snd_fire_on_[i] = false;
  snd_flare_on_ = false;
  snd_car_idx_ = -1;
  snd_step_t_ = 0.f;
  snd_scream_t_ = 0.f;
  snd_buried_t_ = 0.f;
  snd_roof_t_ = 0.f;
}

// Menu_Events Menu_init OnLayoutStart (sid9948355272197826): Play "ambient" looping, -10 dB.
// The game -> menu paths all StopAll first (save&exit sid4751709322939984, restart
// sid5542220247495624, Level_change sid8294409464439039, Next_level_menu sid179852424293007).
void Game::audio_menu_start() {
  for (int i = 0; i < kMaxPendingSfx; ++i) pending_sfx_[i].name = 0;
  if (!audio_) return;
  audio_->stop_all();
  audio_->play("ambient", true, -10.f, "");
}

// Bunker_generate (sid305470436850518 ...): Stop rain_loop, rain_roof, day_loop, night_loop;
// bunker_door (sid642549273016672) when the bunker door animation starts.
void Game::audio_bunker_enter() {
  if (!audio_) return;
  audio_->stop("rain_loop");
  audio_->stop("rain_roof");
  audio_->stop("day_loop");
  audio_->stop("night_loop");
  audio_->play("bunker_door", false, 0.f, "");
}

// Bunker_erase: TriggerOnce inside the function -> runs on every call.
void Game::audio_bunker_exit() {
  if (!audio_) return;
  const int h = weather_.timer_hours();
  if (h >= 0 && h <= 5) {        // sid427082255021896
    audio_->play(c2_night_loop_file(kC2CurrentLevel, true), true, 0.f, "Night_loop");
    audio_->stop("day_loop");
  } else if (h >= 6 && h <= 23) { // sid172106691563512
    audio_->play(c2_day_loop_file(kC2CurrentLevel, true), true, 0.f, "day_loop");
    audio_->stop("night_loop");
  }
}

bool Game::audio_player_in_warmzone(int* floor) const {
  for (int i = 0; i < mapgen_.location_count(); ++i) {
    const MapLocation& L = mapgen_.location(i);
    if (dd2(L.pos, player_.pos) > 900.f * 900.f) continue;
    const BuildingShape* bs = loc_shape(L);
    if (bs && bs->has_wz && building_in_warmzone(*bs, L.pos, player_.pos)) {
      if (floor) *floor = c2_building_floor(L.c2_sub);
      return true;
    }
  }
  return false;
}

// t928 = the building exterior (roof) object: its frame box around the hotspot.
bool Game::audio_player_in_house_area() const {
  for (int i = 0; i < mapgen_.location_count(); ++i) {
    const MapLocation& L = mapgen_.location(i);
    if (dd2(L.pos, player_.pos) > 900.f * 900.f) continue;
    const BuildingShape* bs = loc_shape(L);
    if (!bs) continue;
    const float x0 = L.pos.x - bs->ext_hx * bs->ext_w, y0 = L.pos.y - bs->ext_hy * bs->ext_h;
    if (player_.pos.x >= x0 && player_.pos.y >= y0 &&
        player_.pos.x < x0 + bs->ext_w && player_.pos.y < y0 + bs->ext_h) return true;
  }
  return false;
}

// Animations group: player_base EightDir IsMoving -> overlapping t945 (ground tilemaps) ->
// [sand t411 & not indoor] sand_run | [indoor] wood / concrete by warmzone IV1 |
// [not sand, not indoor] tile != 6 -> forest_run, else run_. Every 0.25 s, -5 dB, tag "run".
void Game::audio_footsteps(float dt, bool moving) {
  if (!audio_ || !moving) return;
  snd_step_t_ += dt;
  if (snd_step_t_ < 0.25f) return;
  snd_step_t_ -= 0.25f;
  if (snd_step_t_ > 0.25f) snd_step_t_ = 0.f;
  int ground = cell_tiles_.tile_at(0, player_.pos.x, player_.pos.y);
  if (ground < 0) ground = world_tiles_.tile_at("ground_lvl1_tilemap", player_.pos.x, player_.pos.y);
  if (ground < 0) return;   // not overlapping t945
  int floor = 0;
  const bool indoor = audio_player_in_warmzone(&floor);
  int surface = -1;
  if (indoor) surface = floor == 1 ? 3 : floor == 2 ? 4 : -1;
  // Sand tilemap t411: the port generates no sand tilemap (KNOWN_ISSUES), so never sand_run.
  else surface = ground != 6 ? 1 : 0;
  if (surface < 0) return;
  audio_->play_at(c2_footstep_sfx(surface, sfx_rng_), false, -5.f, player_.pos.x, player_.pos.y, "run");
}

void Game::update_audio(float dt) {
  if (!audio_) return;
  // Menu layout entry (any path).
  if (state_ == AppState_Menu && snd_prev_state_ != AppState_Menu) audio_menu_start();
  snd_prev_state_ = state_;
  if (state_ != AppState_Playing) return;

  // Pending Wait follow-ups.
  for (int i = 0; i < kMaxPendingSfx; ++i) {
    PendingSfx& p = pending_sfx_[i];
    if (!p.name) continue;
    p.t -= dt;
    if (p.t > 0.f) continue;
    Vec2 at = p.pos;
    if (p.follow == 1) at = player_.pos;
    else if (p.follow >= 100 && p.follow - 100 < vehicles_.count()) at = vehicles_.at(p.follow - 100)->pos;
    if (p.positioned) audio_->play_at(p.name, p.loop, p.db, at.x, at.y, p.tag);
    else audio_->play(p.name, p.loop, p.db, p.tag);
    p.name = 0;
  }

  // The Tutorial event sheet includes Game_events, so day/night/rain also run in the tutorial.

  // --- Day&Night: CompareBetween(Timer_Hours) + TriggerOnce, then inBunker == 0 ---
  const int h = weather_.timer_hours();
  const bool night_c = h >= 0 && h <= 5, day_c = h >= 6 && h <= 23;
  if (night_c && (!snd_night_trig_ || snd_first_tick_) && !in_bunker_) {   // sid607253053633264
    audio_->play(c2_night_loop_file(kC2CurrentLevel, false), true, 0.f, "Night_loop");
    audio_->stop("day_loop");
  }
  if (day_c && (!snd_day_trig_ || snd_first_tick_) && !in_bunker_) {       // sid650766536843728
    audio_->play(c2_day_loop_file(kC2CurrentLevel, false), true, 0.f, "day_loop");
    audio_->stop("night_loop");
  }
  snd_night_trig_ = night_c;
  snd_day_trig_ = day_c;
  snd_first_tick_ = false;

  // --- Rain (Rain timer): start sid609331584746158 / stop sid448001804847237 ---
  if (weather_.raining && !snd_rain_on_) {
    const int rl = weather_.rains_level;
    audio_->play(rl == 1 ? "rain_loop" : rl == 2 ? "blizzard" : "rain_loop", true, -5.f, "rain_loop");
    if (rl == 1) audio_->play("rainroof", true, -5.f, "rain_roof");   // else "----" (no file)
    const bool in337 = audio_player_in_warmzone(0);
    audio_->set_paused("rain_roof", !in337);   // sid126197426530717 / else sid648166332366210
    audio_->set_paused("rain_loop", in337);
  } else if (!weather_.raining && snd_rain_on_) {
    audio_->stop("rain_loop");
    audio_->stop("rain_roof");
  }
  snd_rain_on_ = weather_.raining;
  // House_transparenting Every 0.1 s, player overlapping t928 (sid1706727368417029 / 8515199558342265).
  snd_roof_t_ += dt;
  if (snd_roof_t_ >= 0.1f) {
    snd_roof_t_ = 0.f;
    if (weather_.raining && audio_player_in_house_area()) {
      const bool in337 = audio_player_in_warmzone(0);
      audio_->set_paused("rain_roof", !in337);
      audio_->set_paused("rain_loop", in337);
    }
  }

  // --- Fireplace loops (fireplace_loop, loop, -5 dB, tag "fireplaceloop_"&UID); Stop on the
  // Burn timer (sid2242683480655313). Molotov flames are not fireplaces.
  for (int i = 0; i < campfires_.count() && i < kMaxCampfires; ++i) {
    const Campfire* f = campfires_.at(i);
    const bool on = f->active && !f->molotov_burn;
    char tag[32];
    std::snprintf(tag, sizeof(tag), "fireplaceloop_%d", i);
    if (on && !snd_fire_on_[i]) audio_->play_at("fireplace_loop", true, -5.f, f->pos.x, f->pos.y, tag);
    else if (!on && snd_fire_on_[i]) audio_->stop(tag);
    snd_fire_on_[i] = on;
  }

  // --- Flare: flare_active OnCreated -> flare_burn loop -5 dB tag "Flare" (sid8003786567749088);
  // a looping positioned sound stops when its object is destroyed (c2runtime onInstanceDestroyed).
  const bool flare_on = flare_t_ > 0.f;
  if (flare_on && !snd_flare_on_) audio_->play_at("flare_burn", true, -5.f, flare_pos_.x, flare_pos_.y, "Flare");
  else if (!flare_on && snd_flare_on_) audio_->stop("Flare");
  snd_flare_on_ = flare_on;

  // --- Vehicles (GUI_draw_vehicle sid246391078095214 / sid894055228040088, exit
  // sid115496673766182 + sid806811931860373, engine rate sid422104780485244) ---
  {
    const Vehicle* dv = vehicles_.driven();
    int idx = -1;
    for (int i = 0; dv && i < vehicles_.count(); ++i) if (vehicles_.at(i) == dv) idx = i;
    auto kind = [](VehicleType t) -> const char* {
      return t == Vehicle_Car ? "uaz" : t == Vehicle_Volga ? "volga" : 0;
    };
    if (idx >= 0 && snd_car_idx_ < 0) {
      const char* k = kind(dv->type);
      if (k) {
        audio_->play_at("car_door_open", false, 0.f, player_.pos.x, player_.pos.y, "");
        sfx_later(0.7f, "car_door_close", 0.f, true, player_.pos, 1, false, "");
        if (dv->fuel > 0.f && dv->hp > 0.f) {
          char n1[40], n2[40], tag[32];
          std::snprintf(n1, sizeof(n1), "car_engine_start_%s", k);
          std::snprintf(n2, sizeof(n2), "car_engine_idle_%s", k);
          std::snprintf(tag, sizeof(tag), "%s_engine_%d", k, idx);
          audio_->play_at(n1, false, 0.f, player_.pos.x, player_.pos.y, "");
          sfx_later(0.7f, k[0] == 'u' ? "car_engine_idle_uaz" : "car_engine_idle_volga", -10.f, true,
                    dv->pos, 100 + idx, true, tag);
          (void)n2;
        }
      }
      snd_car_prev_pos_ = dv->pos;
    } else if (idx < 0 && snd_car_idx_ >= 0 && snd_car_idx_ < vehicles_.count()) {
      const Vehicle* ov = vehicles_.at(snd_car_idx_);
      const char* k = kind(ov->type);
      if (k) {
        char tag[32], n[40];
        std::snprintf(tag, sizeof(tag), "%s_engine_%d", k, snd_car_idx_);
        audio_->stop(tag);
        audio_->play_at("car_door_open", false, 0.f, ov->pos.x, ov->pos.y, "");
        if (ov->fuel > 0.f && ov->hp > 0.f) {
          std::snprintf(n, sizeof(n), "car_engine_stop_%s", k);
          audio_->play_at(n, false, 0.f, ov->pos.x, ov->pos.y, "");
        }
      }
    }
    if (idx >= 0) {
      const char* k = kind(dv->type);
      if (k) {
        char tag[32];
        std::snprintf(tag, sizeof(tag), "%s_engine_%d", k, idx);
        audio_->set_tag_position(tag, dv->pos.x, dv->pos.y);
        // SetPlaybackRate 1 + abs(Speed / MaxSpeed): the port's car moves at its max speed or
        // stands still, so the ratio is 1 while it moves and 0 otherwise.
        const bool moved = dd2(dv->pos, snd_car_prev_pos_) > 0.0001f;
        audio_->set_playback_rate(tag, moved ? 2.f : 1.f);
        // Broken car (IV1 <= 0): car_explode (sid728910415701570); TriggerOnce broken ->
        // Stop engine + car_engine_stop (sid971608970050581).
        if (dv->hp <= 0.f && !snd_car_broken_) {
          audio_->play_at("car_explode", false, 0.f, dv->pos.x, dv->pos.y, "");
          audio_->stop(tag);
          char n[40];
          std::snprintf(n, sizeof(n), "car_engine_stop_%s", k);
          audio_->play_at(n, false, 0.f, dv->pos.x, dv->pos.y, "");
        }
      }
      snd_car_broken_ = dv->hp <= 0.f;
      snd_car_prev_pos_ = dv->pos;
    } else {
      snd_car_broken_ = false;
    }
    snd_car_idx_ = idx;
  }

  // --- Zombies ---
  for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
    if (!z->alive) { z->snd_aggro = false; z->hit_tick = false; continue; }
    // Turret OnTargetAcquired (eyes) & distance < Player_Hear_radius -> spotted_0..6 [zmb].
    if (z->type != Zed_Screamer && z->type != Zed_Tank) {
      if (z->c2_aggro && !z->snd_aggro && dd2(z->pos, player_.pos) < kHearR * kHearR)
        audio_->play_at(c2_choose_spotted(sfx_rng_), false, 0.f, z->pos.x, z->pos.y, "zmb");
      z->snd_aggro = z->c2_aggro;
    }
    // Zed_common_dmg_deal / Zed_tank_dmg_deal: attack_* at the zed, tag zmb.
    if (z->hit_tick) audio_->play_at(c2_choose_zed_attack(sfx_rng_), false, 0.f, z->pos.x, z->pos.y, "zmb");
    z->hit_tick = false;
  }
  // zed screamer: Every 2.5 s while attacking & NOT IsTagPlaying("screamer") (sid133382250058741).
  if (zombies_.scream_active()) {
    snd_scream_t_ += dt;
    if (snd_scream_t_ >= 2.5f) {
      snd_scream_t_ -= 2.5f;
      if (!audio_->is_tag_playing("screamer")) {
        const Vec2 sp = zombies_.scream_pos();
        audio_->play_at("attack_8", false, -10.f, sp.x, sp.y, "screamer");
      }
    }
  } else {
    snd_scream_t_ = 0.f;
  }
  // NPC_manager Every 3 s: zed_buried_hidden within 200 px of the player -> deerrun (sid164081998282255).
  snd_buried_t_ += dt;
  if (snd_buried_t_ >= 3.f) {
    snd_buried_t_ -= 3.f;
    for (Zombie* z = zombies_.begin(); z != zombies_.end(); ++z) {
      if (!z->alive || z->type != Zed_Buried || z->c2_aggro) continue;
      if (dd2(z->pos, player_.pos) <= 200.f * 200.f)
        audio_->play_at("deerrun", false, 0.f, z->pos.x, z->pos.y, "");
    }
  }

  // --- Animals ---
  for (int i = 0; i < animals_.count(); ++i) {
    Animal* a = animals_.at(i);
    if (!a->alive) { a->snd_seen = false; a->hit_tick = false; continue; }
    if (a->type == Animal_Wolf || a->type == Animal_Dog) {
      // wolf_eyes / dog_eyes Turret OnTargetAcquired -> wolf_spot / wolf_spot_2 [zmb].
      if (a->snd_target && !a->snd_seen && dd2(a->pos, player_.pos) < kHearR * kHearR)
        audio_->play_at(c2_choose_wolf_spot(sfx_rng_), false, 0.f, a->pos.x, a->pos.y, "zmb");
      a->snd_seen = a->snd_target;
      if (a->hit_tick) {   // Wolf_dmg_deal (sid466188429934342)
        audio_->play_at(c2_choose_wolf_attack(sfx_rng_), false, 0.f, a->pos.x, a->pos.y, "zmb");
        audio_->play_at(c2_choose_body(sfx_rng_), false, -5.f, player_.pos.x, player_.pos.y, "zmb");
      }
      a->hit_tick = false;
    } else if (a->type == Animal_Crow) {
      // crow_fly: Wait random(2) -> crow_1..5 at the crow (sid980041957003775).
      if (a->flew_evt) { a->flew_evt = false; a->crow_snd_t = 0.0001f + sfx_rng_.range(0.f, 2.f); }
      if (a->crow_snd_t > 0.f) {
        a->crow_snd_t -= dt;
        if (a->crow_snd_t <= 0.f)
          audio_->play_at(c2_choose_crow(sfx_rng_), false, 0.f, a->pos.x, a->pos.y, "");
      }
    }
  }
}

} // namespace mdaz
