#include "mdaz/game.hpp"
#include "mdaz/platform.hpp"
#include "mdaz/platform_null.hpp"

#include <cstdio>

int main(int argc, char** argv) {
  const char* data_dir = "data";
  if (argc > 1) data_dir = argv[1];

  mdaz::IPlatform* platform = mdaz::create_platform();
  if (!platform) {
    std::fprintf(stderr, "Failed to create platform\n");
    return 1;
  }

  // PSP native resolution for layout testing on desktop
  if (!platform->init("Mini DAYZ C++ (1.4.1)", mdaz::kPspWidth, mdaz::kPspHeight)) {
    std::fprintf(stderr, "Platform init failed — falling back to null\n");
    platform->shutdown();
    delete platform;
    platform = mdaz::create_null_platform();
    platform->init("Mini DAYZ C++", mdaz::kPspWidth, mdaz::kPspHeight);
  }

  mdaz::Game game;
  if (!game.init(platform, data_dir)) {
    std::fprintf(stderr, "Game init failed\n");
    platform->shutdown();
    delete platform;
    return 1;
  }

  double prev = platform->time_seconds();
  while (!game.wants_quit()) {
    double now = platform->time_seconds();
    float dt = static_cast<float>(now - prev);
    prev = now;
    if (dt < 0.f) dt = 0.f;
    if (dt > 0.1f) dt = 0.1f;
    game.update(dt);
    game.render();
  }

  game.shutdown();
  platform->shutdown();
  delete platform;
  return 0;
}
