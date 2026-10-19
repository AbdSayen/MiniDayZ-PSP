#pragma once

namespace mdaz {

class IPlatform;
class Game;

void draw_debug_hud(IPlatform& platform, const Game& game);

// C2 Show_unlock_wpn (obj551 text + dictionary frame 398).
void draw_weapon_unlock_toast(IPlatform& platform, const Game& game);

} // namespace mdaz
