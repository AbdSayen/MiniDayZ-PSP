#include "mdaz/platform.hpp"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>

// When compiled without PSPSDK headers, provide a stub create_platform
// so the file remains readable. Real implementation uses sceGu / sceCtrl.

#if defined(MDAZ_PSP) && defined(__PSP__)
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspge.h>
#include <pspkernel.h>
#include <psppower.h>
#include <psprtc.h>
#include <malloc.h>
#include <stdint.h>
#endif

namespace mdaz {

#if !(defined(MDAZ_PSP) && defined(__PSP__))

IPlatform* create_platform() {
  std::fprintf(stderr, "platform_psp.cpp: not building with PSPSDK; stub.\n");
  return 0;
}

#else

namespace {

// Soft 565 framebuffer (pitch 512). Displayed via sceDisplaySetFrameBuf.
static unsigned int __attribute__((aligned(16))) s_gu_list[262144];

static unsigned short rgba565(float r, float g, float b, float a) {
  if (a < 0.02f) return 0;
  int ri = (int)(r * 31.f + 0.5f); if (ri < 0) ri = 0; if (ri > 31) ri = 31;
  int gi = (int)(g * 63.f + 0.5f); if (gi < 0) gi = 0; if (gi > 63) gi = 63;
  int bi = (int)(b * 31.f + 0.5f); if (bi < 0) bi = 0; if (bi > 31) bi = 31;
  return (unsigned short)((ri << 11) | (gi << 5) | bi);
}

static void fb_fill(unsigned short* fb, int x, int y, int w, int h, unsigned short c) {
  if (!fb || w <= 0 || h <= 0) return;
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > 480) w = 480 - x;
  if (y + h > 272) h = 272 - y;
  if (w <= 0 || h <= 0) return;
  for (int row = 0; row < h; ++row) {
    unsigned short* dst = fb + (y + row) * 512 + x; // GU pitch 512
    for (int col = 0; col < w; ++col) dst[col] = c;
  }
}

// 5×7 glyphs (same idea as desktop fallback).
static const unsigned char kGlyph5x7[96][7] = {
  {0,0,0,0,0,0,0}, // space
  {0x04,0x04,0x04,0x04,0x00,0x04,0x00}, // !
  {0x0A,0x0A,0x00,0x00,0x00,0x00,0x00},
  {0x0A,0x1F,0x0A,0x1F,0x0A,0x00,0x00},
  {0x04,0x0F,0x14,0x0E,0x05,0x1E,0x04},
  {0x18,0x19,0x02,0x04,0x08,0x13,0x03},
  {0x08,0x14,0x14,0x08,0x15,0x12,0x0D},
  {0x04,0x04,0x00,0x00,0x00,0x00,0x00},
  {0x02,0x04,0x04,0x04,0x04,0x04,0x02},
  {0x08,0x04,0x04,0x04,0x04,0x04,0x08},
  {0x00,0x0A,0x04,0x1F,0x04,0x0A,0x00},
  {0x00,0x04,0x04,0x1F,0x04,0x04,0x00},
  {0x00,0x00,0x00,0x00,0x04,0x04,0x08},
  {0x00,0x00,0x00,0x1F,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x04,0x00},
  {0x01,0x02,0x04,0x08,0x10,0x00,0x00},
  {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, // 0
  {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
  {0x0E,0x11,0x01,0x06,0x08,0x10,0x1F},
  {0x0E,0x11,0x01,0x06,0x01,0x11,0x0E},
  {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
  {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
  {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},
  {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
  {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
  {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},
  {0x00,0x04,0x00,0x00,0x00,0x04,0x00},
  {0x00,0x04,0x00,0x00,0x04,0x04,0x08},
  {0x02,0x04,0x08,0x10,0x08,0x04,0x02},
  {0x00,0x00,0x1F,0x00,0x1F,0x00,0x00},
  {0x08,0x04,0x02,0x01,0x02,0x04,0x08},
  {0x0E,0x11,0x01,0x06,0x04,0x00,0x04},
  {0x0E,0x11,0x17,0x15,0x17,0x10,0x0E},
  {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}, // A
  {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},
  {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},
  {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E},
  {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},
  {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
  {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F},
  {0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
  {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},
  {0x01,0x01,0x01,0x01,0x11,0x11,0x0E},
  {0x11,0x12,0x14,0x18,0x14,0x12,0x11},
  {0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
  {0x11,0x1B,0x15,0x11,0x11,0x11,0x11},
  {0x11,0x19,0x15,0x13,0x11,0x11,0x11},
  {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},
  {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},
  {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D},
  {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
  {0x0E,0x11,0x10,0x0E,0x01,0x11,0x0E},
  {0x1F,0x04,0x04,0x04,0x04,0x04,0x04},
  {0x11,0x11,0x11,0x11,0x11,0x11,0x0E},
  {0x11,0x11,0x11,0x11,0x11,0x0A,0x04},
  {0x11,0x11,0x11,0x15,0x15,0x1B,0x11},
  {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
  {0x11,0x11,0x0A,0x04,0x04,0x04,0x04},
  {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F},
  {0x0E,0x08,0x08,0x08,0x08,0x08,0x0E},
  {0x10,0x08,0x04,0x02,0x01,0x00,0x00},
  {0x0E,0x02,0x02,0x02,0x02,0x02,0x0E},
  {0x04,0x0A,0x11,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00,0x1F},
  {0x08,0x04,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x0E,0x01,0x0F,0x11,0x0F}, // a
  {0x10,0x10,0x1E,0x11,0x11,0x11,0x1E},
  {0x00,0x00,0x0E,0x11,0x10,0x11,0x0E},
  {0x01,0x01,0x0F,0x11,0x11,0x11,0x0F},
  {0x00,0x00,0x0E,0x11,0x1F,0x10,0x0E},
  {0x06,0x08,0x08,0x1E,0x08,0x08,0x08},
  {0x00,0x00,0x0F,0x11,0x0F,0x01,0x0E},
  {0x10,0x10,0x1E,0x11,0x11,0x11,0x11},
  {0x04,0x00,0x0C,0x04,0x04,0x04,0x0E},
  {0x02,0x00,0x06,0x02,0x02,0x12,0x0C},
  {0x10,0x10,0x12,0x14,0x18,0x14,0x12},
  {0x0C,0x04,0x04,0x04,0x04,0x04,0x0E},
  {0x00,0x00,0x1A,0x15,0x15,0x15,0x15},
  {0x00,0x00,0x1E,0x11,0x11,0x11,0x11},
  {0x00,0x00,0x0E,0x11,0x11,0x11,0x0E},
  {0x00,0x00,0x1E,0x11,0x1E,0x10,0x10},
  {0x00,0x00,0x0F,0x11,0x0F,0x01,0x01},
  {0x00,0x00,0x16,0x19,0x10,0x10,0x10},
  {0x00,0x00,0x0F,0x10,0x0E,0x01,0x1E},
  {0x08,0x08,0x1E,0x08,0x08,0x08,0x06},
  {0x00,0x00,0x11,0x11,0x11,0x11,0x0F},
  {0x00,0x00,0x11,0x11,0x11,0x0A,0x04},
  {0x00,0x00,0x11,0x15,0x15,0x15,0x0A},
  {0x00,0x00,0x11,0x0A,0x04,0x0A,0x11},
  {0x00,0x00,0x11,0x11,0x0F,0x01,0x0E},
  {0x00,0x00,0x1F,0x02,0x04,0x08,0x1F},
  {0x02,0x04,0x04,0x08,0x04,0x04,0x02},
  {0x04,0x04,0x04,0x04,0x04,0x04,0x04},
  {0x08,0x04,0x04,0x02,0x04,0x04,0x08},
  {0x00,0x00,0x08,0x15,0x02,0x00,0x00},
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
    if (*p == '\n') { cx = x; y += 8; continue; }
    fb_draw_char(fb, cx, y, *p, c);
    cx += 6;
  }
}

static float stick_axis(unsigned char v) {
  const float f = (v - 128) / 128.f;
  if (f > -0.25f && f < 0.25f) return 0.f;
  return f;
}

} // namespace

// ---------------------------------------------------------------------------
// PspPlatform — full InputState map + GU framebuffer blit
// ---------------------------------------------------------------------------
class PspPlatform : public IPlatform {
public:
  PspPlatform()
    : fb_(0), prev_buttons_(0), boot_tick_(0), inited_(false) {}

  bool init(const char* title, int width, int height) {
    (void)title;
    (void)width;
    (void)height;
    scePowerSetClockFrequency(333, 333, 166);
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    fb_ = (unsigned short*)memalign(16, 512 * 272 * sizeof(unsigned short));
    if (!fb_) return false;
    std::memset(fb_, 0, 512 * 272 * sizeof(unsigned short));

    // Light GU init so future sprite blits can share the list; display uses framebuf.
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
    if (inited_) {
      sceGuTerm();
      inited_ = false;
    }
    if (fb_) { std::free(fb_); fb_ = 0; }
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

    // Movement — D-pad + analog (suppressed on axis when L cycles hotbar).
    float mx = 0.f, my = 0.f;
    if (!l) {
      if (b & PSP_CTRL_LEFT) mx -= 1.f;
      if (b & PSP_CTRL_RIGHT) mx += 1.f;
      if (b & PSP_CTRL_UP) my -= 1.f;
      if (b & PSP_CTRL_DOWN) my += 1.f;
    }
    mx += stick_axis(pad.Lx);
    my += stick_axis(pad.Ly);
    if (mx > 1.f) mx = 1.f; if (mx < -1.f) mx = -1.f;
    if (my > 1.f) my = 1.f; if (my < -1.f) my = -1.f;
    out.move_x = mx;
    out.move_y = my;

    out.run = r && !(pressed & (PSP_CTRL_CROSS | PSP_CTRL_CIRCLE | PSP_CTRL_TRIANGLE | PSP_CTRL_SQUARE));
    // Crouch: L held without edge combos (modifier still used for L+*).
    out.crouch = l && !(pressed & (PSP_CTRL_LEFT | PSP_CTRL_RIGHT | PSP_CTRL_START | PSP_CTRL_SELECT |
                                   PSP_CTRL_CROSS | PSP_CTRL_CIRCLE | PSP_CTRL_TRIANGLE | PSP_CTRL_SQUARE));

    // Primary face (held + edges). L = modifier (hotbar / save / difficulty).
    out.attack = (b & PSP_CTRL_CROSS) != 0 && !r && !l;
    out.attack_pressed = (pressed & PSP_CTRL_CROSS) != 0 && !r && !l;
    out.interact = (b & PSP_CTRL_CIRCLE) != 0 && !r && !l;
    out.interact_pressed = (pressed & PSP_CTRL_CIRCLE) != 0 && !r && !l;
    out.inventory = (b & PSP_CTRL_TRIANGLE) != 0 && !r && !l;
    out.inventory_pressed = (pressed & PSP_CTRL_TRIANGLE) != 0 && !r && !l;

    // Square alone / with R:
    //   Square       → mag reload (V)
    //   R+Square     → craft menu (B)
    //   (legacy) bare Square edge also sets reload_pressed for craft-open path
    if (r && (pressed & PSP_CTRL_SQUARE)) {
      out.craft_pressed = true;
    } else if (!r && !l && (pressed & PSP_CTRL_SQUARE)) {
      out.mag_reload_pressed = true;
      out.reload_pressed = true; // opens craft if menu closed (desktop R)
      out.reload = true;
    } else if (!r && !l && (b & PSP_CTRL_SQUARE)) {
      out.reload = true;
    }

    // R + face combos (secondary actions).
    if (r && (pressed & PSP_CTRL_CROSS)) out.throw_pressed = true;
    if (r && (pressed & PSP_CTRL_CIRCLE)) out.drop_pressed = true;
    if (r && (pressed & PSP_CTRL_TRIANGLE)) out.torch_pressed = true;

    // Confirm / cancel / pause (L suppresses confirm so L+× can set difficulty).
    out.confirm = (b & PSP_CTRL_CROSS) != 0 && !l;
    out.confirm_pressed = (pressed & PSP_CTRL_CROSS) != 0 && !l;
    out.cancel = (b & PSP_CTRL_SELECT) != 0 && !l;
    out.pause = (b & PSP_CTRL_START) != 0 && !l;
    out.pause_pressed = (pressed & PSP_CTRL_START) != 0 && !l;

    // L + Start / Select = save / load.
    if (l && (pressed & PSP_CTRL_START)) out.save_pressed = true;
    if (l && (pressed & PSP_CTRL_SELECT)) out.load_pressed = true;

    // Bag (Tab): L + Circle — avoids Select+cancel race that would close the bag.
    if (l && (pressed & PSP_CTRL_CIRCLE)) {
      out.bag_pressed = true;
      out.bag = true;
    }
    if (l && (b & PSP_CTRL_CIRCLE)) out.bag = true;

    // Hotbar cycle: L + Left/Right.
    if (l && (pressed & PSP_CTRL_LEFT)) out.equip_delta = -1;
    if (l && (pressed & PSP_CTRL_RIGHT)) out.equip_delta = 1;

    // Menu difficulty: L + ×/△/□ → Novice / Regular / Veteran (Circle reserved for bag).
    if (l && (pressed & PSP_CTRL_CROSS)) out.slot_digit = 1;
    if (l && (pressed & PSP_CTRL_TRIANGLE)) out.slot_digit = 2;
    if (l && (pressed & PSP_CTRL_SQUARE)) out.slot_digit = 3;

    // Home / Select→menu handled by cancel; HOME via kernel exit callback.
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
    if (fb_) std::memset(fb_, 0, 512 * 272 * sizeof(unsigned short));
  }

  void clear(float r, float g, float b) {
    fb_fill(fb_, 0, 0, 480, 272, rgba565(r, g, b, 1.f));
  }

  void draw_rect(float x, float y, float w, float h, float r, float g, float b, float a) {
    if (a < 0.05f) return;
    fb_fill(fb_, (int)x, (int)y, (int)(w + 0.5f), (int)(h + 0.5f), rgba565(r, g, b, a));
  }

  void draw_text(float x, float y, const char* text, float r, float g, float b) {
    if (!text) return;
    fb_draw_text(fb_, (int)x, (int)y, text, rgba565(r, g, b, 1.f));
  }

  void end_frame() {
    if (!fb_ || !inited_) return;
    sceKernelDcacheWritebackAll();
    sceDisplayWaitVblankStart();
    sceDisplaySetFrameBuf(fb_, 512, PSP_DISPLAY_PIXEL_FORMAT_565, PSP_DISPLAY_SETBUF_NEXTFRAME);
  }

  bool read_file(const char* path, char** out_data, std::size_t* out_size) {
    if (!path || !out_data || !out_size) return false;
    *out_data = 0;
    *out_size = 0;
    FILE* f = std::fopen(path, "rb");
    if (!f) {
      // Try under ms0:/PSP/GAME/MINIDAYZ/
      char alt[512];
      std::snprintf(alt, sizeof(alt), "ms0:/PSP/GAME/MINIDAYZ/%s", path);
      f = std::fopen(alt, "rb");
    }
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (sz <= 0) { std::fclose(f); return false; }
    char* buf = (char*)std::malloc((std::size_t)sz + 1);
    if (!buf) { std::fclose(f); return false; }
    const std::size_t n = std::fread(buf, 1, (std::size_t)sz, f);
    std::fclose(f);
    buf[n] = 0;
    *out_data = buf;
    *out_size = n;
    return true;
  }

  bool write_file(const char* path, const void* data, std::size_t size) {
    if (!path || !data) return false;
    FILE* f = std::fopen(path, "wb");
    if (!f) {
      char alt[512];
      std::snprintf(alt, sizeof(alt), "ms0:/PSP/GAME/MINIDAYZ/%s", path);
      f = std::fopen(alt, "wb");
    }
    if (!f) return false;
    const std::size_t n = std::fwrite(data, 1, size, f);
    std::fclose(f);
    return n == size;
  }

  int screen_w() const { return kPspWidth; }
  int screen_h() const { return kPspHeight; }

private:
  unsigned short* fb_;
  unsigned int prev_buttons_;
  u64 boot_tick_;
  bool inited_;
};

IPlatform* create_platform() { return new PspPlatform(); }

#endif

} // namespace mdaz
