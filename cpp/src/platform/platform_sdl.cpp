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

// 5x7 pixel font: 7 rows top→bottom, bit 4 = leftmost column. Letters are upper case only.
struct Glyph { char c; unsigned char rows[7]; };
const Glyph kGlyphs[] = {
  { '0', { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E } },
  { '1', { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E } },
  { '2', { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F } },
  { '3', { 0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E } },
  { '4', { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 } },
  { '5', { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E } },
  { '6', { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E } },
  { '7', { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 } },
  { '8', { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E } },
  { '9', { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C } },
  { 'A', { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
  { 'B', { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E } },
  { 'C', { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E } },
  { 'D', { 0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C } },
  { 'E', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F } },
  { 'F', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 } },
  { 'G', { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F } },
  { 'H', { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
  { 'I', { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E } },
  { 'J', { 0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C } },
  { 'K', { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 } },
  { 'L', { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F } },
  { 'M', { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 } },
  { 'N', { 0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11 } },
  { 'O', { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
  { 'P', { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 } },
  { 'Q', { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D } },
  { 'R', { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 } },
  { 'S', { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E } },
  { 'T', { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 } },
  { 'U', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
  { 'V', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 } },
  { 'W', { 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A } },
  { 'X', { 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 } },
  { 'Y', { 0x11, 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04 } },
  { 'Z', { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F } },
  { ':', { 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00 } },
  { '/', { 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x00 } },
  { '-', { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 } },
  { '.', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C } },
  { ',', { 0x00, 0x00, 0x00, 0x00, 0x0C, 0x04, 0x08 } },
  { '+', { 0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00 } },
  { '%', { 0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03 } },
  { '!', { 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04 } },
  { '?', { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04 } },
  { '\'', { 0x04, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00 } },
  { '(', { 0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02 } },
  { ')', { 0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08 } },
  { '<', { 0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02 } },
  { '>', { 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08 } },
  { '=', { 0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00 } },
  { '#', { 0x0A, 0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x0A } },
  { '*', { 0x00, 0x04, 0x15, 0x0E, 0x15, 0x04, 0x00 } },
  { '_', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F } },
  { '[', { 0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E } },
  { ']', { 0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E } },
};

const unsigned char* glyph_rows(char c) {
  static const unsigned char kBlank[7] = { 0, 0, 0, 0, 0, 0, 0 };
  static const unsigned char kUnknown[7] = { 0x1F, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F };
  if (c == ' ') return kBlank;
  if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
  for (unsigned i = 0; i < sizeof(kGlyphs) / sizeof(kGlyphs[0]); ++i)
    if (kGlyphs[i].c == c) return kGlyphs[i].rows;
  return kUnknown;
}

} // namespace

class SdlPlatform : public IPlatform {
public:
  SdlPlatform()
    : window_(0), renderer_(0), quit_(false),
      w_(kPspWidth * 2), h_(kPspHeight * 2),
      prev_attack_(false), prev_interact_(false), prev_inventory_(false), prev_bag_(false),
      prev_reload_(false), prev_pause_(false), prev_confirm_(false),
      prev_save_(false), prev_load_(false), prev_torch_(false),
      prev_drop_(false), prev_mag_reload_(false), prev_craft_(false), prev_throw_(false),
      prev_equip_prev_(false), prev_equip_next_(false), prev_mouse_left_(false) {
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
    if (!has_sprite("bitmap_font-sheet0"))
      std::fprintf(stderr, "[mdaz] HUD font: 5x7 fallback (no bitmap_font sheet in images)\n");
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
    SDL_Texture* tex = sil_ ? silhouette_of(name) : texture_of(name);
    if (!tex || w <= 0.f || h <= 0.f) return;
    if (alpha < 0.f) alpha = 0.f;
    if (alpha > 1.f) alpha = 1.f;
    SDL_SetTextureAlphaMod(tex, (Uint8)(alpha * 255.f));
    SDL_SetTextureColorMod(tex, tint_[0], tint_[1], tint_[2]);
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

  void draw_sprite_frame_mirrored(const char* name, float x, float y, float w, float h,
                                  int sx, int sy, int sw, int sh, float alpha) {
    SDL_Texture* tex = sil_ ? silhouette_of(name) : texture_of(name);
    if (!tex || w <= 0.f || h <= 0.f) return;
    if (alpha < 0.f) alpha = 0.f;
    if (alpha > 1.f) alpha = 1.f;
    SDL_SetTextureAlphaMod(tex, (Uint8)(alpha * 255.f));
    SDL_SetTextureColorMod(tex, tint_[0], tint_[1], tint_[2]);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_Rect dst = { (int)x, (int)y, (int)w, (int)h };
    SDL_Rect src = { sx, sy, sw, sh };
    SDL_RenderCopyEx(renderer_, tex, &src, &dst, 0.0, 0, SDL_FLIP_HORIZONTAL);
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
    out.bag_pressed = out.bag && !prev_bag_;
    out.run = keys[SDL_SCANCODE_LSHIFT] != 0;
    out.crouch = false; // C2 has no crouch
    out.pause = keys[SDL_SCANCODE_P] != 0;
    out.confirm = keys[SDL_SCANCODE_RETURN] != 0;
    out.cancel = keys[SDL_SCANCODE_ESCAPE] != 0;

    out.attack_pressed = out.attack && !prev_attack_;
    out.interact_pressed = out.interact && !prev_interact_;
    out.inventory_pressed = out.inventory && !prev_inventory_;
    out.reload_pressed = out.reload && !prev_reload_;
    out.pause_pressed = out.pause && !prev_pause_;
    // PSP-style semantics on desktop: Esc = Start (pause) in game and = Circle (back) in menus.
    {
      const bool esc = keys[SDL_SCANCODE_ESCAPE] != 0;
      const bool bksp = keys[SDL_SCANCODE_BACKSPACE] != 0;
      if (esc && !prev_esc_) { out.pause_pressed = true; out.back_pressed = true; }
      if (bksp && !prev_bksp_) out.back_pressed = true;
      if (out.interact_pressed) out.back_pressed = true; // E == Circle
      if (keys[SDL_SCANCODE_SPACE] && !prev_space_) out.confirm_pressed = true;
      prev_esc_ = esc;
      prev_bksp_ = bksp;
      prev_space_ = keys[SDL_SCANCODE_SPACE] != 0;
    }
    out.confirm_pressed = (out.confirm && !prev_confirm_) || out.confirm_pressed;

    prev_attack_ = out.attack;
    prev_interact_ = out.interact;
    prev_inventory_ = out.inventory;
    prev_bag_ = out.bag;
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
    out.craft_pressed = keys[SDL_SCANCODE_B] != 0 && !prev_craft_;
    out.throw_pressed = keys[SDL_SCANCODE_F] != 0 && !prev_throw_;
    if (keys[SDL_SCANCODE_Q] && !prev_equip_prev_) out.equip_delta -= 1;
    if (keys[SDL_SCANCODE_X] && !prev_equip_next_) out.equip_delta += 1; // cycle hotbar
    prev_save_ = keys[SDL_SCANCODE_F5] != 0;
    prev_load_ = keys[SDL_SCANCODE_F9] != 0;
    prev_torch_ = keys[SDL_SCANCODE_T] != 0;
    prev_drop_ = keys[SDL_SCANCODE_G] != 0;
    prev_mag_reload_ = keys[SDL_SCANCODE_V] != 0;
    prev_craft_ = keys[SDL_SCANCODE_B] != 0;
    prev_throw_ = keys[SDL_SCANCODE_F] != 0;
    prev_equip_prev_ = keys[SDL_SCANCODE_Q] != 0;
    prev_equip_next_ = keys[SDL_SCANCODE_X] != 0;

    int mwx = 0, mwy = 0;
    const Uint32 mbuttons = SDL_GetMouseState(&mwx, &mwy);
    out.mouse_left = (mbuttons & SDL_BUTTON_LMASK) != 0;
    out.mouse_left_pressed = out.mouse_left && !prev_mouse_left_;
    prev_mouse_left_ = out.mouse_left;
    if (renderer_) {
      float lx = 0.f, ly = 0.f;
      SDL_RenderWindowToLogical(renderer_, static_cast<float>(mwx),
                                static_cast<float>(mwy), &lx, &ly);
      out.mouse_x = lx;
      out.mouse_y = ly;
    }

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
    // 5x7 font, 1 logical px per font pixel + 1px drop shadow (matches PSP).
    const float px = 1.0f; // same metrics as PSP fb font (6 px advance)
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    float cx = x;
    for (const char* p = text; *p; ++p) {
      const unsigned char* rows = glyph_rows(*p);
      for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
          if (!(rows[row] & (0x10 >> col))) continue;
          const SDL_FRect px_rect = { cx + col * px + 1.f, y + row * px + 1.f, px, px };
          SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 160);
          SDL_RenderFillRectF(renderer_, &px_rect);
        }
      }
      for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
          if (!(rows[row] & (0x10 >> col))) continue;
          const SDL_FRect px_rect = { cx + col * px, y + row * px, px, px };
          SDL_SetRenderDrawColor(renderer_, (Uint8)(r * 255), (Uint8)(g * 255), (Uint8)(b * 255), 255);
          SDL_RenderFillRectF(renderer_, &px_rect);
        }
      }
      cx += 6.f * px;
    }
  }
  void end_frame() { SDL_RenderPresent(renderer_); }

  bool save_screenshot(const char* path) {
    int w = 0, h = 0;
    if (SDL_GetRendererOutputSize(renderer_, &w, &h) != 0) return false;
    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!s) return false;
    bool ok = SDL_RenderReadPixels(renderer_, 0, SDL_PIXELFORMAT_ARGB8888, s->pixels, s->pitch) == 0 &&
              SDL_SaveBMP(s, path) == 0;
    SDL_FreeSurface(s);
    return ok;
  }

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

  void set_sprite_tint(float r, float g, float b) {
    tint_[0] = (Uint8)(r * 255.f + 0.5f); tint_[1] = (Uint8)(g * 255.f + 0.5f); tint_[2] = (Uint8)(b * 255.f + 0.5f);
  }
  void set_sprite_silhouette(bool on) { sil_ = on; }

  // White copy of a sheet (RGB = 255, alpha kept): colour-modded to the outline colour.
  SDL_Texture* silhouette_of(const char* name) {
    if (!texture_of(name)) return 0;
    std::map<std::string, SDL_Texture*>::iterator it = sil_tex_.find(name);
    if (it != sil_tex_.end()) return it->second;
    char path[768];
    std::snprintf(path, sizeof(path), std::strstr(name, ".png") ? "%s/%s" : "%s/%s.png", images_dir_, name);
    char* file_data = 0; std::size_t file_size = 0;
    SDL_Texture* t = 0;
    if (read_file(path, &file_data, &file_size) && file_data) {
      int w = 0, h = 0, comp = 0;
      unsigned char* px = stbi_load_from_memory(reinterpret_cast<unsigned char*>(file_data),
                                                static_cast<int>(file_size), &w, &h, &comp, 4);
      delete[] file_data;
      if (px) {
        for (int i = 0; i < w * h; ++i) { px[i * 4] = 255; px[i * 4 + 1] = 255; px[i * 4 + 2] = 255; }
        SDL_Surface* surf = SDL_CreateRGBSurfaceFrom(px, w, h, 32, w * 4,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
          0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#else
          0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#endif
        );
        if (surf) { t = SDL_CreateTextureFromSurface(renderer_, surf); SDL_FreeSurface(surf); }
        stbi_image_free(px);
        if (t) SDL_SetTextureScaleMode(t, SDL_ScaleModeNearest);
      }
    }
    sil_tex_[name] = t;
    return t;
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
    bool got = read_file(path, &file_data, &file_size) && file_data;
    if (!got) {
      // Port-only assets (Bonds Guru intro) live in data/intro, not in the original images dir.
      static const char* kExtra[] = { "data/intro", "C:/Users/abdur/Desktop/MiniDayZ/cpp/data/intro", 0 };
      for (int k = 0; kExtra[k] && !got; ++k) {
        std::snprintf(path, sizeof(path), std::strstr(name, ".png") ? "%s/%s" : "%s/%s.png", kExtra[k], name);
        got = read_file(path, &file_data, &file_size) && file_data;
      }
    }
    if (!got) {
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
    // Pixel art — nearest filter avoids blur / rainbow fringe when scaled.
    SDL_SetTextureScaleMode(e.tex, SDL_ScaleModeNearest);
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
  std::map<std::string, SDL_Texture*> sil_tex_;
  Uint8 tint_[3] = {255, 255, 255};
  bool sil_ = false;
  bool prev_attack_, prev_interact_, prev_inventory_, prev_bag_, prev_reload_, prev_pause_,
      prev_confirm_;
  bool prev_save_, prev_load_, prev_torch_;
  bool prev_drop_, prev_mag_reload_, prev_craft_, prev_throw_;
  bool prev_equip_prev_, prev_equip_next_;
  bool prev_mouse_left_;
  bool prev_esc_ = false, prev_bksp_ = false, prev_space_ = false;
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
