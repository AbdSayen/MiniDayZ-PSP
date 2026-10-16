#include "mdaz/platform.hpp"

#include <cstdio>
#include <cstring>

// When compiled without PSPSDK headers, provide a stub create_platform
// so the file remains readable. Real implementation uses sceGu / sceCtrl.

#if defined(MDAZ_PSP) && defined(__PSP__)
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psppower.h>
// Full GU renderer TBD — see KNOWN_ISSUES.md
#endif

namespace mdaz {

#if !(defined(MDAZ_PSP) && defined(__PSP__))

IPlatform* create_platform() {
  std::fprintf(stderr, "platform_psp.cpp: not building with PSPSDK; stub.\n");
  return 0;
}

#else

// Minimal PSP platform placeholder — expand with GU blit + VRAM texture streaming.
class PspPlatform : public IPlatform {
public:
  bool init(const char* title, int width, int height) {
    (void)title;
    (void)width;
    (void)height;
    scePowerSetClockFrequency(333, 333, 166);
    return true;
  }
  void shutdown() {}
  bool poll(InputState& out) {
    out = InputState();
    SceCtrlData pad;
    sceCtrlReadBufferPositive(&pad, 1);
    if (pad.Buttons & PSP_CTRL_LEFT) out.move_x -= 1.f;
    if (pad.Buttons & PSP_CTRL_RIGHT) out.move_x += 1.f;
    if (pad.Buttons & PSP_CTRL_UP) out.move_y -= 1.f;
    if (pad.Buttons & PSP_CTRL_DOWN) out.move_y += 1.f;
    out.move_x += (pad.Lx - 128) / 128.f;
    out.move_y += (pad.Ly - 128) / 128.f;
    out.attack = (pad.Buttons & PSP_CTRL_CROSS) != 0;
    out.interact = (pad.Buttons & PSP_CTRL_CIRCLE) != 0;
    out.inventory = (pad.Buttons & PSP_CTRL_TRIANGLE) != 0;
    out.reload = (pad.Buttons & PSP_CTRL_SQUARE) != 0;
    out.run = (pad.Buttons & PSP_CTRL_RTRIGGER) != 0;
    out.pause = (pad.Buttons & PSP_CTRL_START) != 0;
    out.confirm = (pad.Buttons & PSP_CTRL_CROSS) != 0;
    out.cancel = (pad.Buttons & PSP_CTRL_SELECT) != 0;
    return !out.cancel;
  }
  double time_seconds() {
    return sceKernelGetSystemTimeWide() / 1000000.0;
  }
  void begin_frame() {}
  void clear(float, float, float) {}
  void draw_rect(float, float, float, float, float, float, float, float) {}
  void draw_text(float, float, const char*, float, float, float) {}
  void end_frame() { sceDisplayWaitVblankStart(); }
  bool read_file(const char*, char**, std::size_t*) { return false; }
  bool write_file(const char*, const void*, std::size_t) { return false; }
};

IPlatform* create_platform() { return new PspPlatform(); }

#endif

} // namespace mdaz
