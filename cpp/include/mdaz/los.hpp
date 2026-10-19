#pragma once
// C2 LineOfSight behaviours (zed MainLook/ShortLook, screamer/wolf/dog LineOfSight, bot Eyes/Gun)
// all add obstacles t927 (every *_int building, Solid polygon) + t339 (big_obstacle_base).
// The Game installs the world test; AI code asks it before seeing / attacking the player.
#include "mdaz/types.hpp"

namespace mdaz {

typedef bool (*LosBlockedFn)(const void* ctx, Vec2 a, Vec2 b);
void set_los_blocker(LosBlockedFn fn, const void* ctx);
// True if the straight line a->b crosses a building wall or big obstacle.
bool los_blocked(Vec2 a, Vec2 b);

} // namespace mdaz
