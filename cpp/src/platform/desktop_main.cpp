#include "mdaz/audio.hpp"
#include "mdaz/game.hpp"
#include "mdaz/platform.hpp"
#include "mdaz/platform_null.hpp"
#include "mdaz/psp_ui.hpp"

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif

#include <cstdio>
#include <cstring>

namespace {

// --shots <dir>: plays a fixed input script and dumps BMP frames, then quits.
class ScriptedPlatform : public mdaz::IPlatform {
public:
  ScriptedPlatform(mdaz::IPlatform* inner, const char* dir) : inner_(inner), dir_(dir), frame_(0) {}
  ~ScriptedPlatform() { delete inner_; }
  bool init(const char* t, int w, int h) { return inner_->init(t, w, h); }
  void shutdown() { inner_->shutdown(); }
  double time_seconds() { return frame_ / 60.0; }
  void begin_frame() { inner_->begin_frame(); }
  void clear(float r, float g, float b) { inner_->clear(r, g, b); }
  void draw_rect(float x, float y, float w, float h, float r, float g, float b, float a) {
    inner_->draw_rect(x, y, w, h, r, g, b, a);
  }
  void draw_text(float x, float y, const char* s, float r, float g, float b) {
    inner_->draw_text(x, y, s, r, g, b);
  }
  bool load_sprites(const char* d) { return inner_->load_sprites(d); }
  bool has_sprite(const char* n) const { return inner_->has_sprite(n); }
  void draw_sprite(const char* n, float x, float y, float w, float h, float a, float al) {
    inner_->draw_sprite(n, x, y, w, h, a, al);
  }
  void draw_sprite_frame(const char* n, float dx, float dy, float dw, float dh,
                         int sx, int sy, int sw, int sh, float a, float al) {
    inner_->draw_sprite_frame(n, dx, dy, dw, dh, sx, sy, sw, sh, a, al);
  }
  bool read_file(const char* p, char** d, std::size_t* s) { return inner_->read_file(p, d, s); }
  bool write_file(const char* p, const void* d, std::size_t s) { return inner_->write_file(p, d, s); }
  int screen_w() const { return inner_->screen_w(); }
  int screen_h() const { return inner_->screen_h(); }

  void set_game(mdaz::Game* g, bool building, bool door = false, bool intro = false) {
    game_ = g; building_ = building; door_ = door; intro_ = intro;
    if (!intro && g) g->skip_intro(); // keep the existing shot scripts' frame numbers
  }

  bool poll(mdaz::InputState& in) {
    if (!inner_->poll(in)) return false;
    in = mdaz::InputState();
    ++frame_;
    if (intro_) return frame_ < 240;
    if (door_) return poll_door(in);
    if (building_) return poll_building(in);
    // Menu -> NEW GAME (focus 0).
    if (frame_ == 60) { in.confirm = in.confirm_pressed = true; }
    if (frame_ > 200 && frame_ < 420) in.move_x = 1.f;
    // Inventory: SELECT opens, D-pad moves focus, SELECT closes.
    if (frame_ == 480) { in.bag = in.bag_pressed = true; }
    if (frame_ == 490 || frame_ == 494) in.move_x = 1.f;
    if (frame_ == 498) in.move_y = 1.f;
    if (frame_ == 515) { in.bag = in.bag_pressed = true; }
    // Craft menu: R+Square opens, D-pad down, Circle closes.
    if (frame_ == 525) { in.craft_pressed = true; }
    if (frame_ == 530 || frame_ == 534) in.move_y = 1.f;
    if (frame_ == 548) { in.back_pressed = true; }
    if (frame_ > 560 && frame_ < 1400) { in.move_x = -0.7f; in.move_y = 0.7f; in.run = true; }
    // Pause menu: START, D-pad down once (SAVE focus) for the shot, then CONTROLS.
    if (frame_ == 1500) { in.pause = in.pause_pressed = true; }
    if (frame_ == 1515) in.move_y = 1.f;
    if (frame_ == 1535 || frame_ == 1540 || frame_ == 1545) in.move_y = 1.f;
    if (frame_ == 1555) { in.confirm = in.confirm_pressed = true; }
    return frame_ < 1580;
  }

  // --shots-building: test-only placement next to a Solid building, then walk into its walls.
  bool poll_building(mdaz::InputState& in) {
    if (frame_ == 60) { in.confirm = in.confirm_pressed = true; }
    if (frame_ == 200 && game_) game_->harness_place_at_building(1); // south of the building
    if (frame_ > 260 && frame_ < 420) in.move_y = -1.f;              // walk north into the wall
    if (frame_ == 480 && game_) game_->harness_place_at_building(0); // inside (warmzone)
    if (frame_ > 540 && frame_ < 700) in.move_x = -1.f;              // walk west into a wall
    return frame_ < 760;
  }

  // --shots-door: start south of the police station (one test placement), push north into the
  // wall east of the door, side-step west to the door gap (x -31..-1), then walk in. Input only.
  bool poll_door(mdaz::InputState& in) {
    if (frame_ == 60) { in.confirm = in.confirm_pressed = true; }
    if (frame_ == 200 && game_) game_->harness_place_at_building(2);
    if (frame_ > 220 && frame_ <= 310) in.move_y = -1.f;               // ~1.5 s into the wall
    else if (frame_ > 310 && game_ && door_phase_ == 0) {
      if (game_->harness_player_rel().x > -16.f) in.move_x = -1.f; else door_phase_ = 1;
    }
    if (door_phase_ == 1 && frame_ > 310) {                             // through the door,
      if (game_->harness_player_rel().y > 60.f) in.move_y = -1.f;      // stop once inside
      else door_phase_ = 2;
    }
    return frame_ < 620;
  }

  void end_frame() {
    if (intro_) {
      static const int kF[] = { 1, 60, 120, 200, 215, 230 };
      for (unsigned i = 0; i < sizeof(kF) / sizeof(kF[0]); ++i) {
        if (kF[i] != frame_ || shot_done_[i]) continue;
        shot_done_[i] = true;
        char path[512];
        std::snprintf(path, sizeof(path), "%s/intro_f%03d.bmp", dir_, frame_);
        std::printf("%s %s\n", inner_->save_screenshot(path) ? "saved" : "FAILED", path);
      }
      inner_->end_frame();
      return;
    }
    if (door_) {
      if (frame_ >= 210 && frame_ <= 600 && (frame_ - 210) % 35 == 0) {
        char path[512];
        std::snprintf(path, sizeof(path), "%s/door_%02d_f%d.bmp", dir_, (frame_ - 210) / 35, frame_);
        std::printf("%s %s\n", inner_->save_screenshot(path) ? "saved" : "FAILED", path);
      }
      inner_->end_frame();
      return;
    }
    if (building_) {
      static const struct { int frame; const char* name; } kB[] = {
        { 250, "bld_outside" }, { 430, "bld_north_wall" }, { 530, "bld_inside" }, { 710, "bld_west_wall" },
      };
      for (unsigned i = 0; i < sizeof(kB) / sizeof(kB[0]); ++i) {
        if (kB[i].frame != frame_) continue;
        char path[512];
        std::snprintf(path, sizeof(path), "%s/%s.bmp", dir_, kB[i].name);
        std::printf("%s %s\n", inner_->save_screenshot(path) ? "saved" : "FAILED", path);
      }
      inner_->end_frame();
      return;
    }
    static const struct { int frame; const char* name; } kShots[] = {
      { 50, "menu" }, { 180, "spawn" },
      { 220, "run_a" }, { 226, "run_b" }, { 232, "run_c" }, { 238, "run_d" },
      { 430, "walk" }, { 510, "bag" }, { 545, "craft" },
      { 900, "explore1" }, { 1390, "explore2" }, { 1530, "pause" }, { 1570, "controls" },
    };
    for (unsigned i = 0; i < sizeof(kShots) / sizeof(kShots[0]); ++i) {
      if (kShots[i].frame != frame_) continue;
      char path[512];
      std::snprintf(path, sizeof(path), "%s/%s.bmp", dir_, kShots[i].name);
      std::printf("%s %s\n", inner_->save_screenshot(path) ? "saved" : "FAILED", path);
    }
    inner_->end_frame();
  }

private:
  mdaz::IPlatform* inner_;
  const char* dir_;
  int frame_;
  mdaz::Game* game_ = 0;
  bool building_ = false;
  bool door_ = false;
  bool intro_ = false;
  bool shot_done_[8] = {};
  int door_phase_ = 0;
};

} // namespace

int main(int argc, char** argv) {
  const char* data_dir = "data";
  const char* shots_dir = 0;
  bool shots_building = false;
  bool shots_door = false;
  bool shots_intro = false;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--shots") == 0 && i + 1 < argc) shots_dir = argv[++i];
    else if (std::strcmp(argv[i], "--shots-building") == 0 && i + 1 < argc) {
      shots_dir = argv[++i];
      shots_building = true;
    }
    else if (std::strcmp(argv[i], "--shots-intro") == 0 && i + 1 < argc) {
      shots_dir = argv[++i];
      shots_intro = true;
    }
    else if (std::strcmp(argv[i], "--shots-door") == 0 && i + 1 < argc) {
      shots_dir = argv[++i];
      shots_door = true;
    }
    else if (std::strcmp(argv[i], "--keys") == 0) mdaz::ui::set_keyboard_hints(true); // keyboard caps in hints
    else data_dir = argv[i];
  }

  mdaz::IPlatform* platform = mdaz::create_platform();
  if (!platform) {
    std::fprintf(stderr, "Failed to create platform\n");
    return 1;
  }
  ScriptedPlatform* scripted = 0;
  if (shots_dir) platform = scripted = new ScriptedPlatform(platform, shots_dir);

  // 2x PSP resolution window with logical 480x272 render
  if (!platform->init("Mini DAYZ C++ (1.4.1)", mdaz::kPspWidth * 2, mdaz::kPspHeight * 2)) {
    std::fprintf(stderr, "Platform init failed — falling back to null\n");
    platform->shutdown();
    delete platform;
    scripted = 0;
    platform = mdaz::create_null_platform();
    platform->init("Mini DAYZ C++", mdaz::kPspWidth, mdaz::kPspHeight);
  }

  mdaz::IAudio* audio = mdaz::create_sdl_audio();
  // Original 1.4.1 Construct media pack
  audio->set_media_dir("C:/Users/abdur/Desktop/MiniDayZ/original_docs/media");

  mdaz::Game game;
  if (!game.init(platform, data_dir)) {
    std::fprintf(stderr, "Game init failed\n");
    delete audio;
    platform->shutdown();
    delete platform;
    return 1;
  }
  game.set_audio(audio);
  if (scripted) scripted->set_game(&game, shots_building, shots_door, shots_intro);

  double prev = platform->time_seconds();
  while (!game.wants_quit()) {
    double now = platform->time_seconds();
    float dt = static_cast<float>(now - prev);
    prev = now;
    if (dt < 0.f) dt = 0.f;
    if (dt > 0.1f) dt = 0.1f;
    game.update(dt);
    game.render();
  }

  game.shutdown();
  delete audio;
  platform->shutdown();
  delete platform;
  return 0;
}
