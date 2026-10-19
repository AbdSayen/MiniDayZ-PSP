// PSP entry — same mdaz::Game loop as desktop.
// Game MUST be heap-allocated: sizeof(Game) ≈ 840KB, PSP main stack is tiny.

#include "mdaz/game.hpp"
#include "mdaz/platform.hpp"

#if defined(MDAZ_PSP) && defined(__PSP__)
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspiofilemgr.h>
#include <psppower.h>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <new>

PSP_MODULE_INFO("MiniDAYZ", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
// All free user RAM minus 1 MB for the kernel/stack (was a fixed 18 MB). Logged at boot.
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_STACK_SIZE_KB(64);

// ---- Boot log: ms0:/PSP/GAME/MINIDAYZ/log.txt (fallback ./log.txt). Each line is written
// with open/append/close so it survives a crash or a forced power-off.
static char s_log_path[128] = "ms0:/PSP/GAME/MINIDAYZ/log.txt";
static unsigned long long s_log_t0 = 0;
extern "C" void mdaz_boot_log(const char* fmt, ...) {
  char line[512];
  unsigned ms = 0;
  {
    const unsigned long long now = (unsigned long long)sceKernelGetSystemTimeWide();
    if (s_log_t0 == 0) s_log_t0 = now;
    ms = (unsigned)((now - s_log_t0) / 1000ull);
  }
  int n = std::snprintf(line, sizeof(line), "[%6u ms] ", ms);
  va_list ap;
  va_start(ap, fmt);
  n += std::vsnprintf(line + n, sizeof(line) - n - 2, fmt, ap);
  va_end(ap);
  if (n > (int)sizeof(line) - 2) n = (int)sizeof(line) - 2;
  line[n++] = '\n';
  line[n] = 0;
  SceUID fd = sceIoOpen(s_log_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
  if (fd < 0) {
    std::strcpy(s_log_path, "log.txt");
    fd = sceIoOpen(s_log_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
  }
  if (fd < 0) return;
  sceIoWrite(fd, line, n);
  sceIoClose(fd);
}

static void boot_log_reset() {
  SceUID fd = sceIoOpen(s_log_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
  if (fd < 0) {
    std::strcpy(s_log_path, "log.txt");
    fd = sceIoOpen(s_log_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
  }
  if (fd >= 0) sceIoClose(fd);
}

static void log_mem(const char* when) {
  mdaz_boot_log("mem %s: max free block %d KB, total free %d KB", when,
                (int)(sceKernelMaxFreeMemSize() / 1024), (int)(sceKernelTotalFreeMemSize() / 1024));
}

// Visible screen before any asset load (plain colour + built-in font; no sprites needed).
static void boot_splash(mdaz::IPlatform* p, const char* line2, float r, float g, float b) {
  p->begin_frame();
  p->clear(r, g, b);
  p->draw_rect(0.f, 100.f, 480.f, 60.f, 0.f, 0.f, 0.f, 0.6f);
  p->draw_text(150.f, 112.f, "MINI DAYZ C++  PSP BUILD", 1.f, 0.95f, 0.7f);
  p->draw_text(150.f, 130.f, line2, 0.9f, 0.9f, 0.9f);
  p->draw_text(150.f, 146.f, "log: ms0:/PSP/GAME/MINIDAYZ/log.txt", 0.7f, 0.8f, 0.7f);
  p->end_frame();
}

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

static void io_log(const char* msg) {
  SceUID fd = sceIoOpen("ms0:/PSP/GAME/MINIDAYZ/mdaz_io.log",
                        PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
  if (fd < 0) fd = sceIoOpen("mdaz_io.log", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
  if (fd < 0) return;
  sceIoWrite(fd, msg, (int)std::strlen(msg));
  sceIoClose(fd);
}

static int pak_exists_at(const char* path) {
  SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0777);
  if (fd >= 0) { sceIoClose(fd); return 1; }
  return 0;
}

static int pak_exists_here() {
  return pak_exists_at("data/sprites.pak");
}

// XMB/PPSSPP often leave cwd elsewhere — find the folder that contains data/sprites.pak.
static void chdir_to_eboot(const char* argv0) {
  char line[640];
  std::snprintf(line, sizeof(line), "boot argv0=%s\n", argv0 ? argv0 : "(null)");
  io_log(line);

  // Absolute pak probes first (no chdir needed).
  static const char* kAbsPak[] = {
    "ms0:/PSP/GAME/MINIDAYZ/data/sprites.pak",
    "ms0:/PSP/GAME/minidayz/data/sprites.pak",
    "ms0:/PSP/GAME/MiniDAYZ/data/sprites.pak",
    "host0:/data/sprites.pak",
    "umd0:/data/sprites.pak",
    "disc0:/PSP_GAME/USRDIR/data/sprites.pak",
    0
  };
  for (int i = 0; kAbsPak[i]; ++i) {
    if (pak_exists_at(kAbsPak[i])) {
      // Chdir to the game root (strip /data/sprites.pak).
      char dir[512];
      std::strncpy(dir, kAbsPak[i], sizeof(dir) - 1);
      dir[sizeof(dir) - 1] = 0;
      char* slash = std::strrchr(dir, '/'); // sprites.pak
      if (slash) {
        *slash = 0;
        slash = std::strrchr(dir, '/'); // data
        if (slash) *slash = 0;
      }
      if (dir[0]) sceIoChdir(dir);
      std::snprintf(line, sizeof(line), "chdir_abs ok %s cwd=%s\n", kAbsPak[i], dir);
      io_log(line);
      return;
    }
  }

  if (argv0 && argv0[0]) {
    char dir[512];
    std::strncpy(dir, argv0, sizeof(dir) - 1);
    dir[sizeof(dir) - 1] = 0;
    for (char* p = dir; *p; ++p)
      if (*p == '\\') *p = '/';
    char* slash = std::strrchr(dir, '/');
    if (slash) {
      *slash = 0;
      if (dir[0]) {
        sceIoChdir(dir);
        if (pak_exists_here()) {
          std::snprintf(line, sizeof(line), "chdir_argv ok %s\n", dir);
          io_log(line);
          return;
        }
      }
    }
  }
  static const char* kDirs[] = {
    "ms0:/PSP/GAME/MINIDAYZ",
    "ms0:/PSP/GAME/minidayz",
    "ms0:/PSP/GAME/MiniDAYZ",
    "host0:",
    "umd0:",
    ".",
    0
  };
  for (int i = 0; kDirs[i]; ++i) {
    sceIoChdir(kDirs[i]);
    if (pak_exists_here()) {
      std::snprintf(line, sizeof(line), "chdir_list ok %s\n", kDirs[i]);
      io_log(line);
      return;
    }
  }
  io_log("chdir FAIL no sprites.pak\n");
}
#endif

int main(int argc, char** argv) {
#if defined(MDAZ_PSP) && defined(__PSP__)
  boot_log_reset();
  mdaz_boot_log("boot start argc=%d argv0=%s", argc, (argc > 0 && argv && argv[0]) ? argv[0] : "(null)");
  log_mem("at start");
  setup_callbacks();
  scePowerSetClockFrequency(333, 333, 166);
  chdir_to_eboot((argc > 0 && argv) ? argv[0] : 0);
  mdaz_boot_log("chdir done (probe details in mdaz_io.log)");
#else
  (void)argc;
  (void)argv;
#endif

  mdaz::IPlatform* platform = mdaz::create_platform();
  if (!platform || !platform->init("Mini DAYZ", mdaz::kPspWidth, mdaz::kPspHeight)) {
#if defined(MDAZ_PSP) && defined(__PSP__)
    mdaz_boot_log("platform init FAILED");
    sceKernelExitGame();
#endif
    return 1;
  }
#if defined(MDAZ_PSP) && defined(__PSP__)
  mdaz_boot_log("platform init ok");
  boot_splash(platform, "LOADING...", 0.10f, 0.22f, 0.35f);
  mdaz_boot_log("splash drawn");
  mdaz_boot_log("sizeof(Game)=%u", (unsigned)sizeof(mdaz::Game));
  log_mem("before Game alloc");
#endif

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

#if defined(MDAZ_PSP) && defined(__PSP__)
  mdaz_boot_log("Game allocated %p", (void*)game);
  log_mem("after Game alloc");
#endif
  const bool init_ok = game->init(platform, "data");
#if defined(MDAZ_PSP) && defined(__PSP__)
  mdaz_boot_log("game init %s", init_ok ? "ok" : "FAILED");
  log_mem("after game init");
  if (!init_ok) {
    boot_splash(platform, "INIT FAILED - SEE LOG.TXT", 0.45f, 0.05f, 0.05f);
    for (int i = 0; i < 600; ++i) sceDisplayWaitVblankStart();
  }
  bool first_frame_logged = false;
#else
  (void)init_ok;
#endif

  double prev = platform->time_seconds();
  while (!game->wants_quit()) {
    double now = platform->time_seconds();
    float dt = (float)(now - prev);
    prev = now;
    if (dt < 0.f) dt = 0.f;
    if (dt > 0.1f) dt = 0.1f;
    platform->profile_mark(mdaz::IPlatform::Prof_Logic);
    game->update(dt);
    game->render();
#if defined(MDAZ_PSP) && defined(__PSP__)
    if (!first_frame_logged) { mdaz_boot_log("first game frame drawn"); first_frame_logged = true; }
#endif
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
