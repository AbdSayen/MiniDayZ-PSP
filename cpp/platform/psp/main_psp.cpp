// PSP entry — same mdaz::Game loop as desktop.
// Game MUST be heap-allocated: sizeof(Game) ≈ 840KB, PSP main stack is tiny.

#include "mdaz/game.hpp"
#include "mdaz/platform.hpp"

#if defined(MDAZ_PSP) && defined(__PSP__)
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <psppower.h>
#include <cstdio>
#include <cstring>
#include <new>

PSP_MODULE_INFO("MiniDAYZ", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
// Leave most of user RAM for heap (Game + fb). Stack only for main locals.
PSP_HEAP_SIZE_KB(18 * 1024);
PSP_MAIN_THREAD_STACK_SIZE_KB(64);

static int exit_callback(int, int, void*) {
  sceKernelExitGame();
  return 0;
}

static int callback_thread(SceSize, void*) {
  const int cbid = sceKernelCreateCallback("exit_cb", exit_callback, 0);
  sceKernelRegisterExitCallback(cbid);
  sceKernelSleepThreadCB();
  return 0;
}

static void setup_callbacks() {
  const int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
  if (thid >= 0) sceKernelStartThread(thid, 0, 0);
}

// Minimal boot splash before Game ctor (proves EBOOT started).
static void boot_splash(const char* msg) {
  // Platform not up yet — use debug screen if available later; no-op safe.
  (void)msg;
}
#endif

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

#if defined(MDAZ_PSP) && defined(__PSP__)
  setup_callbacks();
  scePowerSetClockFrequency(333, 333, 166);
  boot_splash("boot");
#endif

  mdaz::IPlatform* platform = mdaz::create_platform();
  if (!platform || !platform->init("Mini DAYZ", mdaz::kPspWidth, mdaz::kPspHeight)) {
#if defined(MDAZ_PSP) && defined(__PSP__)
    sceKernelExitGame();
#endif
    return 1;
  }

  // Heap — never stack. OOM here → clean exit instead of black reboot.
  mdaz::Game* game = new (std::nothrow) mdaz::Game();
  if (!game) {
    platform->clear(0.4f, 0.f, 0.f);
    platform->draw_text(80.f, 120.f, "OUT OF MEMORY", 1.f, 1.f, 1.f);
    platform->end_frame();
    for (int i = 0; i < 180; ++i) {
#if defined(MDAZ_PSP) && defined(__PSP__)
      sceDisplayWaitVblankStart();
#endif
    }
    platform->shutdown();
    delete platform;
#if defined(MDAZ_PSP) && defined(__PSP__)
    sceKernelExitGame();
#endif
    return 2;
  }

  // Slim data next to EBOOT only — do NOT ship original_docs/images (1000+ files).
  const char* data_dir = "data";
  game->init(platform, data_dir);

  double prev = platform->time_seconds();
  while (!game->wants_quit()) {
    double now = platform->time_seconds();
    float dt = (float)(now - prev);
    prev = now;
    if (dt < 0.f) dt = 0.f;
    if (dt > 0.1f) dt = 0.1f;
    game->update(dt);
    game->render();
  }

  game->shutdown();
  delete game;
  platform->shutdown();
  delete platform;

#if defined(MDAZ_PSP) && defined(__PSP__)
  sceKernelExitGame();
#endif
  return 0;
}
