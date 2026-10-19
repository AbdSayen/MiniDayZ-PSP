#include "mdaz/item_sprites.hpp"

namespace mdaz {

namespace {

struct Entry { ItemId id; ItemIcon icon; };

const Entry kMap[] = {
  { 1,  { "canned_beans-sheet0", 0, 0, 8, 8 } },
  { 2,  { "canned_tuna-sheet0", 0, 0, 8, 8 } },
  { 3,  { "tactical_bacon-sheet0", 0, 0, 8, 8 } },
  { 4,  { "rice-sheet0", 0, 0, 10, 8 } },
  { 5,  { "tomato-sheet0", 0, 0, 8, 8 } },
  { 6,  { "apple-sheet0", 0, 0, 8, 8 } },
  { 7,  { "banana-sheet0", 0, 0, 10, 6 } },
  { 8,  { "pipsi-sheet0", 0, 0, 6, 10 } },
  { 9,  { "spite-sheet0", 0, 0, 6, 10 } },
  { 10, { "kvas-sheet0", 0, 0, 6, 10 } },
  { 11, { "ammo_5x56-sheet0", 0, 0, 10, 7 } },
  { 12, { "ammo_45acp-sheet0", 0, 0, 10, 7 } },
  { 13, { "ammo_7x62-sheet0", 0, 0, 10, 7 } },
  { 14, { "ammo_7x62x39-sheet0", 0, 0, 10, 7 } },
  { 15, { "ammo_5x45-sheet0", 0, 0, 10, 7 } },
  { 16, { "ammo_357-sheet0", 0, 0, 10, 7 } },
  { 17, { "ammo_12cal-sheet0", 0, 0, 10, 7 } },
  { 18, { "bandage-sheet0", 0, 0, 9, 5 } },
  { 19, { "blood_bag-sheet0", 0, 0, 9, 7 } },
  { 20, { "flare-sheet0", 0, 0, 9, 3 } },
  { 21, { "f1_grenade-sheet0", 0, 0, 10, 10 } },
  { 22, { "batteries-sheet0", 0, 0, 8, 5 } },
  { 23, { "campfire_off-sheet0", 0, 0, 16, 16 } },
  { 24, { "matches-sheet0", 0, 0, 8, 8 } },
  { 25, { "whiskey-sheet0", 0, 0, 6, 12 } },
  { 26, { "rags-sheet0", 0, 0, 12, 9 } },
  { 27, { "apple-sheet0", 0, 0, 8, 8 } }, // cranberry — no dedicated atlas
  { 28, { "saline_bag-sheet0", 0, 0, 9, 7 } },
  { 29, { "vitamins-sheet0", 0, 0, 8, 8 } },
  { 30, { "notacola-sheet0", 0, 0, 6, 10 } },
  { 31, { "squash-sheet0", 0, 0, 10, 8 } },
  { 32, { "bell_pepper-sheet0", 0, 0, 8, 8 } },
  { 33, { "orange-sheet0", 0, 0, 8, 8 } },
  { 34, { "heatpack-sheet0", 0, 0, 10, 8 } },
  { 35, { "sewingkit-sheet0", 0, 0, 10, 8 } },
  { 36, { "cleaning_kit-sheet0", 0, 0, 10, 8 } },
  { 37, { "ashwood_stick-sheet0", 0, 0, 12, 4 } },
  { 38, { "wood_sticks-sheet0", 0, 0, 12, 8 } },
  { 39, { "duct_tape-sheet0", 0, 0, 10, 8 } },
  { 40, { "burplap_sack-sheet0", 0, 0, 12, 12 } },
  { 41, { "rope-sheet0", 0, 0, 12, 8 } },
  { 42, { "knife_hunter-sheet0", 0, 0, 11, 4 } },
  { 43, { "knife_army-sheet0", 0, 0, 11, 4 } },
  { 44, { "knife_butcher-sheet0", 0, 0, 11, 4 } },
  { 45, { "morphine-sheet0", 0, 0, 11, 5 } },
  { 46, { "tetracycline-sheet0", 0, 0, 8, 8 } },
  { 47, { "tomato_seeds_pack-sheet0", 0, 0, 10, 8 } },
  { 48, { "pepper_seeds_pack-sheet0", 0, 0, 10, 8 } },
  { 49, { "papers-sheet0", 0, 0, 10, 8 } },
  { 50, { "wood_piles-sheet0", 0, 0, 14, 10 } },
  { 51, { "handmade_arrows-sheet0", 0, 0, 12, 6 } },
  { 52, { "composite_arrows-sheet0", 0, 0, 12, 6 } },
  { 53, { "fueltank-sheet0", 0, 0, 10, 12 } },
  { 54, { "waterbottle-sheet0", 0, 0, 6, 12 } },
  { 55, { "canteen-sheet0", 0, 0, 9, 10 } },
  { 56, { "landmine-sheet0", 0, 0, 12, 10 } },
  { 57, { "beartrap-sheet0", 0, 0, 14, 12 } },
  { 58, { "ammo_22lr-sheet0", 0, 0, 10, 7 } },
  { 59, { "toolbox-sheet0", 0, 0, 12, 10 } },
  { 60, { "raw_steak-sheet0", 0, 0, 12, 8 } },
  { 61, { "cooked_steak-sheet0", 0, 0, 12, 8 } },
  { 62, { "nukacola-sheet0", 0, 0, 6, 10 } },
  { 63, { "edrink-sheet0", 0, 0, 6, 10 } },
  { 68, { "squash_seeds_pack-sheet0", 0, 0, 10, 8 } },
  { 85, { "mre-sheet0", 0, 0, 12, 10 } },
  { 88, { "quest_case-sheet0", 0, 0, 14, 12 } },
  { 89, { "gui_firearm_slot_scope-sheet0", 0, 0, 12, 12 } }, // Magpull stub
  { 90, { "choke_bor-sheet0", 0, 0, 10, 6 } },
  { 91, { "fish_herring-sheet0", 0, 0, 14, 6 } },
  { 92, { "fish_salmon-sheet0", 0, 0, 16, 6 } },
  { 93, { "fish_ruff-sheet0", 0, 0, 12, 6 } },
  { 94, { "fish_perch-sheet0", 0, 0, 12, 6 } },
  { 97, { "tomato-sheet0", 0, 0, 8, 8 } }, // mushroom emoji item
  { 98, { "gui_firearm_slot_scope-sheet0", 0, 0, 12, 12 } },
  { 99, { "claymore_item-sheet0", 0, 0, 14, 10 } },
  { 102, { "molotov_cocktail-sheet0", 0, 0, 8, 12 } },
  { 69, { "ammo_9mm-sheet0", 0, 0, 10, 7 } },
  { 70, { "barbedwire-sheet0", 0, 0, 12, 11 } },
  { 71, { "raw_rabbit_steak-sheet0", 0, 0, 12, 8 } },
  { 72, { "cooked_rabbit_steak-sheet0", 0, 0, 12, 8 } },
  { 73, { "apple-sheet0", 0, 0, 8, 8 } }, // cloudberry
  { 74, { "banana-sheet0", 0, 0, 10, 6 } }, // bilberry
  { 75, { "radio_item-sheet0", 0, 0, 12, 10 } },
  { 76, { "gui_firearm_slot_scope-sheet0", 0, 0, 12, 12 } },
  { 77, { "gui_firearm_slot_scope-sheet0", 0, 0, 12, 12 } },
  { 78, { "gui_firearm_slot_scope-sheet0", 0, 0, 12, 12 } },
  { 79, { "gui_firearm_slot_scope-sheet0", 0, 0, 12, 12 } },
  { 80, { "silencer_east-sheet0", 0, 0, 14, 4 } },
  { 81, { "silencer_nato-sheet0", 0, 0, 14, 4 } },
  { 82, { "hacksaw-sheet0", 0, 0, 14, 8 } },
  { 83, { "bandolier-sheet0", 0, 0, 14, 10 } },
  { 86, { "fishing_rod-sheet0", 0, 0, 16, 6 } },
  { 87, { "fishing_rod-sheet0", 0, 0, 16, 6 } },
  { 95, { "fish_filet_small-sheet0", 0, 0, 12, 6 } },
  { 96, { "fish_filet_big-sheet0", 0, 0, 14, 6 } },
  { 100, { "ammo_9x18-sheet0", 0, 0, 10, 7 } },
  { 101, { "ammo_9x39-sheet0", 0, 0, 10, 7 } },
  { 151, { "fnx_pistol-sheet0", 1, 1, 30, 30 } },
  { 152, { "colt_pistol-sheet0", 0, 0, 11, 7 } },
  { 153, { "magnum_pistol-sheet0", 1, 1, 30, 30 } },
  { 154, { "bm16_shotgun-sheet0", 1, 1, 30, 30 } },
  { 155, { "r670_shotgun-sheet0", 1, 1, 30, 30 } },
  { 156, { "mosin_nagant-sheet0", 1, 1, 30, 30 } },
  { 157, { "sks_rifle-sheet0", 1, 1, 30, 30 } },
  { 158, { "m4_rifle-sheet0", 1, 1, 30, 30 } },
  { 159, { "akm_rifle-sheet0", 1, 1, 30, 30 } },
  { 160, { "ak74_rifle-sheet0", 1, 1, 30, 30 } },
  { 161, { "aks74u_rifle-sheet0", 1, 1, 30, 30 } },
  { 162, { "l85_rifle-sheet0", 1, 1, 30, 30 } },
  { 163, { "improvised_bow-sheet0", 1, 1, 30, 30 } },
  { 164, { "crossbow-sheet0", 1, 1, 30, 30 } },
  { 165, { "amphibia-sheet0", 1, 1, 30, 30 } },
  { 166, { "sporter_rifle-sheet0", 1, 1, 30, 30 } },
  { 167, { "repeater_rifle-sheet0", 1, 1, 30, 30 } },
  { 168, { "svd_rifle-sheet0", 1, 1, 30, 30 } },
  { 169, { "mp5_smg-sheet0", 1, 1, 30, 30 } },
  { 170, { "glock_pistol-sheet0", 1, 1, 30, 30 } },
  { 171, { "sawed_izh-sheet0", 1, 1, 30, 30 } },
  { 172, { "sawed_mosin-sheet0", 1, 1, 30, 30 } },
  { 173, { "ump45_smg-sheet0", 1, 1, 30, 30 } },
  { 174, { "rpk_rifle-sheet0", 1, 1, 30, 30 } },
  { 175, { "fncal_rifle-sheet0", 1, 1, 30, 30 } },
  { 176, { "colt_pistol-sheet0", 1, 1, 30, 30 } },
  { 177, { "aug_rifle-sheet0", 1, 1, 30, 30 } },
  { 178, { "saiga_shotgun-sheet0", 1, 1, 30, 30 } },
  { 179, { "mac10_pistol-sheet0", 1, 1, 30, 30 } },
  { 180, { "sawed_repeater-sheet0", 1, 1, 30, 30 } }, // Mare's Leg
  { 181, { "pm_pistol-sheet0", 1, 1, 30, 30 } },
  { 182, { "pb_pistol-sheet0", 1, 1, 30, 30 } },
  { 183, { "bizon_smg-sheet0", 1, 1, 30, 30 } },
  { 184, { "groza_rifle-sheet0", 1, 1, 30, 30 } },
  { 185, { "vss_rifle-sheet0", 1, 1, 30, 30 } },
  { 186, { "sv98_rifle-sheet0", 1, 1, 30, 30 } },
  { 187, { "madsen_mg-sheet0", 1, 1, 30, 30 } },
  { 200, { "pipe_wrench-sheet0", 1, 1, 30, 30 } },
  { 201, { "shovel-sheet0", 1, 1, 30, 30 } },
  { 202, { "axe_red-sheet0", 1, 1, 30, 30 } },
  { 203, { "baseball_bat-sheet0", 1, 1, 30, 30 } },
  { 204, { "fireaxe-sheet0", 1, 1, 30, 30 } },
  { 205, { "crowbar-sheet0", 1, 1, 30, 30 } },
  { 206, { "pickaxe-sheet0", 1, 1, 30, 30 } },
  { 207, { "pitchfork-sheet0", 1, 1, 30, 30 } },
  { 208, { "sledgehammer-sheet0", 1, 1, 30, 30 } },
  { 209, { "sword-sheet0", 1, 1, 30, 30 } },
  { 210, { "pan-sheet0", 1, 1, 30, 30 } },
  { 250, { "gasmask-sheet0", 1, 1, 30, 30 } },
  { 251, { "helmet_army-sheet0", 1, 1, 30, 30 } },
  { 252, { "helmet_moto-sheet0", 1, 1, 30, 30 } },
  { 253, { "helmet_moto_red-sheet0", 1, 1, 30, 30 } },
  { 254, { "warm_hat-sheet0", 1, 1, 30, 30 } },
  { 255, { "beret_red-sheet0", 1, 1, 30, 30 } },
  { 256, { "baseball_cap-sheet0", 1, 1, 30, 30 } },
  { 257, { "balaklava-sheet0", 1, 1, 30, 30 } },
  { 258, { "ushanka-sheet0", 1, 1, 30, 30 } },
  { 259, { "helmet_hard-sheet0", 1, 1, 30, 30 } },
  { 260, { "headlamp-sheet0", 1, 1, 30, 30 } },
  { 261, { "cowboy_hat-sheet0", 1, 1, 30, 30 } },
  { 262, { "welding_mask-sheet0", 1, 1, 30, 30 } },
  { 263, { "gorka_helmet-sheet0", 1, 1, 30, 30 } },
  { 264, { "police_hat-sheet0", 1, 1, 30, 30 } },
  { 265, { "helmet_nvg-sheet0", 1, 1, 30, 30 } },
  { 266, { "bandana-sheet0", 1, 1, 30, 30 } },
  { 267, { "gorka_helmet-sheet0", 1, 1, 30, 30 } },
  { 300, { "jeans-sheet0", 1, 1, 30, 30 } },
  { 301, { "workpants-sheet0", 1, 1, 30, 30 } },
  { 302, { "sportpants-sheet0", 1, 1, 30, 30 } },
  { 303, { "gorka_pants-sheet0", 1, 1, 30, 30 } },
  { 304, { "orel_pants-sheet0", 1, 1, 30, 30 } },
  { 305, { "paramedic_pants-sheet0", 1, 1, 30, 30 } },
  { 306, { "hunter_pants-sheet0", 1, 1, 30, 30 } },
  { 350, { "tshirt-sheet0", 1, 1, 30, 30 } },
  { 351, { "shirt_gray-sheet0", 1, 1, 30, 30 } },
  { 352, { "shirt_green-sheet0", 1, 1, 30, 30 } },
  { 353, { "raincoat_blue-sheet0", 1, 1, 30, 30 } },
  { 354, { "down_jacket-sheet0", 1, 1, 30, 30 } },
  { 355, { "gorka_jacket-sheet0", 1, 1, 30, 30 } },
  { 356, { "hoodie_gray-sheet0", 1, 1, 30, 30 } },
  { 357, { "hoodie_red-sheet0", 1, 1, 30, 30 } },
  { 358, { "cloak_brown-sheet0", 1, 1, 30, 30 } },
  { 359, { "paramedic_jacket-sheet0", 1, 1, 30, 30 } },
  { 360, { "orel_jacket-sheet0", 1, 1, 30, 30 } },
  { 361, { "sport_jacket-sheet0", 1, 1, 30, 30 } },
  { 362, { "dress-sheet0", 1, 1, 30, 30 } },
  { 363, { "down_jacket-sheet0", 1, 1, 30, 30 } },
  { 364, { "hunter_jacket-sheet0", 1, 1, 30, 30 } },
  { 365, { "sweater-sheet0", 1, 1, 30, 30 } },
  { 400, { "molle_vest-sheet0", 1, 1, 30, 30 } },
  { 401, { "vest_press-sheet0", 1, 1, 30, 30 } },
  { 402, { "vest-sheet0", 1, 1, 30, 30 } },
  { 403, { "molle_vest-sheet0", 1, 1, 30, 30 } }, // high capacity vest
  { 404, { "soviet_vest-sheet0", 1, 1, 30, 30 } },
  { 405, { "soviet_vest-sheet0", 1, 1, 30, 30 } },
  { 450, { "taloon_backpack-sheet0", 1, 1, 30, 30 } },
  { 451, { "hunter_backpack-sheet0", 1, 1, 30, 30 } },
  { 452, { "mountain_backpack-sheet0", 1, 1, 30, 30 } },
  { 453, { "school_backpack-sheet0", 1, 1, 30, 30 } },
  { 454, { "improvised_bag-sheet0", 1, 1, 30, 30 } },
  { 455, { "improvised_backpack-sheet0", 1, 1, 30, 30 } },
  { 456, { "civilian_tent-sheet0", 1, 1, 30, 30 } },
  { 457, { "tortilla_backpack-sheet0", 1, 1, 30, 30 } },
};

const int kMapCount = static_cast<int>(sizeof(kMap) / sizeof(kMap[0]));

} // namespace

const ItemIcon* find_item_icon(ItemId id) {
  for (int i = 0; i < kMapCount; ++i) {
    if (kMap[i].id == id) return &kMap[i].icon;
  }
  return 0;
}


namespace {
struct GroundEntry { const char* icon_sheet; ItemIcon ground; };
const GroundEntry kGround[] = {
#include "item_ground_frames.inc"
};
const int kGroundCount = static_cast<int>(sizeof(kGround) / sizeof(kGround[0]));
} // namespace

// C2 items_on_ground: weapon/clothing/backpack types draw their "on_ground" animation
// frame (e.g. hunter backpack 15x10, AKM 19x7); the 30x30 sheet0 frames are the
// worn/held overlays. Small items (food, meds, ammo) have no on_ground anim: their
// single frame (8x8 etc.) is already the ground sprite.
const ItemIcon* find_item_ground_icon(ItemId id) {
  const ItemIcon* ic = find_item_icon(id);
  if (!ic) return 0;
  for (int i = 0; i < kGroundCount; ++i) {
    const char* a = kGround[i].icon_sheet;
    const char* b = ic->sprite;
    while (*a && *a == *b) { ++a; ++b; }
    if (*a == 0 && *b == 0) return &kGround[i].ground;
  }
  return ic;
}

} // namespace mdaz
