#include "mdaz/platform.hpp"

#include <cstdio>
#include <fstream>

namespace mdaz {
IPlatform* create_null_platform();
}

#if defined(MDAZ_HAS_SDL2) && MDAZ_HAS_SDL2
#include <SDL.h>

namespace mdaz {

class SdlPlatform : public IPlatform {
public:
  SdlPlatform() : window_(0), renderer_(0), quit_(false) {}

  bool init(const char* title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
      std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
      return false;
    }
    window_ = SDL_CreateWindow(title ? title : "Mini DAYZ C++",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               width, height, SDL_WINDOW_SHOWN);
    if (!window_) return false;
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_) renderer_ = SDL_CreateRenderer(window_, -1, 0);
    return renderer_ != 0;
  }

  void shutdown() {
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
  }

  bool poll(InputState& out) {
    out = InputState();
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) quit_ = true;
    }
    const Uint8* keys = SDL_GetKeyboardState(0);
    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) out.move_x -= 1.f;
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) out.move_x += 1.f;
    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) out.move_y -= 1.f;
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) out.move_y += 1.f;
    out.attack = keys[SDL_SCANCODE_Z] || keys[SDL_SCANCODE_SPACE];
    out.interact = keys[SDL_SCANCODE_E] != 0;
    out.inventory = keys[SDL_SCANCODE_C] != 0;
    out.reload = keys[SDL_SCANCODE_R] != 0;
    out.run = keys[SDL_SCANCODE_LSHIFT] != 0;
    out.pause = keys[SDL_SCANCODE_P] != 0;
    out.confirm = keys[SDL_SCANCODE_RETURN] != 0;
    out.cancel = keys[SDL_SCANCODE_ESCAPE] != 0;
    if (out.cancel) quit_ = true;
    return !quit_;
  }

  double time_seconds() { return SDL_GetTicks() / 1000.0; }

  void begin_frame() {}
  void clear(float r, float g, float b) {
    SDL_SetRenderDrawColor(renderer_,
                           (Uint8)(r * 255), (Uint8)(g * 255), (Uint8)(b * 255), 255);
    SDL_RenderClear(renderer_);
  }
  void draw_rect(float x, float y, float w, float h, float r, float g, float b, float a) {
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_,
                           (Uint8)(r * 255), (Uint8)(g * 255), (Uint8)(b * 255), (Uint8)(a * 255));
    SDL_Rect rect = { (int)x, (int)y, (int)w, (int)h };
    SDL_RenderFillRect(renderer_, &rect);
  }
  void draw_text(float x, float y, const char* text, float r, float g, float b) {
    if (!text) return;
    float cx = x;
    for (const char* p = text; *p; ++p) {
      draw_rect(cx, y, 4, 8, r, g, b, 0.9f);
      cx += 5.f;
    }
  }
  void end_frame() { SDL_RenderPresent(renderer_); }

  bool read_file(const char* path, char** out_data, std::size_t* out_size) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    in.seekg(0, std::ios::end);
    const std::streamoff len = in.tellg();
    in.seekg(0, std::ios::beg);
    char* buf = new char[static_cast<std::size_t>(len) + 1];
    in.read(buf, len);
    buf[len] = 0;
    *out_data = buf;
    *out_size = static_cast<std::size_t>(len);
    return true;
  }
  bool write_file(const char* path, const void* data, std::size_t size) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    return true;
  }

private:
  SDL_Window* window_;
  SDL_Renderer* renderer_;
  bool quit_;
};

IPlatform* create_platform() {
  return new SdlPlatform();
}

} // namespace mdaz

#else

namespace mdaz {

IPlatform* create_platform() {
  return create_null_platform();
}

} // namespace mdaz

#endif
