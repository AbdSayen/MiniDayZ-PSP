#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

class Inventory;
class LootSystem;

// C2 TD_drop_weapons: spawn ground weapon from player weapons IV93[0] (Buildings + Bullet at drop).
void td_drop_weapons(LootSystem& loot, Vec2 player_pos, ItemId equipped_weapon_id, int count);

} // namespace mdaz
