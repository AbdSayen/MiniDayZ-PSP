// PSP platform: hardware GU renderer (2026-10-19 perf pass).
//  * Sprites / tiles / rects / text are GE draws (GU_SPRITES in through mode), batched per
//    texture; no CPU pixel blits. Textures live in RAM in their final GE format
//    (T8 + CLUT8888 lossless, or 5551 / 4444) from sprites.pak MDZSPR02; sheets wider/taller
//    than 512 are addressed as <=512x512 sub-textures of the same buffer (tbw = row stride).
//  * Ground cache: the ground layer is kept in a VRAM buffer; each frame the previous
//    buffer is shifted by the camera delta (GE copy) and only the newly exposed strips are
//    redrawn (ground draws are recorded and replayed under a per-strip scissor).
//  * Optional half-resolution world (240x136 target, 2x nearest upscale); HUD always 480x272.
//  * Per-stage frame timing written to log.txt every 120 frames.
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

extern "C" void mdaz_boot_log(const char* fmt, ...);

static unsigned int __attribute__((aligned(16))) s_gu_list[262144];

// ---- VRAM layout (offsets from the GE's VRAM base) ----
static const unsigned kFbBytes = 512u * 272u * 2u;          // 278528
static const unsigned kOffFb0 = 0;
static const unsigned kOffFb1 = kFbBytes;
static const unsigned kOffG0 = 2u * kFbBytes;               // ground cache A (512x272 565)
static const unsigned kOffG1 = 3u * kFbBytes;               // ground cache B
static const unsigned kOffHalf = 4u * kFbBytes;             // half-res world 256x136 565
static inline void* vram_abs(unsigned off) { return (void*)(0x04000000u + off); }
static inline unsigned short* vram_uncached(unsigned off) { return (unsigned short*)(0x44000000u + off); }

static inline unsigned int abgr(float r, float g, float b, float a) {
  int ri = (int)(r * 255.f + 0.5f), gi = (int)(g * 255.f + 0.5f), bi = (int)(b * 255.f + 0.5f),
      ai = (int)(a * 255.f + 0.5f);
  if (ri < 0) ri = 0; if (ri > 255) ri = 255;
  if (gi < 0) gi = 0; if (gi > 255) gi = 255;
  if (bi < 0) bi = 0; if (bi > 255) bi = 255;
  if (ai < 0) ai = 0; if (ai > 255) ai = 255;
  return ((unsigned)ai << 24) | ((unsigned)bi << 16) | ((unsigned)gi << 8) | (unsigned)ri;
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

static float stick_axis(unsigned char v) {
  const float f = (v - 128) / 128.f;
  if (f > -0.25f && f < 0.25f) return 0.f;
  return f;
}

struct TV { float u, v; unsigned int c; float x, y, z; };
struct CV { unsigned int c; float x, y, z; };
static const int kTVFmt = GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D;
static const int kCVFmt = GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D;

struct PakEntry {
  char name[48];
  unsigned offset, csize, rawsize;
  unsigned short w, h, stride;
  unsigned char fmt;   // 0 = T8 + CLUT8888 (1024 B CLUT first), 1 = 5551, 2 = 4444, 9 = PNG (v1)
};

struct Tex {
  const PakEntry* pe;
  unsigned char* mem;  // memalign(16) block (CLUT + pixels for T8)
  const void* clut;
  const unsigned char* pix;
  int w, h, stride, psm, bpp;
  unsigned bytes;
  unsigned last_frame;
  unsigned char state; // 0 unloaded, 1 resident, 2 failed
};

struct Rect { int x0, y0, x1, y1; };

struct GroundCmd {
  Tex* t;              // 0 => solid rect
  float x, y, w, h;    // target px
  short sx, sy, sw, sh;
  unsigned int c;
};

struct TextCmd { float x, y; unsigned int c; char s[64]; };

static inline int pow2_at_least(int v) { int r = 1; while (r < v) r <<= 1; return r; }
static inline unsigned fnv(const char* s) {
  unsigned h = 2166136261u;
  for (; *s; ++s) { h ^= (unsigned char)*s; h *= 16777619u; }
  return h;
}

static const int kHash = 4096;
static const int kMaxBatch = 1024;            // vertices (512 sprites)
static const int kMaxGroundCmds = 3072;
static const int kMaxText = 96;
static const unsigned kTexBudget = 8u * 1024u * 1024u;

} // namespace

class PspPlatform : public IPlatform {
public:
  PspPlatform()
    : prev_buttons_(0), boot_tick_(0), inited_(false), frames_(0), draw_off_(kOffFb0),
      pak_data_(0), pak_size_(0), pak_entries_(0), pak_count_(0), pak_v2_(false),
      tex_(0), tex_bytes_(0), font_(0),
      mode_(0), bound_(0), bound_tx_(-1), bound_ty_(-1), nv_(0),
      tgt_w_(480), tgt_h_(272), scale_(1.f), in_world_(false), hr_(false), force_hr_(false),
      clear_c_(0xff000000u),
      gc_valid_(false), gc_cur_(0), gc_cam_x_(0), gc_cam_y_(0), gc_key_(0), gc_w_(0), gc_h_(0),
      recording_(false), ndirty_(0), ngc_(0), ntext_(0),
      prof_stage_(-1), prof_last_(0), prev_frame_end_(0),
      sum_vs_(0), max_vs_(0), avg_vs_(0), last_max_vs_(0),
      sum_frame_(0), max_frame_(0), avg_frame_(0), last_max_frame_(0), nframe_(0),
      prof_frames_(0), prof_dirty_px_(0), overlay_(false), force_overlay_(false),
      rf_fail_logs_(0), autoplay_(false), nocache_(false), auto_frame_(0) {
    images_dir_[0] = 0;
    for (int i = 0; i < kHash; ++i) hash_[i] = -1;
    for (int i = 0; i < Prof_Count; ++i) stage_us_[i] = sum_us_[i] = max_us_[i] = avg_us_[i] = last_max_[i] = 0;
  }
  ~PspPlatform() { shutdown(); }

  bool init(const char* title, int width, int height) {
    (void)title; (void)width; (void)height;
    // NOTE: pre-existing clock call kept as-is (the user did not ask for clock changes).
    scePowerSetClockFrequency(333, 333, 166);
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    sceGuInit();
    sceGuStart(GU_DIRECT, s_gu_list);
    sceGuDrawBuffer(GU_PSM_5650, (void*)kOffFb0, 512);
    sceGuDispBuffer(480, 272, (void*)kOffFb1, 512);
    sceGuOffset(2048 - (480 / 2), 2048 - (272 / 2));
    sceGuViewport(2048, 2048, 480, 272);
    sceGuScissor(0, 0, 480, 272);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_CULL_FACE);
    sceGuShadeModel(GU_FLAT);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGuEnable(GU_ALPHA_TEST);
    sceGuAlphaFunc(GU_GREATER, 0, 0xff);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuTexFilter(GU_NEAREST, GU_NEAREST);
    sceGuTexWrap(GU_CLAMP, GU_CLAMP);
    sceGuClearColor(0xff000000u);
    sceGuClear(GU_COLOR_BUFFER_BIT);
    sceGuFinish();
    sceGuSync(0, 0);
    std::memset(vram_uncached(kOffFb0), 0, kFbBytes);
    std::memset(vram_uncached(kOffFb1), 0, kFbBytes);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
    draw_off_ = kOffFb0;
    mdaz_boot_log("video: GU hardware renderer, fb 0/%u, ground %u/%u, half %u", kOffFb1, kOffG0, kOffG1, kOffHalf);

    build_font();
    u64 t = 0;
    sceRtcGetCurrentTick(&t);
    boot_tick_ = t;
    SceUID fd = sceIoOpen("ms0:/PSP/GAME/MINIDAYZ/autoplay.txt", PSP_O_RDONLY, 0777);
    if (fd < 0) fd = sceIoOpen("autoplay.txt", PSP_O_RDONLY, 0777);
    if (fd >= 0) {
      char b[64] = {0};
      sceIoRead(fd, b, 63);
      sceIoClose(fd);
      autoplay_ = true;
      force_hr_ = std::strstr(b, "half") != 0;
      nocache_ = std::strstr(b, "nocache") != 0;   // A/B check of the ground cache
      force_overlay_ = std::strstr(b, "overlay") != 0;
      auto_pause_ = std::strstr(b, "pause") != 0;     // open pause, focus HALF RES (menu check)
      auto_tutorial_ = std::strstr(b, "tutorial") != 0; // benchmark the tutorial instead of a new game
      if (force_overlay_) overlay_ = true;
      mdaz_boot_log("autoplay.txt found: scripted benchmark input%s", force_hr_ ? " + half-res" : "");
      if (force_hr_) hr_ = true;
    }
    inited_ = true;
    return true;
  }

  void shutdown() {
    clear_textures();
    close_pak();
    if (font_) { std::free(font_); font_ = 0; }
    if (inited_) { sceGuTerm(); inited_ = false; }
  }

  // ---------------- sprites ----------------
  bool load_sprites(const char* images_dir) {
    clear_textures();
    close_pak();
    images_dir_[0] = 0;
    if (!images_dir || !images_dir[0]) return false;
    char path[540];
    const char* ext = std::strrchr(images_dir, '.');
    if (ext && (std::strcmp(ext, ".pak") == 0 || std::strcmp(ext, ".PAK") == 0))
      std::snprintf(path, sizeof(path), "%s", images_dir);
    else
      std::snprintf(path, sizeof(path), "%s/sprites.pak", images_dir);
    if (open_pak(path) && lookup("player_skin_def_1-sheet0")) {
      std::strncpy(images_dir_, path, sizeof(images_dir_) - 1);
      mdaz_boot_log("sprites: pak ok %s (%u entries, %s)", path, pak_count_,
                    pak_v2_ ? "MDZSPR02 GE-native" : "MDZSPR01 PNG (slow fallback)");
      return true;
    }
    mdaz_boot_log("sprites: pak FAIL %s", path);
    close_pak();
    return false;
  }

  bool has_sprite(const char* name) const {
    return const_cast<PspPlatform*>(this)->lookup(name) != 0;
  }

  void draw_sprite(const char* name, float x, float y, float w, float h, float angle_deg, float alpha) {
    draw_sprite_frame(name, x, y, w, h, -1, -1, -1, -1, angle_deg, alpha);
  }

  void draw_sprite_frame(const char* name, float x, float y, float w, float h,
                         int sx, int sy, int sw, int sh, float angle_deg, float alpha) {
    if (w <= 1.e-3f || h <= 1.e-3f || alpha < 0.02f) return;
    Tex* t = lookup(name);
    if (!t) return;
    if (sx < 0 || sy < 0 || sw <= 0 || sh <= 0) { sx = 0; sy = 0; sw = t->w; sh = t->h; }
    if (sx + sw > t->w) sw = t->w - sx;
    if (sy + sh > t->h) sh = t->h - sy;
    if (sw <= 0 || sh <= 0) return;
    if (alpha > 1.f) alpha = 1.f;
    const unsigned int c = ((unsigned)(alpha * 255.f + 0.5f) << 24) | 0x00ffffffu;
    x *= scale_; y *= scale_; w *= scale_; h *= scale_;
    if (recording_) { record(t, x, y, w, h, sx, sy, sw, sh, c); return; }
    if (std::fabs(angle_deg) > 0.01f) { emit_rotated(t, x, y, w, h, sx, sy, sw, sh, angle_deg, c); return; }
    emit_sprite(t, x, y, w, h, (float)sx, (float)sy, (float)sw, (float)sh, c);
  }

  void draw_sprite_frame_mirrored(const char* name, float x, float y, float w, float h,
                                  int sx, int sy, int sw, int sh, float alpha) {
    if (recording_) { draw_sprite_frame(name, x, y, w, h, sx, sy, sw, sh, 0.f, alpha); return; }
    const int before = nv_;
    draw_sprite_frame(name, x, y, w, h, sx, sy, sw, sh, 0.f, alpha);
    // Swap u of each emitted sprite pair (single 512 tile; split sprites keep piece order).
    for (int i = before; i + 1 < nv_; i += 2) { const float u = tv_[i].u; tv_[i].u = tv_[i + 1].u; tv_[i + 1].u = u; }
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
    out.crouch = false; // C2 has no crouch

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
    // A leftover autoplay.txt on the memory stick (dev benchmark) used to replace the pad
    // silently and skip the tutorial (user report 2026-10-20 02:31: stick dead, map instead of
    // Tutorial). Any real button or stick deflection now cancels it for the rest of the run.
    if (autoplay_ && (b != 0 || out.move_x != 0.f || out.move_y != 0.f)) {
      autoplay_ = false;
      mdaz_boot_log("autoplay cancelled by pad input (frame %u)", auto_frame_);
    }
    if (autoplay_) scripted_input(out);
    return true;
  }

  // autoplay.txt benchmark (dev only; absent in normal deploys): start a new game from the
  // menu, then walk a fixed square route so the ground cache and world draw are exercised.
  void scripted_input(InputState& out) {
    const unsigned f = auto_frame_++;
    out = InputState();
    // Intro (Bonds Guru, 3.5 s) + LOADING come first: press NEW GAME after it, then confirm.
    if (f == 330 || f == 390) { out.confirm = true; out.confirm_pressed = true; }
    if (f < 480) return;
    if (auto_pause_ && f >= 840) {
      if (f == 840) out.pause_pressed = out.pause = true;
      // Down x5 -> HALF RES (index 5) on the 8-item PSP pause menu.
      if (f >= 860 && f <= 900 && (f - 860) % 8 == 0) out.move_y = 1.f;
      return;
    }
    const unsigned leg = ((f - 480) / 240) % 4;
    out.run = ((f / 480) & 1) != 0;
    if (leg == 0) out.move_x = 1.f;
    else if (leg == 1) out.move_y = 1.f;
    else if (leg == 2) out.move_x = -1.f;
    else out.move_y = -1.f;
  }

  bool dev_skip_tutorial() const { return autoplay_ && !auto_tutorial_; }

  double time_seconds() {
    u64 t = 0;
    sceRtcGetCurrentTick(&t);
    const u32 res = sceRtcGetTickResolution();
    if (res == 0) return 0.0;
    return (double)(t - boot_tick_) / (double)res;
  }

  void begin_frame() {
    if (!inited_) return;
    sceGuStart(GU_DIRECT, s_gu_list);
    sceGuDrawBufferList(GU_PSM_5650, (void*)draw_off_, 512);
    tgt_w_ = 480; tgt_h_ = 272; scale_ = 1.f; in_world_ = false;
    sceGuScissor(0, 0, 480, 272);
    mode_ = 0; bound_ = 0; bound_tx_ = bound_ty_ = -1; nv_ = 0; ntext_ = 0;
  }

  void clear(float r, float g, float b) {
    clear_c_ = abgr(r, g, b, 1.f);
    flush();
    // Opaque rect over the current target (scissor-proof, works for every target).
    set_mode(2);
    CV* v = (CV*)sceGuGetMemory(2 * sizeof(CV));
    v[0].c = clear_c_; v[0].x = 0.f; v[0].y = 0.f; v[0].z = 0.f;
    v[1].c = clear_c_; v[1].x = (float)tgt_w_; v[1].y = (float)tgt_h_; v[1].z = 0.f;
    sceGuDrawArray(GU_SPRITES, kCVFmt, 2, 0, v);
  }

  void draw_rect(float x, float y, float w, float h, float r, float g, float b, float a) {
    if (a < 0.02f || w <= 0.f || h <= 0.f) return;
    x *= scale_; y *= scale_; w *= scale_; h *= scale_;
    const unsigned int c = abgr(r, g, b, a > 1.f ? 1.f : a);
    if (recording_) { record(0, x, y, w, h, 0, 0, 0, 0, c); return; }
    emit_rect(x, y, w, h, c);
  }

  void draw_text(float x, float y, const char* text, float r, float g, float b) {
    if (!text || !font_) return;
    const unsigned int c = abgr(r, g, b, 1.f);
    if (in_world_ && hr_) {           // keep text sharp: defer to the full-res pass
      if (ntext_ < kMaxText) {
        TextCmd& tc = text_[ntext_++];
        tc.x = x; tc.y = y; tc.c = c;
        std::strncpy(tc.s, text, sizeof(tc.s) - 1); tc.s[sizeof(tc.s) - 1] = 0;
      }
      return;
    }
    emit_text(x * scale_, y * scale_, text, c);
  }

  void end_frame() {
    if (!inited_) return;
    if (in_world_) world_pass_end();
    if (overlay_) draw_overlay();
    if (autoplay_) {   // never invisible: the scripted benchmark owns the pad
      draw_rect(0.f, 0.f, 480.f, 11.f, 0.f, 0.f, 0.f, 0.7f);
      draw_text(4.f, 2.f, "AUTOPLAY BENCH (autoplay.txt) - press any button", 1.f, 0.85f, 0.2f);
    }
    flush();
    profile_mark(Prof_Present);              // GU finish + sync (GE work still pending)
    sceKernelDcacheWritebackAll();
    sceGuFinish();
    sceGuSync(0, 0);
    const u64 tv = now_us();
    stage_us_[Prof_Present] += (unsigned)(tv - prof_last_);
    prof_stage_ = -1;
    sceDisplayWaitVblankStart();
    draw_off_ = (unsigned)sceGuSwapBuffers();
    const u64 now = now_us();
    frame_end(tv, now);
    if (frames_ < 3) mdaz_boot_log("frame %u presented (GU)", frames_);
    ++frames_;
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
      // Throttled: the old build logged + reopened 3 paths per frame for has_save().
      if (rf_fail_logs_ < 12) {
        ++rf_fail_logs_;
        log_io("rf_fail", path);
        mdaz_boot_log("file %s: OPEN FAILED (cwd + ms0:/PSP/GAME/MINIDAYZ*/ tried)", path);
      }
      return false;
    }

    const SceOff sz = sceIoLseek(fd, 0, PSP_SEEK_END);
    sceIoLseek(fd, 0, PSP_SEEK_SET);
    if (sz <= 0 || sz > 12 * 1024 * 1024) {
      sceIoClose(fd);
      mdaz_boot_log("file %s: bad size %d", tried, (int)sz);
      return false;
    }
    char* buf = (char*)std::malloc((std::size_t)sz + 1);
    if (!buf) {
      sceIoClose(fd);
      mdaz_boot_log("file %s: OUT OF MEMORY for %d bytes (free %d)", tried, (int)sz,
                    (int)sceKernelMaxFreeMemSize());
      return false;
    }
    const int n = sceIoRead(fd, buf, (int)sz);
    sceIoClose(fd);
    if (n <= 0) { std::free(buf); mdaz_boot_log("file %s: READ FAILED", tried); return false; }
    buf[n] = 0;
    mdaz_boot_log("file %s: ok %d bytes", tried, n);
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

  // ---------------- perf hooks ----------------
  // Per-stage CPU wall time in microseconds (sceKernelGetSystemTimeWide). A stage runs from
  // its mark to the next mark; "gpu" = sceGuFinish+sceGuSync, "vsync" = vblank wait.
  static u64 now_us() { return (u64)sceKernelGetSystemTimeWide(); }
  void profile_mark(int stage) {
    const u64 now = now_us();
    if (prof_stage_ >= 0 && prof_stage_ < Prof_Count) stage_us_[prof_stage_] += (unsigned)(now - prof_last_);
    prof_stage_ = stage;
    prof_last_ = now;
  }

  void frame_end(u64 vsync_start, u64 now) {
    const unsigned vs = (unsigned)(now - vsync_start);
    const unsigned frame = prev_frame_end_ ? (unsigned)(now - prev_frame_end_) : 0u;
    prev_frame_end_ = now;
    for (int i = 0; i < Prof_Count; ++i) {
      sum_us_[i] += stage_us_[i];
      if (stage_us_[i] > max_us_[i]) max_us_[i] = stage_us_[i];
      stage_us_[i] = 0;
    }
    sum_vs_ += vs; if (vs > max_vs_) max_vs_ = vs;
    if (frame) { sum_frame_ += frame; if (frame > max_frame_) max_frame_ = frame; ++nframe_; }
    if (++prof_frames_ >= 120) log_profile();
  }

  int ground_snap_px() const { return hr_ ? 2 : 1; }

  bool ground_cache_begin(float cam_px_x, float cam_px_y, unsigned key) {
    const int cx = (int)std::floor(cam_px_x * scale_ + 0.5f);
    const int cy = (int)std::floor(cam_px_y * scale_ + 0.5f);
    const int W = tgt_w_, H = tgt_h_;
    ndirty_ = 0;
    flush();
    const unsigned cur = gc_cur_ ^ 1;
    const int dx = cx - gc_cam_x_, dy = cy - gc_cam_y_;
    const bool full = nocache_ || !gc_valid_ || key != gc_key_ || W != gc_w_ || H != gc_h_ ||
                      dx >= W || -dx >= W || dy >= H || -dy >= H;
    if (full) {
      dirty_[ndirty_++] = Rect{0, 0, W, H};
    } else if (dx != 0 || dy != 0) {
      // new(p) = old(p + d): shift the previous buffer into the other one.
      const int cw = W - (dx > 0 ? dx : -dx), ch = H - (dy > 0 ? dy : -dy);
      sceGuCopyImage(GU_PSM_5650, dx > 0 ? dx : 0, dy > 0 ? dy : 0, cw, ch, 512,
                     vram_abs(gc_cur_ ? kOffG1 : kOffG0),
                     dx > 0 ? 0 : -dx, dy > 0 ? 0 : -dy, 512, vram_abs(cur ? kOffG1 : kOffG0));
      sceGuTexSync();
      if (dx > 0) dirty_[ndirty_++] = Rect{W - dx, 0, W, H};
      else if (dx < 0) dirty_[ndirty_++] = Rect{0, 0, -dx, H};
      const int vx0 = dx > 0 ? 0 : -dx, vx1 = dx > 0 ? W - dx : W;   // exclude the vertical strip
      if (dy > 0) dirty_[ndirty_++] = Rect{vx0, H - dy, vx1, H};
      else if (dy < 0) dirty_[ndirty_++] = Rect{vx0, 0, vx1, -dy};
    }
    gc_pending_cur_ = (ndirty_ > 0 && !full) ? cur : (full ? cur : gc_cur_);
    gc_new_cam_x_ = cx; gc_new_cam_y_ = cy;
    gc_key_ = key; gc_w_ = W; gc_h_ = H;
    ngc_ = 0;
    recording_ = ndirty_ > 0;
    for (int i = 0; i < ndirty_; ++i)
      prof_dirty_px_ += (unsigned)((dirty_[i].x1 - dirty_[i].x0) * (dirty_[i].y1 - dirty_[i].y0));
    return recording_;
  }

  void ground_cache_end() {
    flush();
    const unsigned cur = gc_pending_cur_;
    if (recording_) {
      recording_ = false;
      sceGuDrawBufferList(GU_PSM_5650, (void*)(cur ? kOffG1 : kOffG0), 512);
      for (int d = 0; d < ndirty_; ++d) {
        const Rect& r = dirty_[d];
        sceGuScissor(r.x0, r.y0, r.x1, r.y1);
        emit_rect((float)r.x0, (float)r.y0, (float)(r.x1 - r.x0), (float)(r.y1 - r.y0), clear_c_);
        for (int i = 0; i < ngc_; ++i) {
          const GroundCmd& g = gc_[i];
          if (g.x >= r.x1 || g.y >= r.y1 || g.x + g.w <= r.x0 || g.y + g.h <= r.y0) continue;
          if (g.t) emit_sprite(g.t, g.x, g.y, g.w, g.h, g.sx, g.sy, g.sw, g.sh, g.c);
          else emit_rect(g.x, g.y, g.w, g.h, g.c);
        }
        flush();
      }
      restore_target();
      gc_cur_ = cur;
      gc_valid_ = true;
    }
    gc_cam_x_ = gc_new_cam_x_; gc_cam_y_ = gc_new_cam_y_;
    // Ground buffer -> current target (1:1 VRAM copy, replaces the clear).
    sceGuCopyImage(GU_PSM_5650, 0, 0, tgt_w_, tgt_h_, 512, vram_abs(gc_cur_ ? kOffG1 : kOffG0),
                   0, 0, hr_ && in_world_ ? 256 : 512, vram_abs(target_off()));
    sceGuTexSync();
    mode_ = 0; bound_ = 0;
  }

  void ground_cache_invalidate() { gc_valid_ = false; }

  void world_pass_begin() {
    in_world_ = true;
    if (!hr_) return;
    flush();
    tgt_w_ = 240; tgt_h_ = 136; scale_ = 0.5f;
    sceGuDrawBufferList(GU_PSM_5650, (void*)kOffHalf, 256);
    sceGuScissor(0, 0, 240, 136);
  }

  void world_pass_end() {
    if (!in_world_) return;
    in_world_ = false;
    if (!hr_) return;
    flush();
    tgt_w_ = 480; tgt_h_ = 272; scale_ = 1.f;
    sceGuDrawBufferList(GU_PSM_5650, (void*)draw_off_, 512);
    sceGuScissor(0, 0, 480, 272);
    // Upscale the 240x136 world 2x (nearest; pixel art).
    set_mode(1);
    sceGuTexMode(GU_PSM_5650, 0, 0, 0);
    sceGuTexImage(0, 256, 256, 256, vram_abs(kOffHalf));
    sceGuTexFlush();
    bound_ = 0;
    TV* v = (TV*)sceGuGetMemory(2 * sizeof(TV));
    v[0].u = 0.f; v[0].v = 0.f; v[0].c = 0xffffffffu; v[0].x = 0.f; v[0].y = 0.f; v[0].z = 0.f;
    v[1].u = 240.f; v[1].v = 136.f; v[1].c = 0xffffffffu; v[1].x = 480.f; v[1].y = 272.f; v[1].z = 0.f;
    sceGuDrawArray(GU_SPRITES, kTVFmt, 2, 0, v);
    for (int i = 0; i < ntext_; ++i) emit_text(text_[i].x, text_[i].y, text_[i].s, text_[i].c);
    ntext_ = 0;
  }

  bool supports_half_res() const { return true; }
  void set_half_res(bool on) {
    if (force_hr_) on = true;
    if (on != hr_) { hr_ = on; gc_valid_ = false; }
  }
  bool half_res() const { return hr_; }
  void log_line(const char* s) { mdaz_boot_log("%s", s); }
  bool supports_perf_overlay() const { return true; }
  void set_perf_overlay(bool on) { overlay_ = on || force_overlay_; }
  bool perf_overlay() const { return overlay_; }

private:
  // ---------------- GE helpers ----------------
  unsigned target_off() const { return (hr_ && in_world_) ? kOffHalf : draw_off_; }
  void restore_target() {
    if (hr_ && in_world_) {
      sceGuDrawBufferList(GU_PSM_5650, (void*)kOffHalf, 256);
      sceGuScissor(0, 0, 240, 136);
    } else {
      sceGuDrawBufferList(GU_PSM_5650, (void*)draw_off_, 512);
      sceGuScissor(0, 0, 480, 272);
    }
  }

  void set_mode(int m) {
    if (m == mode_) return;
    flush();
    if (m == 1) sceGuEnable(GU_TEXTURE_2D); else sceGuDisable(GU_TEXTURE_2D);
    mode_ = m;
  }

  void flush() {
    if (nv_ <= 0) return;
    if (mode_ == 1) {
      TV* v = (TV*)sceGuGetMemory(nv_ * sizeof(TV));
      std::memcpy(v, tv_, nv_ * sizeof(TV));
      sceGuDrawArray(GU_SPRITES, kTVFmt, nv_, 0, v);
    } else {
      CV* v = (CV*)sceGuGetMemory(nv_ * sizeof(CV));
      std::memcpy(v, cv_, nv_ * sizeof(CV));
      sceGuDrawArray(GU_SPRITES, kCVFmt, nv_, 0, v);
    }
    nv_ = 0;
  }

  void bind(Tex* t, int tx, int ty) {
    if (mode_ == 1 && bound_ == t && bound_tx_ == tx && bound_ty_ == ty) return;
    set_mode(1);
    flush();
    const int x0 = tx * 512, y0 = ty * 512;
    int tw = t->w - x0; if (tw > 512) tw = 512;
    int th = t->h - y0; if (th > 512) th = 512;
    const unsigned char* base = t->pix + ((unsigned)y0 * (unsigned)t->stride + (unsigned)x0) * (unsigned)t->bpp;
    sceGuTexMode(t->psm, 0, 0, 0);
    if (t->psm == GU_PSM_T8) {
      sceGuClutMode(GU_PSM_8888, 0, 0xff, 0);
      sceGuClutLoad(32, t->clut);
    }
    sceGuTexImage(0, pow2_at_least(tw), pow2_at_least(th), t->stride, base);
    sceGuTexFlush();
    bound_ = t; bound_tx_ = tx; bound_ty_ = ty;
  }

  // Clip dest to the target (GE through-mode dislikes negative coords) with matching uv.
  bool clip(float& x, float& y, float& w, float& h, float& u, float& v, float& uw, float& vh) const {
    const float W = (float)tgt_w_, H = (float)tgt_h_;
    if (x >= W || y >= H || x + w <= 0.f || y + h <= 0.f) return false;
    if (x < 0.f) { const float k = -x / w; u += uw * k; uw -= uw * k; w += x; x = 0.f; }
    if (y < 0.f) { const float k = -y / h; v += vh * k; vh -= vh * k; h += y; y = 0.f; }
    if (x + w > W) { const float k = (x + w - W) / w; uw -= uw * k; w = W - x; }
    if (y + h > H) { const float k = (y + h - H) / h; vh -= vh * k; h = H - y; }
    return w > 0.f && h > 0.f;
  }

  void emit_sprite(Tex* t, float x, float y, float w, float h, float sx, float sy, float sw, float sh,
                   unsigned int c) {
    if (!resident(t)) return;
    if (!clip(x, y, w, h, sx, sy, sw, sh)) return;
    const int tx0 = (int)sx / 512, ty0 = (int)sy / 512;
    const int tx1 = (int)(sx + sw - 0.001f) / 512, ty1 = (int)(sy + sh - 0.001f) / 512;
    if (tx0 == tx1 && ty0 == ty1) {
      bind(t, tx0, ty0);
      push_tv(x, y, w, h, sx - tx0 * 512, sy - ty0 * 512, sw, sh, c);
      return;
    }
    // Source rect spans several 512 sub-textures: split along the tile borders.
    for (int ty = ty0; ty <= ty1; ++ty) {
      for (int tx = tx0; tx <= tx1; ++tx) {
        const float bx0 = tx * 512.f, by0 = ty * 512.f;
        const float ux0 = sx > bx0 ? sx : bx0, ux1 = (sx + sw) < bx0 + 512.f ? (sx + sw) : bx0 + 512.f;
        const float vy0 = sy > by0 ? sy : by0, vy1 = (sy + sh) < by0 + 512.f ? (sy + sh) : by0 + 512.f;
        if (ux1 <= ux0 || vy1 <= vy0) continue;
        const float dx0 = x + (ux0 - sx) * w / sw, dx1 = x + (ux1 - sx) * w / sw;
        const float dy0 = y + (vy0 - sy) * h / sh, dy1 = y + (vy1 - sy) * h / sh;
        bind(t, tx, ty);
        push_tv(dx0, dy0, dx1 - dx0, dy1 - dy0, ux0 - bx0, vy0 - by0, ux1 - ux0, vy1 - vy0, c);
      }
    }
  }

  void push_tv(float x, float y, float w, float h, float u, float v, float uw, float vh, unsigned int c) {
    if (nv_ + 2 > kMaxBatch) flush();
    TV* a = &tv_[nv_++];
    a->u = u; a->v = v; a->c = c; a->x = x; a->y = y; a->z = 0.f;
    TV* b = &tv_[nv_++];
    b->u = u + uw; b->v = v + vh; b->c = c; b->x = x + w; b->y = y + h; b->z = 0.f;
  }

  void emit_rect(float x, float y, float w, float h, unsigned int c) {
    const float W = (float)tgt_w_, H = (float)tgt_h_;
    if (x < 0.f) { w += x; x = 0.f; }
    if (y < 0.f) { h += y; y = 0.f; }
    if (x + w > W) w = W - x;
    if (y + h > H) h = H - y;
    if (w <= 0.f || h <= 0.f) return;
    set_mode(2);
    if (nv_ + 2 > kMaxBatch) flush();
    CV* a = &cv_[nv_++]; a->c = c; a->x = x; a->y = y; a->z = 0.f;
    CV* b = &cv_[nv_++]; b->c = c; b->x = x + w; b->y = y + h; b->z = 0.f;
  }

  void emit_rotated(Tex* t, float x, float y, float w, float h, int sx, int sy, int sw, int sh,
                    float angle_deg, unsigned int c) {
    if (!resident(t)) return;
    const float cxp = x + w * 0.5f, cyp = y + h * 0.5f;
    const float rad = angle_deg * 0.01745329252f;
    const float cs = std::cos(rad), sn = std::sin(rad);
    const float r = 0.5f * std::sqrt(w * w + h * h);
    if (cxp + r < 0.f || cyp + r < 0.f || cxp - r > tgt_w_ || cyp - r > tgt_h_) return;
    if (sx / 512 != (sx + sw - 1) / 512 || sy / 512 != (sy + sh - 1) / 512) {
      emit_sprite(t, x, y, w, h, (float)sx, (float)sy, (float)sw, (float)sh, c); // rare: no rotation
      return;
    }
    const int tx = sx / 512, ty = sy / 512;
    bind(t, tx, ty);
    flush();
    const float u0 = (float)(sx - tx * 512), v0 = (float)(sy - ty * 512);
    const float hx[4] = { -w * 0.5f, w * 0.5f, -w * 0.5f, w * 0.5f };
    const float hy[4] = { -h * 0.5f, -h * 0.5f, h * 0.5f, h * 0.5f };
    const float uu[4] = { u0, u0 + sw, u0, u0 + sw };
    const float vv[4] = { v0, v0, v0 + sh, v0 + sh };
    TV* v = (TV*)sceGuGetMemory(4 * sizeof(TV));
    for (int i = 0; i < 4; ++i) {
      v[i].u = uu[i]; v[i].v = vv[i]; v[i].c = c;
      v[i].x = cxp + hx[i] * cs - hy[i] * sn;
      v[i].y = cyp + hx[i] * sn + hy[i] * cs;
      v[i].z = 0.f;
    }
    sceGuDrawArray(GU_TRIANGLE_STRIP, kTVFmt, 4, 0, v);
  }

  void emit_text(float x, float y, const char* text, unsigned int c) {
    if (!font_) return;
    Tex* f = &font_tex_;
    float cx = x;
    for (int pass = 0; pass < 2; ++pass) {
      const unsigned int col = pass == 0 ? (c & 0xff000000u) : c;   // 1px drop shadow first
      const float off = pass == 0 ? 1.f : 0.f;
      cx = x; float cy = y;
      for (const char* p = text; *p; ++p) {
        if (*p == '\n') { cx = x; cy += 10.f; continue; }
        int ch = (unsigned char)*p;
        if (ch < 32 || ch > 127) ch = '?';
        const int gi = ch - 32;
        if (gi > 0) {
          float dx = cx + off, dy = cy + off, dw = 5.f, dh = 7.f;
          float u = (float)((gi % 16) * 8), v = (float)((gi / 16) * 8), uw = 5.f, vh = 7.f;
          if (clip(dx, dy, dw, dh, u, v, uw, vh)) {
            bind(f, 0, 0);
            push_tv(dx, dy, dw, dh, u, v, uw, vh, col);
          }
        }
        cx += 6.f;
      }
    }
  }

  void build_font() {
    // T8 128x64, glyph i at ((i%16)*8, (i/16)*8); CLUT: 0 transparent, 1 white.
    const unsigned bytes = 1024u + 128u * 64u;
    font_ = (unsigned char*)memalign(16, bytes);
    if (!font_) return;
    std::memset(font_, 0, bytes);
    unsigned int* clut = (unsigned int*)font_;
    clut[1] = 0xffffffffu;
    unsigned char* px = font_ + 1024;
    for (int g = 0; g < 96; ++g)
      for (int row = 0; row < 7; ++row)
        for (int col = 0; col < 5; ++col)
          if (kGlyph5x7[g][row] & (0x10 >> col))
            px[((g / 16) * 8 + row) * 128 + (g % 16) * 8 + col] = 1;
    sceKernelDcacheWritebackRange(font_, bytes);
    font_tex_.pe = 0; font_tex_.mem = font_; font_tex_.clut = font_; font_tex_.pix = px;
    font_tex_.w = 128; font_tex_.h = 64; font_tex_.stride = 128; font_tex_.psm = GU_PSM_T8;
    font_tex_.bpp = 1; font_tex_.bytes = bytes; font_tex_.last_frame = 0xffffffffu; font_tex_.state = 1;
  }

  void record(Tex* t, float x, float y, float w, float h, int sx, int sy, int sw, int sh, unsigned int c) {
    bool hit = false;
    for (int i = 0; i < ndirty_ && !hit; ++i) {
      const Rect& r = dirty_[i];
      hit = !(x >= r.x1 || y >= r.y1 || x + w <= r.x0 || y + h <= r.y0);
    }
    if (!hit || ngc_ >= kMaxGroundCmds) return;
    GroundCmd& g = gc_[ngc_++];
    g.t = t; g.x = x; g.y = y; g.w = w; g.h = h;
    g.sx = (short)sx; g.sy = (short)sy; g.sw = (short)sw; g.sh = (short)sh; g.c = c;
  }

  void log_profile() {
    const unsigned n = prof_frames_ ? prof_frames_ : 1;
    const unsigned nf = nframe_ ? nframe_ : 1;
    for (int i = 0; i < Prof_Count; ++i) { avg_us_[i] = sum_us_[i] / n; last_max_[i] = max_us_[i]; }
    avg_vs_ = sum_vs_ / n; last_max_vs_ = max_vs_;
    avg_frame_ = sum_frame_ / nf; last_max_frame_ = max_frame_;
    mdaz_boot_log("perf us avg/max over %u frames: logic %u/%u ground %u/%u objects %u/%u hud %u/%u "
                  "gpu(finish+sync) %u/%u vsync %u/%u | frame %u/%u | half_res %d ground-redraw %u px/f tex %u KB",
                  n, avg_us_[Prof_Logic], last_max_[Prof_Logic], avg_us_[Prof_Ground], last_max_[Prof_Ground],
                  avg_us_[Prof_World], last_max_[Prof_World], avg_us_[Prof_Hud], last_max_[Prof_Hud],
                  avg_us_[Prof_Present], last_max_[Prof_Present], avg_vs_, last_max_vs_,
                  avg_frame_, last_max_frame_, hr_ ? 1 : 0, prof_dirty_px_ / n, tex_bytes_ / 1024u);
    for (int i = 0; i < Prof_Count; ++i) { sum_us_[i] = 0; max_us_[i] = 0; }
    sum_vs_ = max_vs_ = sum_frame_ = max_frame_ = nframe_ = 0;
    prof_frames_ = 0; prof_dirty_px_ = 0;
  }

  void draw_overlay() {
    // Last 120-frame window, milliseconds with one decimal (avg / max).
    char l1[96], l2[96];
    std::snprintf(l1, sizeof(l1), "LOG %u.%u GND %u.%u OBJ %u.%u HUD %u.%u",
                  avg_us_[Prof_Logic] / 1000, (avg_us_[Prof_Logic] / 100) % 10,
                  avg_us_[Prof_Ground] / 1000, (avg_us_[Prof_Ground] / 100) % 10,
                  avg_us_[Prof_World] / 1000, (avg_us_[Prof_World] / 100) % 10,
                  avg_us_[Prof_Hud] / 1000, (avg_us_[Prof_Hud] / 100) % 10);
    std::snprintf(l2, sizeof(l2), "GPU %u.%u VS %u.%u FRM %u.%u MAX %u.%u MS",
                  avg_us_[Prof_Present] / 1000, (avg_us_[Prof_Present] / 100) % 10,
                  avg_vs_ / 1000, (avg_vs_ / 100) % 10, avg_frame_ / 1000, (avg_frame_ / 100) % 10,
                  last_max_frame_ / 1000, (last_max_frame_ / 100) % 10);
    emit_rect(4.f, 222.f, 252.f, 22.f, 0xb0000000u);
    emit_text(7.f, 224.f, l1, 0xff80ff80u);
    emit_text(7.f, 234.f, l2, 0xff80ff80u);
  }

  // ---------------- textures / pak ----------------
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
    if (tex_) { std::free(tex_); tex_ = 0; }
    pak_size_ = 0; pak_count_ = 0; pak_v2_ = false;
    for (int i = 0; i < kHash; ++i) hash_[i] = -1;
  }

  static unsigned rd32(const char* p) {
    return (unsigned)(unsigned char)p[0] | ((unsigned)(unsigned char)p[1] << 8) |
           ((unsigned)(unsigned char)p[2] << 16) | ((unsigned)(unsigned char)p[3] << 24);
  }
  static unsigned rd16(const char* p) {
    return (unsigned)(unsigned char)p[0] | ((unsigned)(unsigned char)p[1] << 8);
  }

  bool open_pak(const char* path) {
    char* data = 0;
    std::size_t size = 0;
    if (!read_file(path, &data, &size) || !data || size < 16) { if (data) std::free(data); return false; }
    const bool v2 = std::memcmp(data, "MDZSPR02", 8) == 0;
    const bool v1 = std::memcmp(data, "MDZSPR01", 8) == 0;
    if (!v1 && !v2) { std::free(data); return false; }
    const unsigned count = rd32(data + 8);
    const unsigned esz = v2 ? 72u : 56u;
    if (count == 0 || 16u + count * esz > size) { std::free(data); return false; }
    PakEntry* ents = (PakEntry*)std::malloc(sizeof(PakEntry) * count);
    Tex* tex = (Tex*)std::malloc(sizeof(Tex) * count);
    if (!ents || !tex) { std::free(ents); std::free(tex); std::free(data); return false; }
    for (unsigned i = 0; i < count; ++i) {
      const char* e = data + 16 + i * esz;
      std::memcpy(ents[i].name, e, 48); ents[i].name[47] = 0;
      ents[i].offset = rd32(e + 48);
      ents[i].csize = rd32(e + 52);
      if (v2) {
        ents[i].rawsize = rd32(e + 56);
        ents[i].w = (unsigned short)rd16(e + 60);
        ents[i].h = (unsigned short)rd16(e + 62);
        ents[i].stride = (unsigned short)rd16(e + 64);
        ents[i].fmt = (unsigned char)e[66];
      } else {
        ents[i].rawsize = 0; ents[i].w = ents[i].h = ents[i].stride = 0; ents[i].fmt = 9;
      }
      std::memset(&tex[i], 0, sizeof(Tex));
      tex[i].pe = &ents[i];
      tex[i].w = ents[i].w; tex[i].h = ents[i].h;
      unsigned hsh = fnv(ents[i].name) & (kHash - 1);
      while (hash_[hsh] >= 0) hsh = (hsh + 1) & (kHash - 1);
      hash_[hsh] = (int)i;
    }
    pak_data_ = data; pak_size_ = size; pak_entries_ = ents; pak_count_ = count; pak_v2_ = v2;
    tex_ = tex;
    return true;
  }

  int find_index(const char* key) const {
    unsigned hsh = fnv(key) & (kHash - 1);
    for (int probe = 0; probe < kHash; ++probe) {
      const int i = hash_[hsh];
      if (i < 0) return -1;
      if (std::strcmp(pak_entries_[i].name, key) == 0) return i;
      hsh = (hsh + 1) & (kHash - 1);
    }
    return -1;
  }

  Tex* lookup(const char* name) {
    if (!name || !name[0] || !tex_) return 0;
    char key[56];
    std::strncpy(key, name, sizeof(key) - 1); key[sizeof(key) - 1] = 0;
    char* dot = std::strstr(key, ".png");
    if (dot) *dot = 0;
    int i = find_index(key);
    if (i < 0) {                       // C2 alias "foo" <-> "foo-sheet0"
      char alt[64];
      char* sh = std::strstr(key, "-sheet");
      if (!sh) { std::snprintf(alt, sizeof(alt), "%s-sheet0", key); i = find_index(alt); }
      else { *sh = 0; i = find_index(key); }
    }
    if (i < 0) return 0;
    Tex* t = &tex_[i];
    if (t->state == 2) return 0;
    if (t->state == 0 && !pak_v2_) { if (!load_tex(t)) return 0; }  // v1: need w/h -> decode now
    return t;
  }

  bool resident(Tex* t) {
    if (t == &font_tex_) return true;
    if (t->state == 0 && !load_tex(t)) return false;
    if (t->state != 1) return false;
    t->last_frame = frames_;
    return true;
  }

  void evict_for(unsigned need) {
    while (tex_bytes_ + need > kTexBudget) {
      int victim = -1;
      unsigned best = 0xffffffffu;
      for (unsigned i = 0; i < pak_count_; ++i) {
        const Tex& t = tex_[i];
        if (t.state != 1 || t.last_frame >= frames_) continue;   // never drop this frame's textures
        if (t.last_frame < best) { best = t.last_frame; victim = (int)i; }
      }
      if (victim < 0) return;
      Tex& v = tex_[victim];
      std::free(v.mem); v.mem = 0; v.pix = 0; v.clut = 0;
      tex_bytes_ -= v.bytes; v.bytes = 0; v.state = 0;
    }
  }

  bool load_tex(Tex* t) {
    const PakEntry* pe = t->pe;
    if (!pe || pe->offset + pe->csize > pak_size_) { t->state = 2; return false; }
    const char* src = pak_data_ + pe->offset;
    if (pe->fmt == 9) return load_png_v1(t, src, (int)pe->csize);
    const unsigned raw = pe->rawsize;
    evict_for(raw);
    unsigned char* mem = (unsigned char*)memalign(16, raw);
    if (!mem) { evict_for(kTexBudget); mem = (unsigned char*)memalign(16, raw); }
    if (!mem) { mdaz_boot_log("tex %s: OOM %u bytes", pe->name, raw); return false; }
    const int got = stbi_zlib_decode_buffer((char*)mem, (int)raw, src, (int)pe->csize);
    if (got != (int)raw) {
      std::free(mem); t->state = 2;
      mdaz_boot_log("tex %s: inflate failed (%d/%u)", pe->name, got, raw);
      return false;
    }
    sceKernelDcacheWritebackRange(mem, raw);
    t->mem = mem; t->w = pe->w; t->h = pe->h; t->stride = pe->stride;
    if (pe->fmt == 0) { t->psm = GU_PSM_T8; t->bpp = 1; t->clut = mem; t->pix = mem + 1024; }
    else { t->psm = pe->fmt == 1 ? GU_PSM_5551 : GU_PSM_4444; t->bpp = 2; t->clut = 0; t->pix = mem; }
    t->bytes = raw; t->state = 1; t->last_frame = frames_;
    tex_bytes_ += raw;
    return true;
  }

  // Old MDZSPR01 pak (raw PNG): decode + convert to 4444 (slow; only for stale deploys).
  bool load_png_v1(Tex* t, const char* src, int len) {
    int w = 0, h = 0, comp = 0;
    unsigned char* px = stbi_load_from_memory((const unsigned char*)src, len, &w, &h, &comp, 4);
    if (!px) { t->state = 2; return false; }
    const int stride = (w + 7) & ~7;
    const unsigned raw = (unsigned)stride * (unsigned)h * 2u;
    evict_for(raw);
    unsigned short* mem = (unsigned short*)memalign(16, raw);
    if (!mem) { stbi_image_free(px); return false; }
    for (int y = 0; y < h; ++y)
      for (int x = 0; x < stride; ++x) {
        unsigned short v = 0;
        if (x < w) {
          const unsigned char* p = px + (y * w + x) * 4;
          v = (unsigned short)((p[0] >> 4) | ((p[1] >> 4) << 4) | ((p[2] >> 4) << 8) | ((p[3] >> 4) << 12));
        }
        mem[y * stride + x] = v;
      }
    stbi_image_free(px);
    sceKernelDcacheWritebackRange(mem, raw);
    t->mem = (unsigned char*)mem; t->pix = (unsigned char*)mem; t->clut = 0;
    t->w = w; t->h = h; t->stride = stride; t->psm = GU_PSM_4444; t->bpp = 2;
    t->bytes = raw; t->state = 1; t->last_frame = frames_;
    tex_bytes_ += raw;
    return true;
  }

  void clear_textures() {
    if (tex_) {
      for (unsigned i = 0; i < pak_count_; ++i) {
        if (tex_[i].mem) std::free(tex_[i].mem);
        tex_[i].mem = 0; tex_[i].pix = 0; tex_[i].clut = 0; tex_[i].state = 0; tex_[i].bytes = 0;
      }
    }
    tex_bytes_ = 0;
    bound_ = 0;
  }

  // ---------------- state ----------------
  unsigned int prev_buttons_;
  u64 boot_tick_;
  bool inited_;
  unsigned frames_;
  unsigned draw_off_;

  char images_dir_[512];
  char* pak_data_;
  std::size_t pak_size_;
  PakEntry* pak_entries_;
  unsigned pak_count_;
  bool pak_v2_;
  int hash_[kHash];
  Tex* tex_;
  unsigned tex_bytes_;
  unsigned char* font_;
  Tex font_tex_;

  int mode_;            // 0 none, 1 textured, 2 colour
  Tex* bound_;
  int bound_tx_, bound_ty_;
  int nv_;
  TV tv_[kMaxBatch];
  CV cv_[kMaxBatch];

  int tgt_w_, tgt_h_;
  float scale_;
  bool in_world_, hr_, force_hr_;
  unsigned int clear_c_;

  bool gc_valid_;
  unsigned gc_cur_, gc_pending_cur_;
  int gc_cam_x_, gc_cam_y_, gc_new_cam_x_, gc_new_cam_y_;
  unsigned gc_key_;
  int gc_w_, gc_h_;
  bool recording_;
  Rect dirty_[3];
  int ndirty_;
  GroundCmd gc_[kMaxGroundCmds];
  int ngc_;
  TextCmd text_[kMaxText];
  int ntext_;

  int prof_stage_;
  u64 prof_last_;
  u64 prev_frame_end_;
  unsigned stage_us_[Prof_Count], sum_us_[Prof_Count], max_us_[Prof_Count];
  unsigned avg_us_[Prof_Count], last_max_[Prof_Count];
  unsigned sum_vs_, max_vs_, avg_vs_, last_max_vs_;
  unsigned sum_frame_, max_frame_, avg_frame_, last_max_frame_, nframe_;
  unsigned prof_frames_;
  unsigned prof_dirty_px_;
  bool overlay_, force_overlay_;
  int rf_fail_logs_;
  bool autoplay_;
  bool auto_tutorial_ = false;
  bool auto_pause_ = false;
  bool nocache_;
  unsigned auto_frame_;
};

IPlatform* create_platform() { return new PspPlatform(); }

#endif

} // namespace mdaz
