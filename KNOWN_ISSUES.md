# Known Issues & Limitations

Last updated: 2026-10-17 (dmg_hint ported; no invented consumable restores; PSP EBOOT still unverified)

Last updated: 2026-10-19 (UI rewrite, forest Solid, culling)

## 2026-10-19 open items

| Issue | Impact | Status |
|-------|--------|--------|
| **Real PSP: black screen** (user test 2026-10-19 19:11) with `cpp/platform/psp/deploy/MINIDAYZ`: deploy EBOOT/pak were stale (17.10). Root-cause candidate found in code: `platform_psp.cpp` passed the **RAM** backbuffer pointer to `sceGuDrawBuffer/sceGuDispBuffer` (which take VRAM *offsets*) and to `sceDisplaySetFrameBuf` (display controller cannot scan out of main RAM) → black on hardware. Fix 19:14: two 512×272 RGB565 VRAM buffers at 0x44000000 (uncached) + flip via `sceDisplaySetFrameBuf(...NEXTFRAME)`, CPU frame memcpy'd each `end_frame`; GU offsets 0 / 278528; heap `PSP_HEAP_SIZE_KB(-1024)` (all-but-1MB) instead of fixed 18MB; boot log `ms0:/PSP/GAME/MINIDAYZ/log.txt` (open/append/close per line) for boot, chdir, platform init, each file read result, pak load, free memory before/after `new Game`, init ok/FAILED, first 3 frames; coloured splash with text before asset load (red splash 10 s on init failure). EBOOT rebuilt from current sources with WSL pspdev (psp-gcc 15.2.0, `make clean && make`, exit 0) and deploy/sprites.pak regenerated 19:14 | Game unusable on PSP | Реализована, но не проверена (needs PPSSPP/real-PSP run + log.txt) |
| **USE acts on first item, not selected slot** (user report 2026-10-19 19:20). Cause: `Game::use_equipped_item(true)` (△ / USE in play) (a) scanned the WHOLE bag for gasoline 53 / toolbox 59 before looking at the selection and (b) when the selected item was not consumable (weapon, tool, empty slot) fell back to the FIRST consumable in the bag; it also played the use anim/sound when nothing was used. Fix 19:27: USE acts only on `equipped_slot_` (inventory Cross sets it to the focused slot temporarily), fallback removed, vehicle items only from the selected slot; craft index clamped in `update_craft_menu`; save-loaded `equipped_slot_` clamped; hotbar switching via `Game::next_hotbar_slot`. Audited, already correct: drop (focused/equipped), equip/to-hand swap, reload (ammo for selected gun), craft (selected recipe), inventory focus clamp + open-sync to equipped. Loot transfer has no focused-item UI (interact takes the whole pile, as before) — no selected-index path to fix. Not changed: F throw prefers selected grenade/molotov/stick, else scans the bag (dedicated hotkey); auto heatpack at <18° scans the bag. Tests: `tests/test_selection.cpp` 21 checks, `mdaz_tests.exe` ALL TESTS PASSED 19:27 (the vehicle-refuel branch is not exercised: no vehicle in the test) | Wrong item consumed/used | Протестирована (desktop) |
| **HUD vitals icons wrong** (user report 2026-10-19 19:23, see play5/explore1.png): the 4 top bars (red/cyan/green/yellow) show wrongly drawn and wrong icons; must take frames/positions/sizes/colours from the original HUD in data.js and check what the 4th indicator really is | Misleading HUD | Исследуется (queued after USE fix) |
| ~~Intermittent silent exit in `--shots`~~ (build 19:01, frames 2/13/10/1 over 4 runs) | None: **false alarm**, the process was terminated by the user | Closed 2026-10-19 19:09 (user-terminated, not a bug) |
| PSP EBOOT after the UI rewrite | Rebuilt 2026-10-19 19:14 incl. `psp_ui.o`/`screens.o` (compiles+links); runtime unverified on PSP/PPSSPP | Реализована, но не проверена |
| `generate_location` forest stamps (72 SetTile offsets) are an approximate decode of a truncated export | Not applied any more: with forest drawn+Solid they produced isolated corner tiles around POIs. Forest exists only where the Map layout has tilemap_forest (borders) | Заблокирована внешним ограничением (export truncated) |
| `trees_` scatter around POIs is a port approximation (C2 tree instances not decoded) | Tree placement not faithful; trunk push-out (14 px) is port-side, C2 trees are not Solid | Исследована |
| Building walls: C2 building "in" animations are Solid with polygons; port still uses `mapgen.apply_collision` blocked cells | Can walk through some walls / blocked near centres | Не исследована (next) |
| NPCs (zeds/bandits/animals) ignore tilemap_forest Solid | Only player/car collide with forest | Реализуется |
| Ground loot uses the C2 on_ground frames (white outline is in the original art) | — | Сверена с оригиналом (frames) |
| Ground seams: 60 px ground cells of visibly different shade (explore1) | Patchwork look | Queued |
| Minimap draws black (fog covers everything) | No map info | Queued |
| White outline around loot near the helicopter | Check against the original on_ground art | Queued |
| Remaining undefined Game methods: apply_loot_view, pack_loot_view, hurt_animal/bandit/zombie/zombie_bullet, maybe_add_hammer_loot, maybe_spawn_quest_bandits, notify_weapon_unlock, on_quest_completed, spawn_bunker_zeds, spawn_hostiles_for_poi, spawn_loot_for_poi, sync_location_stream, tick_molotov_burn, try_build_fence, zed_check_bunker | Declared, not called by current code (link OK) | Не исследована / pending restore |
| Ground biome layout (which ground_lvl atlas where) is a port hash, not decoded from C2 | Ground looks patchy in large squares (visible in explore shots) | Исследована |
| Inventory/hotbar show items with the on_ground frame (C2 GUI icon source not confirmed) | Icons small, but they are original art | Исследована |
| Mouse inventory clicks (old desktop path) | Removed from UI design; keyboard/PSP buttons only | By design |

## External / environment

| Issue | Impact | Status |
|-------|--------|--------|
| No Construct `.capx` — only HTML5 export | Some editor-only data unavailable | Permanent for this repo |
| PSP EBOOT / textures | `Game` on heap; sprites via **`data/sprites.pak`** + CPU blit (`platform_psp.cpp`, `tools/pack_psp_sprites.js`). Deploy: `tools/pack_psp_deploy.ps1`. | Like PC assets; no GU VRAM textures yet (CPU blit). Re-test on PPSSPP |
| Online login / DeltaDNA / ads in original | Not ported | Intentionally stubbed offline |

## Incomplete / approximate logic

| Issue | Detail |
|-------|--------|
| Item numeric stats | Eat/drink/heal only from `item_activate` IV literals (`item_activate_iv_sverka.json`). Unknown food/drink/medical with no IV restore **refuse use** (no invented +15/+10). **Morphine (45) blocked**. Cook-only raws (60/71/91–94) and mushroom (97) cannot be eaten. |
| Infection acquire | **Partial:** tetracycline clears IV34; vitamins suppress DoT. `Player_get_hit` does **not** set IV34 in export — invented on-hit infection roll **removed**. Full C2 disease-acquire tree still open. |
| Gun jam / melee break | **Removed:** no jam/misfire/break weapon strings in `data.js`. `gun_jammed_` / `melee_break_mul_` kept for save compat only (always clear / unused). |
| Wetness meter | **Removed (gameplay):** no Wet/soak/drench strings in export. `wetness_` forced 0 (save field retained). Rain/cold still via `Day_mode`/`Rain` survival path. |
| Firearm base damage | **Sverka:** Fist **10**/range **45**, FNX (151) bullet IV0 **23** via `Game_events` spawn chain (`tools/_scan_bullet_spawn_iv0.js` → `weapon_bullet_iv0_sverka.json`), sword **15**, hit variance ±25%, bleed start `random(3)>=1.5`. All other firearms use **`kGunDmgPlaceholder` (25)** in `weapons.cpp` — not sverka. Do not promote `extract_gun_iv0_deep.js` candidates without the IV1←172 + IV0 literal path. |
| Mag capacities | Approximate; not fully extracted from C2 reload events. |
| Survival / weather timebase | `Timer_ALL`/`Day_Night_speed`/`Rain_speed` drive phase + rain toggle; cold globals applied when non-zero. Still approximate vs full C2 Day&Night event tree. |
| Molotov / bow | **Sverka (partial):** molotov throw speed **300**, ~**5s** flame, **0.5s** `Molotov_damage` ticks (10/9/8/7/6 by target class). Bow **163**: **V** nocks arrow (**2s**, `Bow_reload`/`Mag_reload` path); crossbow uses mag reload. |
| Death screen | Native `dead_screen` + stats; `last_death.txt` uses `Death_log_*` field names. **Partial:** `gui_tutor_tip_bg` / `bot_tutor_*` after `deathtime` delay; UI **#542** “You are dead”; `tutor_death` audio. **Decoded:** `Death_guide` is **tutorial-only** (dict **#466** fake-death text, `Deadscreen` layout) — see `cpp/docs_gen/functions/Death_guide_decoded.txt`. Sandbox death has no decoded tutortext panel. |
| Map generation | POI counts from `balance_globals`; static Map-layout borders; runtime dirt **tile 15** POI footprints + **generate_array_roads_1/_2** sector markers (**12/13/14** between paired POI cells on rows/cols) stamped as dirt corridors; heli water **tile 4**. Sector grid tags **village/city/military** as **6/9/11**. **generate_location** → **72** forest SetTile stamps (`map_decode_tables.hpp`). **`generate_mapgen_toelements`** **verified:** ForEach obj507 → `generate_location`. **`create_map_element`** **partial:** zed_resp tile → obj501 **IV10** (`create_map_element_decode.hpp`); **`map_element-sheet0`** at POI forest stamp tile-0 when sprite loaded; zed_resp grid + IV10→prop frames **blocked**. **River** seed-only. |
| Bunker | RLE load + walkable dens; typed bunker/mil/hosp loot + multi-type zeds — still not full C2 bunker event tree. |
| Crafting | Inventory recipes **only** C2 names with Spawn_drop: **`Craft_1`/`Craft_4`/`Craft_5`/`Craft_fishing_rod`**. Fence **`Craft_31`/`Craft_32`** world place. **`Craft_21`/`22`/`33`/`34`/`3`** + **"Craft arrows"** = Crafted/pickpile workstation (no inventory Spawn_drop) — blocked. Invented Cook_*/rags/arrows/molotov/ashwood/matches/spinning inventory recipes **removed**. Cooking (`meat_cook` audio exists) — fire cook tree still open. |
| Reload / ammo | **Sverka:** `Mag_reload` sets `Reloading_mag`, pistol path uses `reload_pistol` when `player_weapons` IV93==0, rifle uses `Reload_rifle`; `Reloading_time=1` on many pistol paths; `+19` ticks added in `Mag_reload_body.json`. Port: R/V → `try_mag_reload`, `consume_ammo_for_reload` (bag then IV19→17→11→9 cascade), pistol 1s reload. **Partial:** clothing IV93 containers — 4 sub-slots/region, gates from worn piece; per-garment cargo volume (Array **536** col **301** / IV10 UI in `Items_update`) — table not in export, not wired. Exact per-weapon IV0 reload ticks still open. |
| Bunker zeds | **Partial C2:** `Zed_check_bunker` → `spawn_bunker_zeds(need)`; **21** `bunker_zed_spawner` XY from export (`bunker_zed_spawners.json`, `%` layout **approx**) then random walkable fallback. |
| Bunker entrance | `ensure_bunker_entrance` when **CurrentLevel≥1** (new game uses level 1); C2 `items_on_ground` instance / full spawn sheet not decoded. |
| Quests | **Partial C2:** tasks **1–3** with **`generate_quest_item_rewards`** / pin / karma (`quest.cpp`, `test_systems`). Task **2** uses **`spawn_bandit_camp`** at quest anchor (not obj245 markers). Ambient **`Bot_bandits_check_situation`** + IV0 filter on **`Spawn_team_camp`** markers ported; quest **`Remove_bandit_bots`** distance branch still partial; full C2 quest GUI strings not decoded. |
| Farming | Seeds plant → grow **20s** (item_activate sverka) → E harvest; rain/night modifiers; plots in MDAZ **v8**. Not full C2 garden plots. |
| Placed traps | Bear/barbed/fence/mine/claymore persist in MDAZ **v9**. |
| Campfires / tent | Active fires + tent warmth timer persist in MDAZ **v11**; adrenaline in **v12**. |
| Airdrop timer | Uses `Airdrop_Timer_ALL`=360 (was recharge 400); radio ETA HUD shows estimate. |
| Vehicle stash | 4 slots via G near vehicle; persisted in MDAZ **v7**. |
| Drivable cars | **Partial C2:** UAZ/Volga/Truck spawn; fuel max **100**, gas **+40**; crash hp **15/30**; UAZ/Volga **600s+1s** respawn (**verified** in `Drivable vehicles_decoded.txt`). IV284 **−8/0.1s** tick, **boat** fuel, **truck** respawn **blocked** in `data.js` export (`vehicle_fuel_truck_decode.txt`). **GUI_draw_vehicle** partial: `gui_car_bg`, vertical **bar_fuel** / **bar_hp** (IV284/294), nofuel/disable icons, car button sprites; hidden when not driving (**GUI_erase_vehicle** / States). **Sverka (partial):** mount flow mirrors **AltMove** + **Pin** (t184/t946) from `Drivable vehicles` decode; **`kDriveSpeedApprox=160`** — custom AltMove/Vehicle behavior JS missing from export, so px/s not decodable. Full **Vehicles UI** / dpad, truck respawn, boat fuel tick **not** verified. |
| Zed / animal HP | **NPC_get_damage** wired (`npc_apply_damage` + kind map 0–11); HP bar uses C2 `min(30, hp×scale)`. Max HP / speed still stubs pending full IV extract. |
| Difficulty NPC dmg | Novice/Regular/Veteran tables for Zed/Army/Wolf from C2 States assigns; Zed_fast Novice/Veteran interpolated (4/8). **Wolf→player** hits use **Zed_dmg** in `Wolf_dmg_deal` (not `Wolf_dmg` global); player→wolf still uses `Wolf_dmg` via `NPC_get_damage` kind 0. |
| Bullets_dmg_deal / armor | **Hit zones:** bullet IV2 1/2/3 vs player **blocked** for head/body/legs mul (`bullet_hit_zones_decode.txt` — branches are kill-cam/IV8). **Sverka (partial):** rolled dmg ±25%; `NPC_get_damage` kinds 0–11; player bullets skip vest/helmet **mul** (`Player_get_hit` bullet path). **IV6** wear on bullet + melee hits (`dmg_randomizer` 3–7 → helmet/vest/outer/pants/backpack slots); broken armor (IV6≤0) drops mul. **IV8** second hit on army/tank zeds for mosin/SKS/SVD/repeater/SV-98/sawed mosin when `dmg < random(0,100)`. Melee **def_percent** block roll (IV4 tiers 5/15/25/35 + `perk_meleedef`). **Hit_effect** layout flash (100% / 0.2s) on player damage. **`dmg_hint`** ported (`DmgHintSystem`): floating `-N`, orange/red crit-style when IV8 second-hit > roll. **IV6 max** approx. **Bullet IV2:** travel decay 3→2→1 @ 100/220px; player hitbox zones / kill cam still open. |
| Wolf attack | **Sverka (partial):** range **50**, cooldown **3s** (ScrollTo), SFX `wolf_attack`/`_2`/`_3`. Animation/interactor overlap not simulated — distance check only. |
| Loot save/load | MDAZ **v18:** **96** ground piles (32 legacy + 64 ext) + `loot_respawn_t`. C2 **Save_loot/Load_loot** CSV decoded (`save_loot_csv_decode.txt`: obj909 ForEach, token indices 6–12) — GUI CSV not ported. **checkpile** = inventory/craft stick UI, not save format. Per-pile **LOOT_respawn** stagger (**3/4/5s**) unchanged. |
| Pickup space | **Partial C2:** `Pick_inventory_item` → `pick_inventory_item` / `Check_space_inventory` then `Check_space_pants`→outerwear→vest→backpack when bag full (`clothing_containers.cpp`). C2 space checks use IV93 **slot** Arrays (`Contains` / empty cell), not col 301 volume sum. |
| Inventory UI | **Partial C2:** Tab toggle/hold opens bag (`Inventory_opened`); **Esc** / Tab-close / **V** mag reload / death call `close_inventory`. Hotbar **1–8** + **C** “Using inventory items” on equipped slot. **`Pick_player_item`** / **`init_item_activation`** decoded (GUI → `item_activate`); not ported headless. |
| POI loot tables | Typed tables (mil/hosp/gas/heli/police/fire/church/secret) + weighted Spawn_drop — not full C2 Check_loot trees. |
| AI | Zed/animal/bandit behaviors functional but incomplete vs C2 event trees. **`bot_reloading`** ported for firearm bandits (IV9 mag, IV10 reload, IV14 mag table + wait 2/2.5/3s in `bandit.cpp` / `update_bandit_guns`). Bot shooter bullet params / full obj243 spawn sheet still partial. |
| Audio | Original OGG tags mapped for common cues; not every C2 audio tag. |
| Starter inventory | Lean survival kit by difficulty in `game.cpp`. `starter_inventory_decode.json`: Start events hints only (ids 1–4 counts), no Spawn_drop seed table — **blocked**. |
| Plus / Reloaded | Out of scope (baseline 1.4.1). |

## Removed: not found in original (2026-10-17)

Audit (`node tools/audit_port.js` + keyword search in `original_docs/data.js`) found no evidence for these, so they were **removed** from code and tests: Craft_heatpack / Craft_flare / Craft_baseball_bat; guessed Craft_3 / Craft_21 / Craft_22 recipes (ingredient UIDs 308–311 unresolved); tree stump regrowth; overencumber slowdown; drinking from shore; limp / leg fracture; radio shortening airdrop by 45 s; night aggro / night animal spawn boost; wolves fleeing campfires; rain extinguishing torch; shovel caches; typed zed-corpse loot; bear-trap infection; Military / Helicrash quest steps.

Save format: MDAZ v10–v13 keep the old `limp_t` float as `reserved_v10` (written as 0, ignored on load) so existing saves still load. A saved Military/Helicrash quest id is dropped on load.

Still unverified against C2 (not yet removed): many `craft.cpp` recipes without a matching `"Craft_*"` name in `data.js` (e.g. rags→bandage, cooks, molotov); wetness soak rates; full C2 quest GUI strings (task logic + rewards/pin/karma are ported).

**Removed 2026-10-17 (no `"Craft_*"` in C2):** raincoat/bandolier/bandana/burlap-sack/fence-pile/firekit inventory recipes; guessed rain sprint slow + rain infection roll; military POI + placed barbed-wire move slow; auto heatpack; crow corpse scavenging + crow stick drop.

**Sverka from `item_activate` decode:** Whiskey IV14+20 / IV16+40 (temperature); Energy drink IV14+20 + 10s max-speed boost (135/125 vs 110/100); Heatpack manual IV16+25; Flare IV26=1 → torch equip only (no zed-pull/campfire). See `cpp/docs_gen/consumable_item_activate_sverka.json`.

## Presentation gaps (priority 5)

| Issue | Detail |
|-------|--------|
| Overworld tiles | Base grass = tile 6; static borders from `world_tilemaps.txt`; POI dirt footprints stamped at runtime (tile 15). Away from POIs/roads the interior is still mostly bare grass — C2 also plants buildings/trees via `create_map_element` / `generate_location` (partially approximated). |
| Menu | Background is the original Menu layout (tilemaps + 149 sprites from `tools/extract_layout_sprites.js`), `menu_btn_wood` buttons. Pan uses **`camera_roll_menu`** chase (distance **100**, Stopper speed **3** → scaled); pin placement without touch is **approx** (`menu_camera_decode.txt`). |
| Font | 5×7 rect font in `platform_sdl.cpp`; no `bitmap_font*.png` in `original_docs/images` (C2 likely used web/system text). Sheet hook reserved if mirror adds it. |
| Character anims | Idle/run/axe/pistol/twohand/using_item + zed walk/run/attack from `char_anims.txt`. **Death:** zed/wolf play `die1`/`die2` ~0.55s on kill when frames exist, then static dead sheets; tank/screamer/deer/rabbit often skip (no living-skin die in export). Player still `player_skin_dead` sprite. |
| Minimap | Tiled `minimap_tile`, road tint, `minimap_mark`/`minimap_hint` POIs, layout-sized `minimap_player`; full C2 `generate_minimap` tilemap not ported. |
| Buildings | POI sprites at native frame size; pair spacing **±150** = half of verified **±300** SetTile window (`create_map_location_poi_decode.txt`). **`Buildings_base` Map-layout prefab XY** not ported — still tag-based `poi_layout.cpp`. |
| Clothing layers | Worn helmet/vest/jacket sheets overlay body via `player_worn_visual` + `draw_c2_char` grid; C2 `player_helmets`/`player_vests` containers not decoded. |
| Exact HUD layout | Scaled GUI crops from original sheets; some panels approximate. |

## Build notes

- Desktop: MinGW + vendored SDL2 — see `BUILDING.md`.
- Tests: `mdaz_tests.exe` (survival, combat constants, craft, bunker, save, systems).
- Successful runs must be recorded in `PORTING_STATUS.md` when claimed.

## Legal

Assets and game design belong to Bohemia Interactive / original authors. This tree is a technical porting effort from publicly mirrored browser files; redistribution of assets may be restricted.
