#include "mdaz/audio.hpp"
#include "mdaz/game.hpp"
#include "mdaz/platform.hpp"
#include "mdaz/platform_null.hpp"

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

  bool poll(mdaz::InputState& in) {
    if (!inner_->poll(in)) return false;
    in = mdaz::InputState();
    ++frame_;
    if (frame_ == 60) { in.confirm = in.confirm_pressed = true; }
    if (frame_ > 200 && frame_ < 420) in.move_x = 1.f;
    if (frame_ >= 480 && frame_ < 520) in.bag = true;
    if (frame_ > 560 && frame_ < 1400) { in.move_x = -0.7f; in.move_y = 0.7f; in.run = true; }
    if (frame_ == 1500) { in.pause = in.pause_pressed = true; }
    return frame_ < 1540;
  }

  void end_frame() {
    static const struct { int frame; const char* name; } kShots[] = {
      { 50, "menu" }, { 180, "spawn" }, { 430, "walk" }, { 510, "bag" },
      { 900, "explore1" }, { 1390, "explore2" }, { 1530, "pause" },
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
};

} // namespace

int main(int argc, char** argv) {
  const char* data_dir = "data";
  const char* shots_dir = 0;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--shots") == 0 && i + 1 < argc) shots_dir = argv[++i];
    else data_dir = argv[i];
  }

  mdaz::IPlatform* platform = mdaz::create_platform();
  if (!platform) {
    std::fprintf(stderr, "Failed to create platform\n");
    return 1;
  }
  if (shots_dir) platform = new ScriptedPlatform(platform, shots_dir);

  // 2x PSP resolution window with logical 480x272 render
  if (!platform->init("Mini DAYZ C++ (1.4.1)", mdaz::kPspWidth * 2, mdaz::kPspHeight * 2)) {
    std::fprintf(stderr, "Platform init failed — falling back to null\n");
    platform->shutdown();
    delete platform;
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
