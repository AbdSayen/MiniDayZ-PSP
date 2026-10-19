#include "mdaz/platform.hpp"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>

#if defined(MDAZ_PSP) && defined(__PSP__)
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspge.h>
#include <pspkernel.h>
#include <pspiofilemgr.h>
#include <psppower.h>
#include <psprtc.h>
#include <malloc.h>
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include "stb_image.h"
#endif

namespace mdaz {

#if !(defined(MDAZ_PSP) && defined(__PSP__))

IPlatform* create_platform() {
  std::fprintf(stderr, "platform_psp.cpp: not building with PSPSDK; stub.\n");
  return 0;
}

#else

namespace {

static unsigned int __attribute__((aligned(16))) s_gu_list[262144];

// PSP 565 = BBBBBGGGGGGRRRRR (R in low bits). Toggle if a specific build looks swapped.
#ifndef MDAZ_PSP_BGR565
#define MDAZ_PSP_BGR565 1
#endif

static unsigned short rgba565(float r, float g, float b, float a) {
  if (a < 0.02f) return 0;
  int ri = (int)(r * 31.f + 0.5f); if (ri < 0) ri = 0; if (ri > 31) ri = 31;
  int gi = (int)(g * 63.f + 0.5f); if (gi < 0) gi = 0; if (gi > 63) gi = 63;
  int bi = (int)(b * 31.f + 0.5f); if (bi < 0) bi = 0; if (bi > 31) bi = 31;
#if MDAZ_PSP_BGR565
  return (unsigned short)((bi << 11) | (gi << 5) | ri);
#else
  return (unsigned short)((ri << 11) | (gi << 5) | bi);
#endif
}

static unsigned short rgba8_to_565(unsigned char r, unsigned char g, unsigned char b) {
#if MDAZ_PSP_BGR565
  return (unsigned short)(((b >> 3) << 11) | ((g >> 2) << 5) | (r >> 3));
#else
  return (unsigned short)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
#endif
}

static void fb_fill(unsigned short* fb, int x, int y, int w, int h, unsigned short c) {
  if (!fb || w <= 0 || h <= 0) return;
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > 480) w = 480 - x;
  if (y + h > 272) h = 272 - y;
  if (w <= 0 || h <= 0) return;
  for (int row = 0; row < h; ++row) {
    unsigned short* dst = fb + (y + row) * 512 + x;
    for (int col = 0; col < w; ++col) dst[col] = c;
  }
}

// Alpha-blended fill (RGB565/BGR565 — per-channel blend is order-agnostic).
static void fb_fill_blend(unsigned short* fb, int x, int y, int w, int h, unsigned short c, int a8) {
  if (!fb || w <= 0 || h <= 0 || a8 <= 4) return;
  if (a8 >= 250) { fb_fill(fb, x, y, w, h, c); return; }
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > 480) w = 480 - x;
  if (y + h > 272) h = 272 - y;
  if (w <= 0 || h <= 0) return;
  const int a5 = (a8 + 4) >> 3;           // 0..32
  const int ia = 32 - a5;
  const int cr = c >> 11, cg = (c >> 5) & 63, cb = c & 31;
  const int pr = cr * a5, pg = cg * a5, pb = cb * a5;
  for (int row = 0; row < h; ++row) {
    unsigned short* dst = fb + (y + row) * 512 + x;
    for (int col = 0; col < w; ++col) {
      const unsigned short d = dst[col];
      const int r = (pr + (d >> 11) * ia) >> 5;
      const int g = (pg + ((d >> 5) & 63) * ia) >> 5;
      const int b = (pb + (d & 31) * ia) >> 5;
      dst[col] = (unsigned short)((r << 11) | (g << 5) | b);
    }
  }
}

static const unsigned char kGlyph5x7[96][7] = {
  {0,0,0,0,0,0,0},
  {0x04,0x04,0x04,0x04,0x00,0x04,0x00},{0x0A,0x0A,0x00,0x00,0x00,0x00,0x00},
  {0x0A,0x1F,0x0A,0x1F,0x0A,0x00,0x00},{0x04,0x0F,0x14,0x0E,0x05,0x1E,0x04},
  {0x18,0x19,0x02,0x04,0x08,0x13,0x03},{0x08,0x14,0x14,0x08,0x15,0x12,0x0D},
  {0x04,0x04,0x00,0x00,0x00,0x00,0x00},{0x02,0x04,0x04,0x04,0x04,0x04,0x02},
  {0x08,0x04,0x04,0x04,0x04,0x04,0x08},{0x00,0x0A,0x04,0x1F,0x04,0x0A,0x00},
  {0x00,0x04,0x04,0x1F,0x04,0x04,0x00},{0x00,0x00,0x00,0x00,0x04,0x04,0x08},
  {0x00,0x00,0x00,0x1F,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x04,0x00},
  {0x01,0x02,0x04,0x08,0x10,0x00,0x00},{0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},
  {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},{0x0E,0x11,0x01,0x06,0x08,0x10,0x1F},
  {0x0E,0x11,0x01,0x06,0x01,0x11,0x0E},{0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
  {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},{0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},
  {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},{0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
  {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},{0x00,0x04,0x00,0x00,0x00,0x04,0x00},
  {0x00,0x04,0x00,0x00,0x04,0x04,0x08},{0x02,0x04,0x08,0x10,0x08,0x04,0x02},
  {0x00,0x00,0x1F,0x00,0x1F,0x00,0x00},{0x08,0x04,0x02,0x01,0x02,0x04,0x08},
  {0x0E,0x11,0x01,0x06,0x04,0x00,0x04},{0x0E,0x11,0x17,0x15,0x17,0x10,0x0E},
  {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},{0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},
  {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},{0x1E,0x11,0x11,0x11,0x11,0x11,0x1E},
  {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},{0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
  {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F},{0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
  {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},{0x01,0x01,0x01,0x01,0x11,0x11,0x0E},
  {0x11,0x12,0x14,0x18,0x14,0x12,0x11},{0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
  {0x11,0x1B,0x15,0x11,0x11,0x11,0x11},{0x11,0x19,0x15,0x13,0x11,0x11,0x11},
  {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},{0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},
  {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D},{0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
  {0x0E,0x11,0x10,0x0E,0x01,0x11,0x0E},{0x1F,0x04,0x04,0x04,0x04,0x04,0x04},
  {0x11,0x11,0x11,0x11,0x11,0x11,0x0E},{0x11,0x11,0x11,0x11,0x11,0x0A,0x04},
  {0x11,0x11,0x11,0x15,0x15,0x1B,0x11},{0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
  {0x11,0x11,0x0A,0x04,0x04,0x04,0x04},{0x1F,0x01,0x02,0x04,0x08,0x10,0x1F},
  {0x0E,0x08,0x08,0x08,0x08,0x08,0x0E},{0x10,0x08,0x04,0x02,0x01,0x00,0x00},
  {0x0E,0x02,0x02,0x02,0x02,0x02,0x0E},{0x04,0x0A,0x11,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00,0x1F},{0x08,0x04,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x0E,0x01,0x0F,0x11,0x0F},{0x10,0x10,0x1E,0x11,0x11,0x11,0x1E},
  {0x00,0x00,0x0E,0x11,0x10,0x11,0x0E},{0x01,0x01,0x0F,0x11,0x11,0x11,0x0F},
  {0x00,0x00,0x0E,0x11,0x1F,0x10,0x0E},{0x06,0x08,0x08,0x1E,0x08,0x08,0x08},
  {0x00,0x00,0x0F,0x11,0x0F,0x01,0x0E},{0x10,0x10,0x1E,0x11,0x11,0x11,0x11},
  {0x04,0x00,0x0C,0x04,0x04,0x04,0x0E},{0x02,0x00,0x06,0x02,0x02,0x12,0x0C},
  {0x10,0x10,0x12,0x14,0x18,0x14,0x12},{0x0C,0x04,0x04,0x04,0x04,0x04,0x0E},
  {0x00,0x00,0x1A,0x15,0x15,0x15,0x15},{0x00,0x00,0x1E,0x11,0x11,0x11,0x11},
  {0x00,0x00,0x0E,0x11,0x11,0x11,0x0E},{0x00,0x00,0x1E,0x11,0x1E,0x10,0x10},
  {0x00,0x00,0x0F,0x11,0x0F,0x01,0x01},{0x00,0x00,0x16,0x19,0x10,0x10,0x10},
  {0x00,0x00,0x0F,0x10,0x0E,0x01,0x1E},{0x08,0x08,0x1E,0x08,0x08,0x08,0x06},
  {0x00,0x00,0x11,0x11,0x11,0x11,0x0F},{0x00,0x00,0x11,0x11,0x11,0x0A,0x04},
  {0x00,0x00,0x11,0x15,0x15,0x15,0x0A},{0x00,0x00,0x11,0x0A,0x04,0x0A,0x11},
  {0x00,0x00,0x11,0x11,0x0F,0x01,0x0E},{0x00,0x00,0x1F,0x02,0x04,0x08,0x1F},
  {0x02,0x04,0x04,0x08,0x04,0x04,0x02},{0x04,0x04,0x04,0x04,0x04,0x04,0x04},
  {0x08,0x04,0x04,0x02,0x04,0x04,0x08},{0x00,0x00,0x08,0x15,0x02,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

static void fb_draw_char(unsigned short* fb, int x, int y, char ch, unsigned short c) {
  if (ch < 32 || ch > 127) ch = '?';
  const unsigned char* g = kGlyph5x7[ch - 32];
  for (int row = 0; row < 7; ++row) {
    unsigned char bits = g[row];
    for (int col = 0; col < 5; ++col) {
      if (bits & (0x10 >> col)) {
        const int px = x + col, py = y + row;
        if (px >= 0 && px < 480 && py >= 0 && py < 272)
          fb[py * 512 + px] = c;
      }
    }
  }
}

static void fb_draw_text(unsigned short* fb, int x, int y, const char* text, unsigned short c) {
  int cx = x;
  for (const char* p = text; *p; ++p) {
    if (*p == '\n') { cx = x; y += 10; continue; }
    fb_draw_char(fb, cx + 1, y + 1, *p, 0); // 1px drop shadow (same as desktop)
    fb_draw_char(fb, cx, y, *p, c);
    cx += 6;
  }
}

static float stick_axis(unsigned char v) {
  const float f = (v - 128) / 128.f;
  if (f > -0.25f && f < 0.25f) return 0.f;
  return f;
}

// Nearest-neighbor blit RGBA → RGB565 fb with alpha (clipped loops for PSP CPU).
static void blit_sprite(unsigned short* fb,
                        int dx, int dy, int dw, int dh,
                        const unsigned char* rgba, int iw, int ih,
                        int sx, int sy, int sw, int sh,
                        float alpha) {
  if (!fb || !rgba || dw <= 0 || dh <= 0 || iw <= 0 || ih <= 0) return;
  if (sx < 0) { sx = 0; sy = 0; sw = iw; sh = ih; }
  if (sw <= 0 || sh <= 0) return;
  if (sx + sw > iw) sw = iw - sx;
  if (sy + sh > ih) sh = ih - sy;
  if (sw <= 0 || sh <= 0) return;
  if (alpha < 0.02f) return;
  if (alpha > 1.f) alpha = 1.f;
  const int a_mul = (int)(alpha * 256.f);

  int y0 = 0, y1 = dh, x0 = 0, x1 = dw;
  if (dy < 0) y0 = -dy;
  if (dy + dh > 272) y1 = 272 - dy;
  if (dx < 0) x0 = -dx;
  if (dx + dw > 480) x1 = 480 - dx;
  if (y0 >= y1 || x0 >= x1) return;

  for (int y = y0; y < y1; ++y) {
    const int py = dy + y;
    const int src_y = sy + (y * sh) / dh;
    unsigned short* dst_row = fb + py * 512 + dx;
    for (int x = x0; x < x1; ++x) {
      const int src_x = sx + (x * sw) / dw;
      const unsigned char* p = rgba + ((src_y * iw + src_x) * 4);
      int a = (p[3] * a_mul) >> 8;
      if (a < 8) continue;
      unsigned short* dst = &dst_row[x];
      if (a >= 250) {
        *dst = rgba8_to_565(p[0], p[1], p[2]);
      } else {
        const unsigned short bg = *dst;
        const int br = (bg >> 11) << 3;
        const int bg_ = ((bg >> 5) & 63) << 2;
        const int bb = (bg & 31) << 3;
        const int ia = 255 - a;
        *dst = rgba8_to_565((unsigned char)((p[0] * a + br * ia) / 255),
                            (unsigned char)((p[1] * a + bg_ * ia) / 255),
                            (unsigned char)((p[2] * a + bb * ia) / 255));
      }
    }
  }
}

struct PakEntry {
  char name[48];
  unsigned offset;
  unsigned size;
};

struct TexCache {
  char name[48];
  unsigned char* rgba;
  int w, h;
  unsigned tick;
  bool missing;
};

static const int kMaxCache = 96;
static const unsigned kMaxCachePixels = 2 * 1024 * 1024; // ~8MB RGBA — fewer evict flickers

} // namespace

class PspPlatform : public IPlatform {
public:
  PspPlatform()
    : fb_(0), prev_buttons_(0), boot_tick_(0), inited_(false),
      pak_data_(0), pak_size_(0), pak_entries_(0), pak_count_(0),
      cache_count_(0), cache_pixels_(0), cache_tick_(1) {
    images_dir_[0] = 0;
    for (int i = 0; i < kMaxCache; ++i) {
      cache_[i].rgba = 0;
      cache_[i].name[0] = 0;
      cache_[i].missing = false;
      cache_[i].w = cache_[i].h = 0;
      cache_[i].tick = 0;
    }
  }

  ~PspPlatform() { shutdown(); }

  bool init(const char* title, int width, int height) {
    (void)title; (void)width; (void)height;
    scePowerSetClockFrequency(333, 333, 166);
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    fb_ = (unsigned short*)memalign(16, 512 * 272 * sizeof(unsigned short));
    if (!fb_) return false;
    std::memset(fb_, 0, 512 * 272 * sizeof(unsigned short));

    sceGuInit();
    sceGuStart(GU_DIRECT, s_gu_list);
    sceGuDrawBuffer(GU_PSM_5650, fb_, 512);
    sceGuDispBuffer(480, 272, fb_, 512);
    sceGuOffset(2048 - (480 / 2), 2048 - (272 / 2));
    sceGuViewport(2048, 2048, 480, 272);
    sceGuScissor(0, 0, 480, 272);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuFinish();
    sceGuSync(0, 0);

    sceDisplaySetMode(0, 480, 272);
    sceDisplaySetFrameBuf(fb_, 512, PSP_DISPLAY_PIXEL_FORMAT_565, PSP_DISPLAY_SETBUF_IMMEDIATE);
    sceDisplayWaitVblankStart();

    u64 t = 0;
    sceRtcGetCurrentTick(&t);
    boot_tick_ = t;
    inited_ = true;
    return true;
  }

  void shutdown() {
    clear_textures();
    close_pak();
    if (inited_) { sceGuTerm(); inited_ = false; }
    if (fb_) { std::free(fb_); fb_ = 0; }
  }

  bool load_sprites(const char* images_dir) {
    clear_textures();
    close_pak();
    images_dir_[0] = 0;
    if (!images_dir || !images_dir[0]) return false;

    // Path may be a .pak file or a directory (we also try dir/sprites.pak).
    char path[512];
    std::strncpy(path, images_dir, sizeof(path) - 1);
    path[sizeof(path) - 1] = 0;

    const char* ext = std::strrchr(path, '.');
    if (ext && (std::strcmp(ext, ".pak") == 0 || std::strcmp(ext, ".PAK") == 0)) {
      if (open_pak(path) && find_pak("player_skin_def_1-sheet0")) {
        std::strncpy(images_dir_, path, sizeof(images_dir_) - 1);
        log_io("pak_ok", path);
        return true;
      }
      log_io("pak_fail", path);
      return false;
    }

    // Directory mode: prefer packed sprites.pak inside it.
    char pak[540];
    std::snprintf(pak, sizeof(pak), "%s/sprites.pak", path);
    if (open_pak(pak) && find_pak("player_skin_def_1-sheet0")) {
      std::strncpy(images_dir_, pak, sizeof(images_dir_) - 1);
      log_io("pak_ok", pak);
      return true;
    }

    // Loose PNGs (slower; avoid shipping 1000 files if possible).
    std::strncpy(images_dir_, path, sizeof(images_dir_) - 1);
    char probe[560];
    std::snprintf(probe, sizeof(probe), "%s/player_skin_def_1-sheet0.png", images_dir_);
    SceUID fd = sceIoOpen(probe, PSP_O_RDONLY, 0777);
    if (fd < 0) {
      char alt[600];
      std::snprintf(alt, sizeof(alt), "ms0:/PSP/GAME/MINIDAYZ/%s", probe);
      fd = sceIoOpen(alt, PSP_O_RDONLY, 0777);
    }
    if (fd < 0) { images_dir_[0] = 0; log_io("png_fail", probe); return false; }
    sceIoClose(fd);
    log_io("png_ok", probe);
    return true;
  }

  bool has_sprite(const char* name) const {
    return const_cast<PspPlatform*>(this)->texture_of(name) != 0;
  }

  void draw_sprite(const char* name, float x, float y, float w, float h,
                   float angle_deg, float alpha) {
    draw_sprite_frame(name, x, y, w, h, -1, -1, -1, -1, angle_deg, alpha);
  }

  void draw_sprite_frame(const char* name,
                         float x, float y, float w, float h,
                         int sx, int sy, int sw, int sh,
                         float /*angle_deg*/, float alpha) {
    TexCache* t = texture_of(name);
    if (!t || !t->rgba || w <= 1.e-3f || h <= 1.e-3f) return;
    blit_sprite(fb_, (int)x, (int)y, (int)(w + 0.5f), (int)(h + 0.5f),
                t->rgba, t->w, t->h, sx, sy, sw, sh, alpha);
  }

  bool poll(InputState& out) {
    out = InputState();
    SceCtrlData pad;
    sceCtrlReadBufferPositive(&pad, 1);
    const unsigned int b = pad.Buttons;
    const unsigned int pressed = b & ~prev_buttons_;
    prev_buttons_ = b;

    const bool l = (b & PSP_CTRL_LTRIGGER) != 0;
    const bool r = (b & PSP_CTRL_RTRIGGER) != 0;

    float mx = 0.f, my = 0.f;
    if (!l) {
      if (b & PSP_CTRL_LEFT) mx -= 1.f;
      if (b & PSP_CTRL_RIGHT) mx += 1.f;
      if (b & PSP_CTRL_UP) my -= 1.f;
      if (b & PSP_CTRL_DOWN) my += 1.f;
    }
    mx += stick_axis(pad.Lx);
    my += stick_axis(pad.Ly);
    if (mx > 1.f) mx = 1.f;
    if (mx < -1.f) mx = -1.f;
    if (my > 1.f) my = 1.f;
    if (my < -1.f) my = -1.f;
    out.move_x = mx;
    out.move_y = my;

    out.run = r && !(pressed & (PSP_CTRL_CROSS | PSP_CTRL_CIRCLE | PSP_CTRL_TRIANGLE | PSP_CTRL_SQUARE));
    out.crouch = l && !(pressed & (PSP_CTRL_LEFT | PSP_CTRL_RIGHT | PSP_CTRL_START | PSP_CTRL_SELECT |
                                   PSP_CTRL_CROSS | PSP_CTRL_CIRCLE | PSP_CTRL_TRIANGLE | PSP_CTRL_SQUARE));

    out.attack = (b & PSP_CTRL_CROSS) != 0 && !r && !l;
    out.attack_pressed = (pressed & PSP_CTRL_CROSS) != 0 && !r && !l;
    out.interact = (b & PSP_CTRL_CIRCLE) != 0 && !r && !l;
    out.interact_pressed = (pressed & PSP_CTRL_CIRCLE) != 0 && !r && !l;
    out.inventory = (b & PSP_CTRL_TRIANGLE) != 0 && !r && !l;
    out.inventory_pressed = (pressed & PSP_CTRL_TRIANGLE) != 0 && !r && !l;

    if (r && (pressed & PSP_CTRL_SQUARE)) {
      out.craft_pressed = true;
    } else if (!r && !l && (pressed & PSP_CTRL_SQUARE)) {
      out.mag_reload_pressed = true;
      out.reload_pressed = true;
      out.reload = true;
    } else if (!r && !l && (b & PSP_CTRL_SQUARE)) {
      out.reload = true;
    }

    if (r && (pressed & PSP_CTRL_CROSS)) out.throw_pressed = true;
    if (r && (pressed & PSP_CTRL_CIRCLE)) out.drop_pressed = true;
    if (r && (pressed & PSP_CTRL_TRIANGLE)) out.torch_pressed = true;

    out.confirm = (b & PSP_CTRL_CROSS) != 0 && !l;
    out.confirm_pressed = (pressed & PSP_CTRL_CROSS) != 0 && !l;
    out.cancel = (b & PSP_CTRL_SELECT) != 0 && !l;
    // PSP UI: Circle = back in menus; Select = open/close inventory.
    out.back_pressed = (pressed & PSP_CTRL_CIRCLE) != 0 && !l && !r;
    if (!l && (pressed & PSP_CTRL_SELECT)) out.bag_pressed = true;
    out.pause = (b & PSP_CTRL_START) != 0 && !l;
    out.pause_pressed = (pressed & PSP_CTRL_START) != 0 && !l;

    if (l && (pressed & PSP_CTRL_START)) out.save_pressed = true;
    if (l && (pressed & PSP_CTRL_SELECT)) out.load_pressed = true;

    if (l && (pressed & PSP_CTRL_CIRCLE)) {
      out.bag_pressed = true;
      out.bag = true;
    }
    if (l && (b & PSP_CTRL_CIRCLE)) out.bag = true;

    if (l && (pressed & PSP_CTRL_LEFT)) out.equip_delta = -1;
    if (l && (pressed & PSP_CTRL_RIGHT)) out.equip_delta = 1;

    if (l && (pressed & PSP_CTRL_CROSS)) out.slot_digit = 1;
    if (l && (pressed & PSP_CTRL_TRIANGLE)) out.slot_digit = 2;
    if (l && (pressed & PSP_CTRL_SQUARE)) out.slot_digit = 3;
    return true;
  }

  double time_seconds() {
    u64 t = 0;
    sceRtcGetCurrentTick(&t);
    const u32 res = sceRtcGetTickResolution();
    if (res == 0) return 0.0;
    return (double)(t - boot_tick_) / (double)res;
  }

  void begin_frame() {
    // Wait blanking, then paint. No separate memset — clear() fills the frame.
    if (inited_) sceDisplayWaitVblankStart();
  }

  void clear(float r, float g, float b) {
    fb_fill(fb_, 0, 0, 480, 272, rgba565(r, g, b, 1.f));
  }

  void draw_rect(float x, float y, float w, float h, float r, float g, float b, float a) {
    if (a < 0.02f) return;
    fb_fill_blend(fb_, (int)x, (int)y, (int)(w + 0.5f), (int)(h + 0.5f), rgba565(r, g, b, 1.f),
                  (int)(a * 255.f));
  }

  void draw_text(float x, float y, const char* text, float r, float g, float b) {
    if (!text) return;
    fb_draw_text(fb_, (int)x, (int)y, text, rgba565(r, g, b, 1.f));
  }

  void end_frame() {
    if (!fb_ || !inited_) return;
    sceKernelDcacheWritebackAll();
    sceDisplaySetFrameBuf(fb_, 512, PSP_DISPLAY_PIXEL_FORMAT_565, PSP_DISPLAY_SETBUF_NEXTFRAME);
  }

  bool read_file(const char* path, char** out_data, std::size_t* out_size) {
    if (!path || !out_data || !out_size) return false;
    *out_data = 0;
    *out_size = 0;

    // Prefer sceIoOpen — newlib fopen often ignores sceIoChdir / ms0 on PPSSPP.
    static const char* kPrefix[] = {
      "",
      "ms0:/PSP/GAME/MINIDAYZ/",
      "ms0:/PSP/GAME/minidayz/",
      "ms0:/PSP/GAME/MiniDAYZ/",
      0
    };
    const bool abs = (std::strncmp(path, "ms0:", 4) == 0 ||
                      std::strncmp(path, "host0:", 6) == 0 ||
                      std::strncmp(path, "disc0:", 6) == 0 ||
                      std::strncmp(path, "umd0:", 5) == 0);

    SceUID fd = -1;
    char tried[512];
    tried[0] = 0;
    for (int i = 0; kPrefix[i]; ++i) {
      if (abs && i > 0) break;
      if (i == 0) {
        std::strncpy(tried, path, sizeof(tried) - 1);
      } else {
        std::snprintf(tried, sizeof(tried), "%s%s", kPrefix[i], path);
      }
      tried[sizeof(tried) - 1] = 0;
      fd = sceIoOpen(tried, PSP_O_RDONLY, 0777);
      if (fd >= 0) break;
    }
    if (fd < 0) {
      log_io("rf_fail", path);
      return false;
    }

    const SceOff sz = sceIoLseek(fd, 0, PSP_SEEK_END);
    sceIoLseek(fd, 0, PSP_SEEK_SET);
    if (sz <= 0 || sz > 12 * 1024 * 1024) {
      sceIoClose(fd);
      return false;
    }
    char* buf = (char*)std::malloc((std::size_t)sz + 1);
    if (!buf) { sceIoClose(fd); return false; }
    const int n = sceIoRead(fd, buf, (int)sz);
    sceIoClose(fd);
    if (n <= 0) { std::free(buf); return false; }
    buf[n] = 0;
    *out_data = buf;
    *out_size = (std::size_t)n;
    return true;
  }

  bool write_file(const char* path, const void* data, std::size_t size) {
    if (!path || !data) return false;
    SceUID fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (fd < 0) {
      char alt[512];
      std::snprintf(alt, sizeof(alt), "ms0:/PSP/GAME/MINIDAYZ/%s", path);
      fd = sceIoOpen(alt, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    }
    if (fd < 0) return false;
    const int n = sceIoWrite(fd, data, (int)size);
    sceIoClose(fd);
    return n == (int)size;
  }

  int screen_w() const { return kPspWidth; }
  int screen_h() const { return kPspHeight; }

private:
  void log_io(const char* tag, const char* detail) {
    // Append one line to memstick log so PPSSPP runs are diagnosable.
    char line[640];
    std::snprintf(line, sizeof(line), "%s %s\n", tag ? tag : "?", detail ? detail : "");
    SceUID fd = sceIoOpen("ms0:/PSP/GAME/MINIDAYZ/mdaz_io.log",
                          PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
    if (fd < 0)
      fd = sceIoOpen("mdaz_io.log", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
    if (fd < 0) return;
    sceIoWrite(fd, line, (int)std::strlen(line));
    sceIoClose(fd);
  }

  void close_pak() {
    if (pak_entries_) { std::free(pak_entries_); pak_entries_ = 0; }
    if (pak_data_) { std::free(pak_data_); pak_data_ = 0; }
    pak_size_ = 0;
    pak_count_ = 0;
  }

  bool open_pak(const char* path) {
    char* data = 0;
    std::size_t size = 0;
    if (!read_file(path, &data, &size) || !data || size < 16) {
      if (data) std::free(data);
      return false;
    }
    if (std::memcmp(data, "MDZSPR01", 8) != 0) {
      std::free(data);
      return false;
    }
    const unsigned count = (unsigned)(unsigned char)data[8]
      | ((unsigned)(unsigned char)data[9] << 8)
      | ((unsigned)(unsigned char)data[10] << 16)
      | ((unsigned)(unsigned char)data[11] << 24);
    const std::size_t need = 16 + (std::size_t)count * 56;
    if (count == 0 || need > size) {
      std::free(data);
      return false;
    }
    PakEntry* ents = (PakEntry*)std::malloc(sizeof(PakEntry) * count);
    if (!ents) { std::free(data); return false; }
    for (unsigned i = 0; i < count; ++i) {
      const char* e = data + 16 + i * 56;
      std::memcpy(ents[i].name, e, 48);
      ents[i].name[47] = 0;
      ents[i].offset = (unsigned)(unsigned char)e[48]
        | ((unsigned)(unsigned char)e[49] << 8)
        | ((unsigned)(unsigned char)e[50] << 16)
        | ((unsigned)(unsigned char)e[51] << 24);
      ents[i].size = (unsigned)(unsigned char)e[52]
        | ((unsigned)(unsigned char)e[53] << 8)
        | ((unsigned)(unsigned char)e[54] << 16)
        | ((unsigned)(unsigned char)e[55] << 24);
    }
    pak_data_ = data;
    pak_size_ = size;
    pak_entries_ = ents;
    pak_count_ = count;
    return true;
  }

  const PakEntry* find_pak(const char* name) const {
    if (!pak_entries_ || !name) return 0;
    for (unsigned i = 0; i < pak_count_; ++i)
      if (std::strcmp(pak_entries_[i].name, name) == 0) return &pak_entries_[i];
    return 0;
  }

  void clear_textures() {
    for (int i = 0; i < kMaxCache; ++i) {
      if (cache_[i].rgba) { stbi_image_free(cache_[i].rgba); cache_[i].rgba = 0; }
      cache_[i].name[0] = 0;
      cache_[i].missing = false;
      cache_[i].w = cache_[i].h = 0;
      cache_[i].tick = 0;
    }
    cache_count_ = 0;
    cache_pixels_ = 0;
  }

  void evict_one() {
    int victim = -1;
    unsigned best = 0xffffffffu;
    for (int i = 0; i < kMaxCache; ++i) {
      if (!cache_[i].rgba) continue;
      if (cache_[i].tick < best) { best = cache_[i].tick; victim = i; }
    }
    if (victim < 0) return;
    cache_pixels_ -= (unsigned)(cache_[victim].w * cache_[victim].h);
    stbi_image_free(cache_[victim].rgba);
    cache_[victim].rgba = 0;
    cache_[victim].name[0] = 0;
    cache_[victim].missing = false;
    if (cache_count_ > 0) --cache_count_;
  }

  TexCache* texture_of(const char* name) {
    if (!name || !name[0] || !images_dir_[0]) return 0;
    char key[48];
    if (std::strstr(name, ".png")) {
      std::strncpy(key, name, sizeof(key) - 1);
      // strip .png
      char* dot = std::strrchr(key, '.');
      if (dot) *dot = 0;
    } else {
      std::strncpy(key, name, sizeof(key) - 1);
    }
    key[sizeof(key) - 1] = 0;

    for (int i = 0; i < kMaxCache; ++i) {
      if (cache_[i].name[0] && std::strcmp(cache_[i].name, key) == 0) {
        if (cache_[i].missing) return 0;
        cache_[i].tick = cache_tick_++;
        return &cache_[i];
      }
    }

    // Decode PNG from pak or loose file.
    const unsigned char* png = 0;
    int png_sz = 0;
    char* loose = 0;
    std::size_t loose_sz = 0;

    const PakEntry* pe = find_pak(key);
    // Common C2 alias: "foo" ↔ "foo-sheet0"
    if (!pe && pak_data_) {
      char alt[56];
      if (!std::strstr(key, "-sheet")) {
        std::snprintf(alt, sizeof(alt), "%s-sheet0", key);
        pe = find_pak(alt);
      } else if (std::strlen(key) > 7) {
        std::strncpy(alt, key, sizeof(alt) - 1);
        alt[sizeof(alt) - 1] = 0;
        char* s = std::strstr(alt, "-sheet");
        if (s) { *s = 0; pe = find_pak(alt); }
      }
    }
    if (pe && pak_data_ && pe->offset + pe->size <= pak_size_) {
      png = (const unsigned char*)(pak_data_ + pe->offset);
      png_sz = (int)pe->size;
    } else if (!pak_data_) {
      char path[600];
      std::snprintf(path, sizeof(path), "%s/%s.png", images_dir_, key);
      if (!read_file(path, &loose, &loose_sz) || !loose) {
        // mark missing slot
        int slot = alloc_cache_slot();
        if (slot >= 0) {
          std::strncpy(cache_[slot].name, key, sizeof(cache_[slot].name) - 1);
          cache_[slot].missing = true;
        }
        return 0;
      }
      png = (const unsigned char*)loose;
      png_sz = (int)loose_sz;
    } else {
      int slot = alloc_cache_slot();
      if (slot >= 0) {
        std::strncpy(cache_[slot].name, key, sizeof(cache_[slot].name) - 1);
        cache_[slot].missing = true;
      }
      return 0;
    }

    int w = 0, h = 0, comp = 0;
    unsigned char* pixels = stbi_load_from_memory(png, png_sz, &w, &h, &comp, 4);
    if (loose) std::free(loose);
    if (!pixels || w <= 0 || h <= 0) {
      if (pixels) stbi_image_free(pixels);
      int slot = alloc_cache_slot();
      if (slot >= 0) {
        std::strncpy(cache_[slot].name, key, sizeof(cache_[slot].name) - 1);
        cache_[slot].missing = true;
      }
      return 0;
    }

    const unsigned need = (unsigned)(w * h);
    while (cache_count_ >= kMaxCache || cache_pixels_ + need > kMaxCachePixels)
      evict_one();

    int slot = alloc_cache_slot();
    if (slot < 0) {
      stbi_image_free(pixels);
      return 0;
    }
    std::strncpy(cache_[slot].name, key, sizeof(cache_[slot].name) - 1);
    cache_[slot].rgba = pixels;
    cache_[slot].w = w;
    cache_[slot].h = h;
    cache_[slot].missing = false;
    cache_[slot].tick = cache_tick_++;
    cache_pixels_ += need;
    ++cache_count_;
    return &cache_[slot];
  }

  int alloc_cache_slot() {
    for (int i = 0; i < kMaxCache; ++i)
      if (!cache_[i].rgba && !cache_[i].name[0]) return i;
    evict_one();
    for (int i = 0; i < kMaxCache; ++i)
      if (!cache_[i].rgba) {
        cache_[i].name[0] = 0;
        cache_[i].missing = false;
        return i;
      }
    return -1;
  }

  unsigned short* fb_;
  unsigned int prev_buttons_;
  u64 boot_tick_;
  bool inited_;

  char images_dir_[512];
  char* pak_data_;
  std::size_t pak_size_;
  PakEntry* pak_entries_;
  unsigned pak_count_;

  TexCache cache_[kMaxCache];
  int cache_count_;
  unsigned cache_pixels_;
  unsigned cache_tick_;
};

IPlatform* create_platform() { return new PspPlatform(); }

#endif

} // namespace mdaz
