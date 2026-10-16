#include "mdaz/item_db.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

namespace mdaz {

ItemDef::ItemDef()
  : id(0), category(ItemCat_Unknown),
    food_restore(0), water_restore(0), heal_amount(0),
    stack_max(1), known_stats(false) {}

void ItemDatabase::classify(ItemDef& d) {
  const std::string& n = d.name;
  // Heuristic categories from English names (display only until C2 stats extracted).
  if (n.find("ammo") != std::string::npos || n.find("Ammo") != std::string::npos) {
    d.category = ItemCat_Ammo;
    d.stack_max = 30;
  } else if (n.find("Bandage") != std::string::npos || n.find("Blood bag") != std::string::npos ||
             n.find("Saline") != std::string::npos || n.find("Morphine") != std::string::npos ||
             n.find("Tetracycline") != std::string::npos || n.find("Vitamins") != std::string::npos) {
    d.category = ItemCat_Medical;
  } else if (n.find("pistol") != std::string::npos || n.find("rifle") != std::string::npos ||
             n.find("shotgun") != std::string::npos || n.find("carbine") != std::string::npos ||
             n.find("Nagant") != std::string::npos || n.find("bow") != std::string::npos ||
             n.find("Crossbow") != std::string::npos || n.find("AK") != std::string::npos ||
             n.find("M4") != std::string::npos || n.find("SVD") != std::string::npos ||
             n.find("Glock") != std::string::npos || n.find("AUG") != std::string::npos) {
    d.category = ItemCat_Weapon;
  } else if (n.find("knife") != std::string::npos || n.find("axe") != std::string::npos ||
             n.find("bat") != std::string::npos || n.find("Crowbar") != std::string::npos ||
             n.find("Shovel") != std::string::npos || n.find("Sword") != std::string::npos ||
             n.find("Pitchfork") != std::string::npos || n.find("Sledge") != std::string::npos ||
             n.find("Hatchet") != std::string::npos || n.find("Pipewrench") != std::string::npos) {
    d.category = ItemCat_Melee;
  } else if (n.find("backpack") != std::string::npos || n.find("bag") != std::string::npos ||
             n.find("Backpack") != std::string::npos || n.find("Taloon") != std::string::npos ||
             n.find("Tortilla") != std::string::npos) {
    d.category = ItemCat_Backpack;
  } else if (n.find("pants") != std::string::npos || n.find("jacket") != std::string::npos ||
             n.find("shirt") != std::string::npos || n.find("Hoodie") != std::string::npos ||
             n.find("vest") != std::string::npos || n.find("helmet") != std::string::npos ||
             n.find("hat") != std::string::npos || n.find("Jeans") != std::string::npos) {
    d.category = ItemCat_Clothing;
  } else if (n.find("Pipsi") != std::string::npos || n.find("Spite") != std::string::npos ||
             n.find("Kvas") != std::string::npos || n.find("Cola") != std::string::npos ||
             n.find("Whiskey") != std::string::npos || n.find("Canteen") != std::string::npos ||
             n.find("Water bottle") != std::string::npos || n.find("drink") != std::string::npos) {
    d.category = ItemCat_Drink;
    // PLACEHOLDER restores until extracted from C2 — marked known_stats=false
    d.water_restore = 25.f;
  } else if (n.find("Canned") != std::string::npos || n.find("Apple") != std::string::npos ||
             n.find("Banana") != std::string::npos || n.find("Rice") != std::string::npos ||
             n.find("bacon") != std::string::npos || n.find("steak") != std::string::npos ||
             n.find("Tomato") != std::string::npos || n.find("Orange") != std::string::npos ||
             n.find("MRE") != std::string::npos || n.find("berry") != std::string::npos ||
             n.find("Berry") != std::string::npos || n.find("meat") != std::string::npos ||
             n.find("fillet") != std::string::npos || n.find("Pepper") != std::string::npos ||
             n.find("Zucchini") != std::string::npos || n.find("Cranberry") != std::string::npos) {
    d.category = ItemCat_Food;
    d.food_restore = 20.f;
    d.stack_max = 5;
  } else {
    d.category = ItemCat_Misc;
  }
}

static const char* find_key(const char* p, const char* key) {
  return std::strstr(p, key);
}

bool ItemDatabase::load_from_json_string(const char* json) {
  items_.clear();
  if (!json) return false;

  const char* p = json;
  while ((p = find_key(p, "\"id\"")) != 0) {
    p = std::strchr(p, ':');
    if (!p) break;
    ++p;
    ItemDef def;
    def.id = static_cast<ItemId>(std::atoi(p));

    const char* name_key = find_key(p, "\"name\"");
    if (!name_key) break;
    const char* q1 = std::strchr(name_key + 6, '\"');
    if (!q1) break;
    ++q1;
    const char* q2 = std::strchr(q1, '\"');
    if (!q2) break;
    def.name.assign(q1, q2);
    classify(def);
    items_.push_back(def);
    p = q2;
  }
  return !items_.empty();
}

bool ItemDatabase::load_from_json_file(const char* path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return false;
  std::ostringstream ss;
  ss << in.rdbuf();
  return load_from_json_string(ss.str().c_str());
}

const ItemDef* ItemDatabase::find(ItemId id) const {
  for (std::size_t i = 0; i < items_.size(); ++i) {
    if (items_[i].id == id) return &items_[i];
  }
  return 0;
}

} // namespace mdaz
