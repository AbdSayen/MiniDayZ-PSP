#include "mdaz/hud.hpp"
#include "mdaz/game.hpp"
#include "mdaz/platform.hpp"

#include <cstdio>

namespace mdaz {

void draw_debug_hud(IPlatform& platform, const Game& game) {
  const Player& p = game.player();
  char buf[128];

  std::snprintf(buf, sizeof(buf), "HP %.0f  Food %.0f  Water %.0f  Temp %.0f",
                p.survival.blood, p.survival.food, p.survival.water, p.survival.temperature);
  platform.draw_text(8, 8, buf, 0.95f, 0.95f, 0.9f);

  std::snprintf(buf, sizeof(buf), "Bleed %.1f  DayPhase %.2f  Zeds %d  Kills %d",
                p.survival.bleeding, game.weather().day_phase,
                game.zombies().alive_count(), p.zeds_killed);
  platform.draw_text(8, 24, buf, 0.8f, 0.85f, 0.75f);

  std::snprintf(buf, sizeof(buf), "Pos %.0f,%.0f  X:attack  C:use item  Shift:run",
                p.pos.x, p.pos.y);
  platform.draw_text(8, 250, buf, 0.65f, 0.7f, 0.65f);
}

} // namespace mdaz
