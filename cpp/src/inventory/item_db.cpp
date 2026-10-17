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
    // PLACEHOLDER restores until extracted from C2 — marked known_stats=false
    d.water_restore = n.find("Energy") != std::string::npos ? 20.f : 25.f;
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
    d.food_restore = 20.f;
    d.stack_max = 5;
  } else {
    d.category = ItemCat_Misc;
  }

  // Candidate restores from extract_consumable_stats / item_id_effects (not full sverka).
  // known_stats=true only when a clear IV13/14/15 literal was seen for that item.
  if (d.id == 25) { // Whiskey — IV14≈20/25 water-like restores in events
    d.category = ItemCat_Drink;
    d.water_restore = 20.f;
    d.food_restore = 5.f; // mild caloric bump from alcohol events
    d.known_stats = true;
  } else if (d.id == 28) { // Saline bag
    d.category = ItemCat_Medical;
    d.heal_amount = 25.f;
    d.known_stats = true;
  } else if (d.id == 19) { // Blood bag
    d.category = ItemCat_Medical;
    d.heal_amount = 45.f;
    d.known_stats = true;
  } else if (d.id == 18) { // Bandage
    d.category = ItemCat_Medical;
    d.known_stats = true;
  } else if (d.id == 45) { // Morphine
    d.category = ItemCat_Medical;
    d.heal_amount = 5.f;
    d.known_stats = true;
  } else if (d.id == 34) { // Heatpack
    d.category = ItemCat_Misc;
    d.known_stats = true;
  } else if (d.id == 30) { // Nota-Cola
    d.category = ItemCat_Drink;
    d.water_restore = 30.f;
  } else if (d.id == 29) { // Vitamins — mild infection clear chance + heal
    d.category = ItemCat_Medical;
    d.heal_amount = 5.f;
    d.known_stats = true;
  } else if (d.id == 46) { // Tetracycline — clears infection
    d.category = ItemCat_Medical;
    d.known_stats = true;
  } else if (d.id == 20) { // Flare
    d.category = ItemCat_Misc;
  } else if (d.id == 265) { // NVG
    d.category = ItemCat_Misc;
  }

  // Common food/drink placeholders (not sverka — better than generic 20)
  if (d.id == 1) { d.category = ItemCat_Food; d.food_restore = 35.f; }      // Canned beans
  else if (d.id == 2) { // Canned spaghetti — item_id_effects IV13+30 IV14+5
    d.category = ItemCat_Food;
    d.food_restore = 30.f;
    d.water_restore = 5.f;
    d.known_stats = true;
  } else if (d.id == 3) { d.category = ItemCat_Food; d.food_restore = 28.f; } // Tactical bacon
  else if (d.id == 4) { // Rice — item_id_effects IV14+22
    d.category = ItemCat_Food;
    d.water_restore = 22.f;
    d.known_stats = true;
  } else if (d.id == 5) { d.category = ItemCat_Food; d.food_restore = 22.f; } // Tomato
  else if (d.id == 6) { // Apple — item_id_effects IV13+7 IV14+8
    d.category = ItemCat_Food;
    d.food_restore = 7.f;
    d.water_restore = 8.f;
    d.known_stats = true;
  } else if (d.id == 7) { d.category = ItemCat_Food; d.food_restore = 18.f; } // Banana
  else if (d.id == 8) { // Pipsi — item_id_effects IV14+20 (dominant drink restore)
    d.category = ItemCat_Drink;
    d.water_restore = 20.f;
    d.known_stats = true;
  } else if (d.id == 9) { d.category = ItemCat_Drink; d.water_restore = 25.f; } // Spite
  else if (d.id == 10) { // Kvas — item_id_effects IV13+10 IV14+10
    d.category = ItemCat_Drink;
    d.water_restore = 10.f;
    d.food_restore = 10.f;
    d.known_stats = true;
  }
  else if (d.id == 54) { d.category = ItemCat_Drink; d.water_restore = 40.f; } // Water bottle
  else if (d.id == 55) { d.category = ItemCat_Drink; d.water_restore = 50.f; } // Canteen
  else if (d.id == 85) { d.category = ItemCat_Food; d.food_restore = 55.f; } // MRE ration
  else if (d.id == 97) { d.category = ItemCat_Food; d.food_restore = 18.f; } // Mushroom — risky
  else if (d.id == 21) { d.category = ItemCat_Misc; } // F1 grenade
  else if (d.id == 102) { d.category = ItemCat_Misc; } // Molotov
  else if (d.id == 456) { d.category = ItemCat_Misc; } // Tent
  else if (d.id == 250) { d.category = ItemCat_Clothing; } // Gasmask
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
