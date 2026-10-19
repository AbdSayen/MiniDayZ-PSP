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
             n.find("hat") != std::string::npos || n.find("Jeans") != std::string::npos ||
             n.find("Bandana") != std::string::npos || n.find("Helm") != std::string::npos ||
             n.find("Cloak") != std::string::npos || n.find("Dress") != std::string::npos ||
             n.find("Sweater") != std::string::npos) {
    d.category = ItemCat_Clothing;
  } else if (n.find("Pipsi") != std::string::npos || n.find("Spite") != std::string::npos ||
             n.find("Kvas") != std::string::npos || n.find("Cola") != std::string::npos ||
             n.find("Whiskey") != std::string::npos || n.find("Canteen") != std::string::npos ||
             n.find("Water bottle") != std::string::npos || n.find("Energy drink") != std::string::npos ||
             n.find("drink") != std::string::npos) {
    d.category = ItemCat_Drink;
    // Restores only from item_activate IV literals (id overrides below) — no placeholders.
  } else if (n.find("Canned") != std::string::npos || n.find("Apple") != std::string::npos ||
             n.find("Banana") != std::string::npos || n.find("Rice") != std::string::npos ||
             n.find("bacon") != std::string::npos || n.find("steak") != std::string::npos ||
             n.find("Tomato") != std::string::npos || n.find("Orange") != std::string::npos ||
             n.find("MRE") != std::string::npos || n.find("berry") != std::string::npos ||
             n.find("Berry") != std::string::npos || n.find("meat") != std::string::npos ||
             n.find("fillet") != std::string::npos || n.find("Pepper") != std::string::npos ||
             n.find("Zucchini") != std::string::npos || n.find("Cranberry") != std::string::npos ||
             n.find("Perch") != std::string::npos || n.find("fillet") != std::string::npos ||
             n.find("fish") != std::string::npos) {
    d.category = ItemCat_Food;
    d.stack_max = 5;
    // food_restore only from item_activate IV13/14 (id overrides below).
  } else {
    d.category = ItemCat_Misc;
  }

  // Consumable restores: sverka from item_activate (see cpp/docs_gen/item_activate_iv_sverka.json).
  // known_stats=true only when item_activate has literal IV13/14/15/16 Add/Set on player obj172.
  if (d.id == 1) {
    d.category = ItemCat_Food;
    d.food_restore = 30.f;
    d.water_restore = 5.f;
    d.known_stats = true;
  } else if (d.id == 2) {
    d.category = ItemCat_Food;
    d.food_restore = 20.f;
    d.known_stats = true;
  } else if (d.id == 3) {
    d.category = ItemCat_Food;
    d.food_restore = 35.f;
    d.known_stats = true;
  } else if (d.id == 4) {
    d.category = ItemCat_Food;
    d.food_restore = 65.f;
    d.known_stats = true;
  } else if (d.id == 5 || d.id == 6 || d.id == 7) {
    d.category = ItemCat_Food;
    d.food_restore = 15.f;
    d.water_restore = 5.f;
    d.known_stats = true;
  } else if (d.id == 8 || d.id == 9 || d.id == 30) {
    d.category = ItemCat_Drink;
    d.water_restore = 30.f;
    d.known_stats = true;
  } else if (d.id == 10) {
    d.category = ItemCat_Drink;
    d.water_restore = 50.f;
    d.known_stats = true;
  } else if (d.id == 18) {
    d.category = ItemCat_Medical;
    d.known_stats = true; // stops bleed — no IV13/14/15 literal
  } else if (d.id == 19) {
    d.category = ItemCat_Medical;
    d.heal_amount = 50.f;
    d.known_stats = true;
  } else if (d.id == 20) {
    d.category = ItemCat_Misc;
    d.known_stats = true; // IV26 torch — consumable_item_activate_sverka.json
  } else if (d.id == 25) {
    d.category = ItemCat_Drink;
    d.water_restore = 20.f;
    d.known_stats = true;
  } else if (d.id == 27) {
    d.category = ItemCat_Food;
    d.food_restore = 15.f;
    d.water_restore = 15.f;
    d.known_stats = true;
  } else if (d.id == 28) {
    d.category = ItemCat_Medical;
    d.heal_amount = 25.f;
    d.known_stats = true;
  } else if (d.id == 29) {
    d.category = ItemCat_Medical;
    d.known_stats = true; // IV32 buff 60s — item_activate_medicals_sverka.json
  } else if (d.id == 31) {
    d.category = ItemCat_Food;
    d.food_restore = 20.f;
    d.water_restore = 20.f;
    d.known_stats = true;
  } else if (d.id == 32) {
    d.category = ItemCat_Food;
    d.food_restore = 20.f;
    d.water_restore = 5.f;
    d.known_stats = true;
  } else if (d.id == 33) {
    d.category = ItemCat_Food;
    d.food_restore = 15.f;
    d.water_restore = 15.f;
    d.known_stats = true;
  } else if (d.id == 34) {
    d.category = ItemCat_Misc;
    d.known_stats = true;
  } else if (d.id == 45) {
    d.category = ItemCat_Medical;
    // No item_activate CompareParam 45 in C2 export (blocked)
  } else if (d.id == 46) {
    d.category = ItemCat_Medical;
    d.known_stats = true; // clears infection IV34
  } else if (d.id == 54) {
    d.category = ItemCat_Drink;
    d.water_restore = 12.5f;
    d.known_stats = true;
  } else if (d.id == 55) {
    d.category = ItemCat_Drink;
    d.water_restore = 25.f;
    d.known_stats = true;
  } else if (d.id == 60 || d.id == 71 || d.id == 91 || d.id == 92 || d.id == 93 || d.id == 94) {
    d.category = ItemCat_Food; // cook-only / no eat IV in item_activate
  } else if (d.id == 61) {
    d.category = ItemCat_Food;
    d.food_restore = 50.f;
    d.water_restore = 10.f;
    d.heal_amount = 15.f;
    d.known_stats = true;
  } else if (d.id == 62) {
    d.category = ItemCat_Drink;
    d.water_restore = 100.f;
    d.known_stats = true;
  } else if (d.id == 63) {
    d.category = ItemCat_Drink;
    d.water_restore = 20.f;
    d.known_stats = true;
  } else if (d.id == 72) {
    d.category = ItemCat_Food;
    d.food_restore = 35.f;
    d.water_restore = 10.f;
    d.heal_amount = 30.f;
    d.known_stats = true;
  } else if (d.id == 73) {
    d.category = ItemCat_Food;
    d.food_restore = 10.f;
    d.water_restore = 10.f;
    d.known_stats = true;
  } else if (d.id == 74) {
    d.category = ItemCat_Food;
    d.food_restore = 10.f;
    d.water_restore = 20.f;
    d.known_stats = true;
  } else if (d.id == 85) {
    d.category = ItemCat_Food;
    d.food_restore = 40.f;
    d.water_restore = 40.f;
    d.known_stats = true;
  } else if (d.id == 95 || d.id == 96) {
    d.category = ItemCat_Food;
    d.food_restore = 50.f;
    d.water_restore = 10.f;
    d.heal_amount = 15.f;
    d.known_stats = true;
  } else if (d.id == 35) {
    d.category = ItemCat_Misc;
    d.known_stats = true;
  } else if (d.id == 36) {
    d.category = ItemCat_Misc;
    d.known_stats = true;
  } else if (d.id == 265) {
    d.category = ItemCat_Misc;
  } else if (d.id == 97) {
    d.category = ItemCat_Food; // easter egg — not a food restore
  } else if (d.id == 21) {
    d.category = ItemCat_Misc;
  } else if (d.id == 102) {
    d.category = ItemCat_Misc;
  } else if (d.id == 456) {
    d.category = ItemCat_Misc;
  } else if (d.id == 250) {
    d.category = ItemCat_Clothing;
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
