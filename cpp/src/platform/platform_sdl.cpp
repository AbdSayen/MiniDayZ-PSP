#include "mdaz/platform.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <string>

namespace mdaz {
IPlatform* create_null_platform();
}

#if defined(MDAZ_HAS_SDL2) && MDAZ_HAS_SDL2
#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include "stb_image.h"

namespace mdaz {

namespace {

// Tiny 3x5 digit/letter glyphs packed as 15 bits (rows top→bottom).
unsigned short glyph_bits(char c) {
  switch (c) {
  case '0': return 0x7B6F;
  case '1': return 0x2C97;
  case '2': return 0x73E7;
  case '3': return 0x72CF;
  case '4': return 0x5BC9;
  case '5': return 0x79CF;
  case '6': return 0x79EF;
  case '7': return 0x7249;
  case '8': return 0x7BEF;
  case '9': return 0x7BCF;
  case 'A': case 'a': return 0x7BED;
  case 'B': case 'b': return 0x6BAF;
  case 'C': case 'c': return 0x7927;
  case 'D': case 'd': return 0x6B6F;
  case 'E': case 'e': return 0x79E7;
  case 'F': case 'f': return 0x79E4;
  case 'G': case 'g': return 0x796F;
  case 'H': case 'h': return 0x5BED;
  case 'I': case 'i': return 0x7497;
  case 'K': case 'k': return 0x5BAD;
  case 'L': case 'l': return 0x4927;
  case 'M': case 'm': return 0x5F6D;
  case 'N': case 'n': return 0x5B6D;
  case 'O': case 'o': return 0x7B6F;
  case 'P': case 'p': return 0x7BE4;
  case 'R': case 'r': return 0x6BAD;
  case 'S': case 's': return 0x79CF;
  case 'T': case 't': return 0x7492;
  case 'U': case 'u': return 0x5B6F;
  case 'V': case 'v': return 0x5B6A;
  case 'W': case 'w': return 0x5B7F;
  case 'Y': case 'y': return 0x5A92;
  case 'Z': case 'z': return 0x72E7;
  case ':': return 0x0A0;
  case '/': return 0x12A4;
  case '-': return 0x01C0;
  case '.': return 0x0002;
  case ' ': return 0;
  case '+': return 0x05D0;
  case '%': return 0x4A52;
  default: return 0x7FFF;
  }
}

} // namespace

class SdlPlatform : public IPlatform {
public:
  SdlPlatform()
    : window_(0), renderer_(0), quit_(false),
      w_(kPspWidth * 2), h_(kPspHeight * 2),
      prev_attack_(false), prev_interact_(false), prev_inventory_(false),
      prev_reload_(false), prev_pause_(false), prev_confirm_(false),
      prev_save_(false), prev_load_(false), prev_torch_(false),
      prev_drop_(false), prev_mag_reload_(false), prev_throw_(false),
      prev_equip_prev_(false), prev_equip_next_(false) {
    images_dir_[0] = 0;
  }

  bool init(const char* title, int width, int height) {
    // Upscale PSP logical size for desktop readability.
    w_ = width > 0 ? width : kPspWidth * 2;
    h_ = height > 0 ? height : kPspHeight * 2;
    if (w_ < 640) { w_ = kPspWidth * 2; h_ = kPspHeight * 2; }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
      std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
      return false;
    }
    window_ = SDL_CreateWindow(title ? title : "Mini DAYZ C++",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               w_, h_, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window_) return false;
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) renderer_ = SDL_CreateRenderer(window_, -1, 0);
    if (!renderer_) return false;
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // nearest — original pixel art
    SDL_RenderSetLogicalSize(renderer_, kPspWidth, kPspHeight);
    return true;
  }

  void shutdown() {
    clear_textures();
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
  }

  bool load_sprites(const char* images_dir) {
    if (!images_dir || !images_dir[0]) return false;
    // Drop prior miss cache — candidate paths are tried in order
    clear_textures();
    std::strncpy(images_dir_, images_dir, sizeof(images_dir_) - 1);
    images_dir_[sizeof(images_dir_) - 1] = 0;
    // Probe exists on disk first (avoid poisoning miss cache)
    char probe[768];
    std::snprintf(probe, sizeof(probe), "%s/player_skin_def_1-sheet0.png", images_dir_);
    std::ifstream chk(probe, std::ios::binary);
    if (!chk) {
      std::fprintf(stderr, "[mdaz] sprites dir miss: %s\n", images_dir_);
      images_dir_[0] = 0;
      return false;
    }
    chk.close();
    if (!has_sprite("player_skin_def_1-sheet0")) {
      std::fprintf(stderr, "[mdaz] sprite decode fail: %s\n", probe);
      images_dir_[0] = 0;
      clear_textures();
      return false;
    }
    std::fprintf(stderr, "[mdaz] sprites OK: %s\n", images_dir_);
    return true;
  }

  bool has_sprite(const char* name) const {
    return const_cast<SdlPlatform*>(this)->texture_of(name) != 0;
  }

  void draw_sprite(const char* name, float x, float y, float w, float h,
                   float angle_deg, float alpha) {
    draw_sprite_frame(name, x, y, w, h, -1, -1, -1, -1, angle_deg, alpha);
  }

  void draw_sprite_frame(const char* name,
                         float x, float y, float w, float h,
                         int sx, int sy, int sw, int sh,
                         float angle_deg, float alpha) {
    SDL_Texture* tex = texture_of(name);
    if (!tex || w <= 0.f || h <= 0.f) return;
    if (alpha < 0.f) alpha = 0.f;
    if (alpha > 1.f) alpha = 1.f;
    SDL_SetTextureAlphaMod(tex, (Uint8)(alpha * 255.f));
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_Rect dst = { (int)x, (int)y, (int)w, (int)h };
    SDL_Rect src_storage;
    SDL_Rect* src = 0;
    if (sx >= 0 && sy >= 0 && sw > 0 && sh > 0) {
      src_storage.x = sx;
      src_storage.y = sy;
      src_storage.w = sw;
      src_storage.h = sh;
      src = &src_storage;
    }
    if (std::fabs(angle_deg) < 0.01f) {
      SDL_RenderCopy(renderer_, tex, src, &dst);
    } else {
      SDL_RenderCopyEx(renderer_, tex, src, &dst, (double)angle_deg, 0, SDL_FLIP_NONE);
    }
  }

  bool poll(InputState& out) {
    out = InputState();
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) quit_ = true;
      if (e.type == SDL_MOUSEWHEEL) {
        if (e.wheel.y > 0) out.equip_delta -= 1;
        if (e.wheel.y < 0) out.equip_delta += 1;
      }
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
    out.bag = keys[SDL_SCANCODE_TAB] != 0;
    out.run = keys[SDL_SCANCODE_LSHIFT] != 0;
    out.crouch = keys[SDL_SCANCODE_LCTRL] != 0 || keys[SDL_SCANCODE_RCTRL] != 0;
    out.pause = keys[SDL_SCANCODE_P] != 0;
    out.confirm = keys[SDL_SCANCODE_RETURN] != 0;
    out.cancel = keys[SDL_SCANCODE_ESCAPE] != 0;

    out.attack_pressed = out.attack && !prev_attack_;
    out.interact_pressed = out.interact && !prev_interact_;
    out.inventory_pressed = out.inventory && !prev_inventory_;
    out.reload_pressed = out.reload && !prev_reload_;
    out.pause_pressed = out.pause && !prev_pause_;
    out.confirm_pressed = out.confirm && !prev_confirm_;

    prev_attack_ = out.attack;
    prev_interact_ = out.interact;
    prev_inventory_ = out.inventory;
    prev_reload_ = out.reload;
    prev_pause_ = out.pause;
    prev_confirm_ = out.confirm;

    for (int d = 1; d <= 9; ++d) {
      if (keys[SDL_SCANCODE_1 + (d - 1)]) { out.slot_digit = d; break; }
    }

    out.save_pressed = keys[SDL_SCANCODE_F5] != 0 && !prev_save_;
    out.load_pressed = keys[SDL_SCANCODE_F9] != 0 && !prev_load_;
    out.torch_pressed = keys[SDL_SCANCODE_T] != 0 && !prev_torch_;
    out.drop_pressed = keys[SDL_SCANCODE_G] != 0 && !prev_drop_;
    out.mag_reload_pressed = keys[SDL_SCANCODE_V] != 0 && !prev_mag_reload_;
    out.throw_pressed = keys[SDL_SCANCODE_F] != 0 && !prev_throw_;
    if (keys[SDL_SCANCODE_Q] && !prev_equip_prev_) out.equip_delta -= 1;
    if (keys[SDL_SCANCODE_X] && !prev_equip_next_) out.equip_delta += 1; // cycle hotbar
    prev_save_ = keys[SDL_SCANCODE_F5] != 0;
    prev_load_ = keys[SDL_SCANCODE_F9] != 0;
    prev_torch_ = keys[SDL_SCANCODE_T] != 0;
    prev_drop_ = keys[SDL_SCANCODE_G] != 0;
    prev_mag_reload_ = keys[SDL_SCANCODE_V] != 0;
    prev_throw_ = keys[SDL_SCANCODE_F] != 0;
    prev_equip_prev_ = keys[SDL_SCANCODE_Q] != 0;
    prev_equip_next_ = keys[SDL_SCANCODE_X] != 0;

    // ESC is handled by Game (menu/pause); only window close quits the host.
    return !quit_;
  }

  double time_seconds() { return SDL_GetTicks() / 1000.0; }
  int screen_w() const { return kPspWidth; }
  int screen_h() const { return kPspHeight; }

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
    const float px = 2.f;
    float cx = x;
    for (const char* p = text; *p; ++p) {
      const unsigned short bits = glyph_bits(*p);
      for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 3; ++col) {
          const int bit = 14 - (row * 3 + col);
          if (bits & (1u << bit)) {
            draw_rect(cx + col * px, y + row * px, px, px, r, g, b, 1.f);
          }
        }
      }
      cx += 4.f * px;
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
  struct TexEntry {
    SDL_Texture* tex;
    int w, h;
    bool missing;
    TexEntry() : tex(0), w(0), h(0), missing(false) {}
  };

  void clear_textures() {
    for (std::map<std::string, TexEntry>::iterator it = tex_.begin(); it != tex_.end(); ++it) {
      if (it->second.tex) SDL_DestroyTexture(it->second.tex);
    }
    tex_.clear();
  }

  SDL_Texture* texture_of(const char* name) {
    if (!name || !name[0] || !renderer_ || !images_dir_[0]) return 0;
    std::map<std::string, TexEntry>::iterator it = tex_.find(name);
    if (it != tex_.end()) {
      if (it->second.missing) return 0;
      return it->second.tex;
    }
    TexEntry e;
    char path[768];
    // Accept "foo-sheet0" or "foo-sheet0.png"
    if (std::strstr(name, ".png"))
      std::snprintf(path, sizeof(path), "%s/%s", images_dir_, name);
    else
      std::snprintf(path, sizeof(path), "%s/%s.png", images_dir_, name);

    char* file_data = 0;
    std::size_t file_size = 0;
    if (!read_file(path, &file_data, &file_size) || !file_data) {
      e.missing = true;
      tex_[name] = e;
      return 0;
    }
    int w = 0, h = 0, comp = 0;
    unsigned char* pixels = stbi_load_from_memory(
      reinterpret_cast<unsigned char*>(file_data), static_cast<int>(file_size),
      &w, &h, &comp, 4);
    delete[] file_data;
    if (!pixels) {
      e.missing = true;
      tex_[name] = e;
      return 0;
    }
    SDL_Surface* surf = SDL_CreateRGBSurfaceFrom(
      pixels, w, h, 32, w * 4,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
      0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#else
      0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#endif
    );
    if (!surf) {
      stbi_image_free(pixels);
      e.missing = true;
      tex_[name] = e;
      return 0;
    }
    e.tex = SDL_CreateTextureFromSurface(renderer_, surf);
    SDL_FreeSurface(surf);
    stbi_image_free(pixels);
    if (!e.tex) {
      e.missing = true;
      tex_[name] = e;
      return 0;
    }
    e.w = w;
    e.h = h;
    tex_[name] = e;
    return e.tex;
  }

  SDL_Window* window_;
  SDL_Renderer* renderer_;
  bool quit_;
  int w_, h_;
  char images_dir_[512];
  std::map<std::string, TexEntry> tex_;
  bool prev_attack_, prev_interact_, prev_inventory_, prev_reload_, prev_pause_, prev_confirm_;
  bool prev_save_, prev_load_, prev_torch_;
  bool prev_drop_, prev_mag_reload_, prev_throw_;
  bool prev_equip_prev_, prev_equip_next_;
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
