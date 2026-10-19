#include "mdaz/tutorial_layout.hpp"

namespace mdaz {

#include "tutorial_layout.inc"

#define MDAZ_N(a) static_cast<int>(sizeof(a) / sizeof(a[0]))
int tut_building_count() { return MDAZ_N(kTutBuildings); }
const TutBuilding& tut_building(int i) { return kTutBuildings[i]; }
int tut_prop_count() { return MDAZ_N(kTutProps); }
const TutProp& tut_prop(int i) { return kTutProps[i]; }
int tut_tree_count() { return MDAZ_N(kTutTrees); }
const TutTree& tut_tree(int i) { return kTutTrees[i]; }
int tut_item_count() { return MDAZ_N(kTutItems); }
const TutItem& tut_item(int i) { return kTutItems[i]; }
int tut_zed_point_count() { return MDAZ_N(kTutZedPoints); }
const TutZedPoint& tut_zed_point(int i) { return kTutZedPoints[i]; }
int tut_car_loot_count() { return MDAZ_N(kTutCarLoot); }
const TutCarLoot& tut_car_loot(int i) { return kTutCarLoot[i]; }
int tut_misc_count() { return MDAZ_N(kTutMisc); }
const TutMisc& tut_misc(int i) { return kTutMisc[i]; }

namespace {
struct Txt { int id; const char* s; };
// l_eng_ui.xml (original_docs), verbatim. U+2019 (’) is kept as "'" because the bitmap
// font is ASCII-only (fonts are not touched); "\n" in id 465 is the XML's literal line break.
const Txt kUi[] = {
  {41, "Ok"},
  {42, "You need wood piles to repair this boat."},
  {44, "Repair"},
  {45, "Leave"},
  {46, "You can repair this boat with wood piles you have."},
  {451, "Repair this boat with a wood pile to move further. Chop any tree with an axe to get a wood pile."},
  {452, "Hey! This looks like some loot! Move closer to pick it up."},
  {453, "Here you can check all your items you picked up."},
  {454, "This is your hands slot, it is always available, even if you have no other gear. Use items by tapping."},
  {455, "Welcome, survivor. You are trapped on a small island, and your only chance to get to another one is to find a boat."},
  {456, "Move by tapping/drawing"},
  {457, "Movement with the stick"},
  {458, "Choose the movement controls mode. You can switch between them anytime."},
  {459, "As long as you keep surviving and killing enemies, you will obtain experience points (XP) to unlock perks."},
  {460, "Choose one perk and tap on the plus button. It will always be active until your character dies."},
  {461, "Use the interact button to pick up items and interact."},
  {462, "Now you have a ranged weapon, aiming happens automatically. Use this button to shoot."},
  {463, "You can reload your weapon by tapping this button. If you have suitable ammo, it will be loaded."},
  {464, "Switch between melee and ranged weapons mode."},
  {465, "It looks like you're bleeding.\nFind a bandage or tear your t-shirt into rags."},
  {466, "Your blood level dropped to zero. Lucky for you it is only a tutorial and you will be healed now. Go on."},
  {467, "Use drag and drop to manage items in the inventory."},
  {468, "Look, that's a firearm. It can usually be found on military locations."},
  {469, "Infected ahead! Attack it by moving closer. The fight will proceed automatically."},
  {470, "Wolves! Keep quiet, wolves are pretty dangerous, especially in packs."},
  {471, "A water pump. You can drink from it and fill up your canteen or water bottle."},
  {472, "An abandoned car. You can check the trunk for loot. Move closer to the back of the car to do so."},
  {473, "You can hunt deer and flay them with a knife to get meat."},
  {474, "You can hunt rabbits and flay them with a knife to get meat."},
  {475, "You can gather berries from some bushes, just look around."},
};
const Txt kLog[] = {
  {125, "I'm friendly."},
  {126, "Good."},
  {127, "Thanks for repairing the boat for us."},
};
} // namespace

const char* tut_ui_text(int id) {
  for (int i = 0; i < MDAZ_N(kUi); ++i) if (kUi[i].id == id) return kUi[i].s;
  return "";
}
const char* tut_log_text(int id) {
  for (int i = 0; i < MDAZ_N(kLog); ++i) if (kLog[i].id == id) return kLog[i].s;
  return "";
}

} // namespace mdaz
