#pragma once
// C2 perks (data.js: perkmenu / Game_events / Restart_game). The perk index is the frame
// of gui_perk_icon (t544); each frame is tied to one global perk_* variable:
//   0 perk_header (Sharpshooter)  1 perk_spotter (Scout)    2 perk_runner (Sprinter)
//   3 perk_camel                  4 perk_hamster            5 perk_snowborn
//   6 perk_farmer (Bullet Farmer) 7 perk_regen (Survivalist) 8 perk_meleedef (Blocker)
//   9 perk_meleeatk (Puncher)    10 perk_trunkster          11 perk_hunter
//  12 perk_sweethome             13 perk_metabolism (Red+)  14 perk_eye (Nightcrawler)
//  15 perk_agronom (Agronomist)
// Descriptions: l_eng_ui.xml 400 + frame.
namespace mdaz {

enum PerkId {
  Perk_Header = 0, Perk_Spotter, Perk_Runner, Perk_Camel, Perk_Hamster, Perk_Snowborn,
  Perk_Farmer, Perk_Regen, Perk_MeleeDef, Perk_MeleeAtk, Perk_Trunkster, Perk_Hunter,
  Perk_Sweethome, Perk_Metabolism, Perk_Eye, Perk_Agronom, Perk_Count
};

const char* perk_c2_var(int frame);

struct PerkState {
  int xp_raw;          // expierence_points (Restart_game 16; shown as xp_raw - 16)
  int learnt;          // perks_learnt_count
  int cost;            // perk_cost (Restart_game 200; after a buy 216*(c+1) + 100c - 16c)
  bool owned[Perk_Count];
  bool perk2_once;     // TriggerOnce latch of the MD_Perk_2 event
  PerkState() { reset(); }
  void reset();
  bool has(int f) const { return f >= 0 && f < Perk_Count && owned[f]; }
  int shown_xp() const { return xp_raw - 16; }
  int shown_cost() const { return cost - 16; }
  bool all_owned() const;
  // perkmenu buy (Touch gui_perk_icon_buy): selected icon not bought and XP >= perk_cost.
  // XP is not subtracted. Returns true when the perk was learnt.
  bool can_buy(int f) const { return f >= 0 && f < Perk_Count && !owned[f] && xp_raw >= cost; }
  bool buy(int f);
  // gui_btn_perks_notify visible: XP >= perk_cost, hidden again when all 16 perks are owned.
  bool notify_visible() const { return xp_raw >= cost && !all_owned(); }
  int pack_bits() const;
  void unpack_bits(int b);
};

// perkmenu layout (CreateObject gui_perk_icon at HotspotX/HotspotY offsets, C2 px).
struct PerkSlotPos { int frame; int dx; int dy; };
extern const PerkSlotPos kPerkMenuSlots[Perk_Count]; // order: upper row L->R, lower row L->R
// gui_perk_icon-sheet0 source rect for frame f (90x90).
void perk_icon_src(int f, int& sx, int& sy);

} // namespace mdaz
