#include "mdaz/item_db.hpp"
#include "mdaz/inventory.hpp"
#include "mdaz/player_worn_visual.hpp"

#include <cstdio>
#include <cstring>

int test_player_visual() {
  std::printf("--- test_player_visual ---\n");
  int fail = 0;

  if (!mdaz::item_is_worn_visual_layer(251, mdaz::WornVisual_Head) ||
      !mdaz::item_is_worn_visual_layer(400, mdaz::WornVisual_Vest) ||
      !mdaz::item_is_worn_visual_layer(355, mdaz::WornVisual_Outerwear) ||
      mdaz::item_is_worn_visual_layer(303, mdaz::WornVisual_Outerwear)) {
    std::printf("FAIL worn visual layer ids\n");
    ++fail;
  } else {
    std::printf("OK   worn visual layer ids\n");
  }

  const char* base = mdaz::worn_item_sprite_base(251);
  if (!base || std::strcmp(base, "helmet_army") != 0) {
    std::printf("FAIL worn_item_sprite_base helmet\n");
    ++fail;
  } else {
    std::printf("OK   worn_item_sprite_base\n");
  }

  mdaz::ItemDatabase db;
  db.load_from_json_string(
    "{\"items\":[{\"id\":251,\"name\":\"Army helmet\",\"category\":\"clothing\"},"
    "{\"id\":400,\"name\":\"Molle vest\",\"category\":\"clothing\"}]}");
  mdaz::Inventory inv;
  inv.add_item(251, 1, db);
  inv.add_item(400, 1, db);
  (void)inv.wear_clothing(0, db);
  (void)inv.wear_clothing(1, db);
  if (mdaz::worn_visual_for_layer(inv, mdaz::WornVisual_Head) != 251 ||
      mdaz::worn_visual_for_layer(inv, mdaz::WornVisual_Vest) != 400) {
    std::printf("FAIL worn_visual_for_layer\n");
    ++fail;
  } else {
    std::printf("OK   worn_visual_for_layer\n");
  }

  return fail ? 1 : 0;
}
