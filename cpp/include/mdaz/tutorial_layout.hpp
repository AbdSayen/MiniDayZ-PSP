#pragma once
// C2 layout "Tutorial" (8300x2000) placed instances, generated from data.js by
// tools/extract_tutorial.py (src/world/tutorial_layout.inc). Tilemaps: data/tutorial_tilemaps.txt.

namespace mdaz {

struct TutBuilding { const char* ext_sprite; float x, y; };
struct TutProp { int def; float x, y; int frame; };      // MapProps def index (map_props.inc)
struct TutTree { int sub; int frame; float x, y; };     // trunk point (Imagepoint 1); sub 7 = tree_block
struct TutItem { int id; int count; float x, y; };      // items_on_ground instances
struct TutZedPoint { float x, y, angle_deg; };          // zed_resp_point (all frame 0)
struct TutCarLoot { float x, y; };                      // car_loot_point at each car's Imagepoint 1
struct TutMisc { int type; float x, y; int frame; float w, h; int iv0; };

static const int kTutLayoutW = 8300;
static const int kTutLayoutH = 2000;

// C2 object type ids used in TutMisc.
enum {
  kTutT_BotTutor1 = 667, kTutT_BotTutor2 = 668, kTutT_BExit = 614, kTutT_HammerCrash = 434,
  kTutT_Crow = 687, kTutT_PlayerStart = 172, kTutT_Zone = 666
};

int tut_building_count();
const TutBuilding& tut_building(int i);
int tut_prop_count();
const TutProp& tut_prop(int i);
int tut_tree_count();
const TutTree& tut_tree(int i);
int tut_item_count();
const TutItem& tut_item(int i);
int tut_zed_point_count();
const TutZedPoint& tut_zed_point(int i);
int tut_car_loot_count();
const TutCarLoot& tut_car_loot(int i);
int tut_misc_count();
const TutMisc& tut_misc(int i);

// UI strings used by the tutorial (l_eng_ui.xml ids 41..50, 451..475) and the log strings of
// Tutorial_script_final (l_eng_log.xml ids 125..127). Verbatim English text; "" when unknown.
const char* tut_ui_text(int id);
const char* tut_log_text(int id);

} // namespace mdaz
