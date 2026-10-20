#include "mdaz/ui_strings.hpp"

#include "mdaz/platform.hpp"

#include <cstdio>
#include <cstring>
#include <map>
#include <string>

namespace mdaz {

namespace {

struct Fallback {
  int id;
  const char* text;
};

// Verified ids from docs/l_eng_ui.xml (quest pad / HUD task copy).
const Fallback kFallbacks[] = {
  {371, "A group of survivors is looking for this item:"},
  {372, "Hey, we need help! A group of bandits set up a camp not far from here. "
        "Eliminate them and we will reward you."},
  {373, "Did you find what we are looking for?"},
  {377, "Hey! We got ambushed and lost our loot case. If you bring it back to us, "
        "we will reward you."},
  {379, "Did you find our loot case?"},
  {381, "Reward for this task is:"},
  {387, "Current task failed."},
  {367, "No active task at this moment."},
  {368, "Coordinates where a drivable vehicle is located"},
  {369, "Coordinates of an underground bunker entrance"},
  {370, "Coordinates of an interesting place"},
  {376, "Coordinates of a boat we saw on the east coast."},
  {382, "A group of survivors has lost their loot case. I can find it and bring it back, or I can open it and steal all the stuff."},
  {383, "A group of survivors asked me to deal with bandits."},
  {384, "Time left:"},
  {386, "Bandits eliminated. I need to get back to survivors for my reward."},
  // item 10: perk menu (l_eng_ui.xml 337, 400..415, 536)
  {337, "XP "},
  {400, "Sharpshooter\nIncreases a chance of headshot with a ranged weapon."},
  {401, "Scout\nYou can spot enemy survivors from a greater distance."},
  {402, "Sprinter\nRun speed increases by 10%."},
  {403, "Camel\nThirst rate decreases by 15%."},
  {404, "Hamster\nHunger rate decreases by 15%."},
  {405, "Snowborn\nCold resistance increases by 20%."},
  {406, "The Bullet Farmer\nEach ammo stack you find contains +3 bullets."},
  {407, "Survivalist\nRegeneration speed doubles if your blood drops under 50%."},
  {408, "Blocker\nIncreases chance to block enemy attacks in melee fight."},
  {409, "Puncher\nIncreases chance to deal multiple damage attack in melee fight."},
  {410, "Trunk Digger\nIncreases chance to find loot in car trunks."},
  {411, "Hunter\nAnimals have a smaller chance to spot the player."},
  {412, "Sweet home\nStaying inside buildings adds 50% more heat."},
  {413, "Red+\nLowers the blood loss when the player is bleeding or feels sick."},
  {414, "Nightcrawler\nYou see a little better in the night."},
  {415, "Agronomist\nYou gather more berries. None of your plants will ever wither."},
  {536, "Score to unlock: "},
};

// User-requested PSP wording for l_eng_ui.xml lines that name touch controls
// (KNOWN_ISSUES "PSP hint wording"). Applied after the XML is loaded.
const Fallback kPspOverrides[] = {
  {33, "Movement: analog stick / D-pad"},
  {230, "With ranged weapons, aiming takes place automatically. Shoot by pressing Cross. Shooting while running is less accurate than while standing."},
  {454, "This is your hands slot, it is always available, even if you have no other gear. Use items by selecting them with the D-pad and pressing Cross."},
  {456, "Move with the D-pad"},
  {457, "Movement with the analog stick"},
  {460, "Choose one perk and press Cross to take it. It will always be active until your character dies."},
  {461, "Use the Circle button to pick up items and interact."},
  {462, "Now you have a ranged weapon, aiming happens automatically. Press Cross to shoot."},
  {463, "You can reload your weapon by pressing Square. If you have suitable ammo, it will be loaded."},
  {467, "Use the D-pad to choose items in the inventory: Cross uses, Triangle puts in hands, Square drops."},
};

std::map<int, std::string> g_ui;

const char* fallback_for(int id) {
  for (unsigned i = 0; i < sizeof(kFallbacks) / sizeof(kFallbacks[0]); ++i) {
    if (kFallbacks[i].id == id) return kFallbacks[i].text;
  }
  return "";
}

void parse_ui_xml(const char* data, std::size_t len) {
  g_ui.clear();
  if (!data || len == 0) return;
  const char* p = data;
  const char* end = data + len;
  while (p < end) {
    const char* id_tag = std::strstr(p, "<id>");
    if (!id_tag || id_tag >= end) break;
    id_tag += 4;
    const char* id_end = std::strstr(id_tag, "</id>");
    if (!id_end || id_end >= end) break;
    char id_buf[16];
    const std::size_t id_len = static_cast<std::size_t>(id_end - id_tag);
    if (id_len >= sizeof(id_buf)) {
      p = id_end + 5;
      continue;
    }
    std::memcpy(id_buf, id_tag, id_len);
    id_buf[id_len] = 0;
    const int id = std::atoi(id_buf);
    const char* name_tag = std::strstr(id_end, "<name>");
    if (!name_tag || name_tag >= end) break;
    name_tag += 6;
    const char* name_end = std::strstr(name_tag, "</name>");
    if (!name_end || name_end >= end) break;
    g_ui[id] = std::string(name_tag, static_cast<std::size_t>(name_end - name_tag));
    p = name_end + 7;
  }
}

} // namespace

bool ui_strings_load(IPlatform* platform, const char* path) {
  g_ui.clear();
  if (platform && path && path[0]) {
    char* data = 0;
    std::size_t size = 0;
    if (platform->read_file(path, &data, &size) && data) {
      parse_ui_xml(data, size);
      delete[] data;
    }
  }
  for (unsigned i = 0; i < sizeof(kPspOverrides) / sizeof(kPspOverrides[0]); ++i)
    g_ui[kPspOverrides[i].id] = kPspOverrides[i].text;
  for (unsigned i = 0; i < sizeof(kFallbacks) / sizeof(kFallbacks[0]); ++i) {
    if (g_ui.find(kFallbacks[i].id) == g_ui.end()) g_ui[kFallbacks[i].id] = kFallbacks[i].text;
  }
  return !g_ui.empty();
}

const char* ui_string(int id) {
  std::map<int, std::string>::const_iterator it = g_ui.find(id);
  if (it != g_ui.end() && !it->second.empty()) return it->second.c_str();
  const char* fb = fallback_for(id);
  return fb ? fb : "";
}

} // namespace mdaz
