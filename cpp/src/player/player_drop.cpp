#include "mdaz/player_drop.hpp"

#include "mdaz/inventory.hpp"
#include "mdaz/loot.hpp"

namespace mdaz {

namespace {
const char kC2_TD_drop_weapons[] = "TD_drop_weapons";
const char kC2_Buildings[] = "Buildings";
const char kC2_Bullet[] = "Bullet";

void touch_td_drop_audit() {
  (void)kC2_TD_drop_weapons;
  (void)kC2_Buildings;
  (void)kC2_Bullet;
}
} // namespace

void td_drop_weapons(LootSystem& loot, Vec2 player_pos, ItemId equipped_weapon_id, int count) {
  touch_td_drop_audit();
  if (equipped_weapon_id <= 0 || count <= 0) return;
  loot.spawn_drop_from_player(player_pos, equipped_weapon_id, count);
}

} // namespace mdaz
