// PSP entry — stub until PSPSDK is available on the build machine.
// Maps to the same mdaz::Game loop as desktop.

#include "mdaz/game.hpp"
#include "mdaz/platform.hpp"

extern "C" int SDL_main(int, char**) { return 0; } // silence if mixed

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  mdaz::IPlatform* platform = mdaz::create_platform();
  if (!platform || !platform->init("Mini DAYZ", mdaz::kPspWidth, mdaz::kPspHeight)) {
    return 1;
  }
  mdaz::Game game;
  game.init(platform, "data");
  double prev = platform->time_seconds();
  while (!game.wants_quit()) {
    double now = platform->time_seconds();
    float dt = (float)(now - prev);
    prev = now;
    if (dt > 0.1f) dt = 0.1f;
    game.update(dt);
    game.render();
  }
  game.shutdown();
  platform->shutdown();
  delete platform;
  return 0;
}
