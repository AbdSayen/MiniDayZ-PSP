# Known Issues & Limitations

Last updated: 2026-10-17 (dmg_hint ported; no invented consumable restores; PSP EBOOT still unverified)

Last updated: 2026-10-19 (step 1–3 queue: 13/17 bodies restored, 3 invented mechanics removed, cross-platform RNG order fix)

## 2026-10-19 open items
- AI (00:24 Baku):
  - Zed vision is approximated: the cone uses the last chase direction, and the C2 Turret/warn wandering is not modelled.
  - At night, zeds without a player light never aggro by sight; this matches C2 eyes, but noise and hit reactions still use port logic.
  - Port crouch input still exists, although C2 has none; it no longer affects zed detection.
- Quest UI (00:11 Baku):
  - The C2 pad (pad_list tabs/pages) is not ported. Only the HUD panel uses the C2 row1 strings; the reward text and time-left helpers are not drawn yet.
  - Port Q_status numbering differs from C2 (C2: 0 none, 1 active, 2 failed, 3 bandits done, 4 none).
  - Reward 31 suffix uses ASCII x (C2 has Cyrillic х).
  - The task 3 row (382) is longer than the 4-line HUD panel and is cut with "..".
  - Tar sync note: files extracted with old mtimes may not rebuild; touch them before cmake --build.
- Weather/sickness (23:59 Baku):
  - Rain intensity mapping rain_intensity_from_level is a port invention; level 1 gives 0.45.
  - The rain effects gated on rain > 0.7 (sprint slow, wetness soak) are not from C2 and never fire at rains_level 1.
  - Other unverified extras found during this audit:
    - fire-rest blood regen and sleep;
    - rain drinking;
    - bush and dig forage.
  - AI event trees not audited yet.
- Craft (23:53 Baku):
  - C2 Craft_3/31–34 clear both ingredient slots (a157 → 0); the port consumes only 1 of each.
  - The 3–5 s craft wait is missing.
  - Craft_21/22 (arrows) are not implemented.
  - The fence "≥5 sticks" rule is unverified against C2.

### Tutorial (23:44 Baku)
- Touch-only buttons are mapped to PSP input: tip touch = Cross (confirm); interact = Circle; inventory = bag; attack/reload/switch = their buttons. The perks button (595/598) has no port menu, so Cross on tips 10/11 stands in. The control-mode choice icons of tip 6 (457/456) are not shown; Cross closes it (stick only).
- walk_marker button highlights are not drawn (no on-screen buttons on PSP). The camera_roll target highlight is a 44 px hole in the dark overlay instead of redrawing the target above the fade.
- Perks/XP system is not ported: tip 10 only sets the tutorial XP value 316.
- Car trunk loot (car_loot_point Spawn_drop) is not ported; the car encounter (id 4) still shows its hint.
- tree_block frames 1–3 are drawn and collided as frame 0 (big_obstacle_base 17). Chopped trees vanish instead of leaving the stump frame / obst_base 8.
- b_exit is not solid and water has no collision (the player can walk onto the boat and the sea).
- Day_cold=1 of the tutorial start is not applied. Final-script text fade length (1 s) is an assumption; the Wait timings are from data.js.
- Saving is disabled while the tutorial runs. The camera_roll id 8 string (476) does not exist in l_eng_ui.xml.
- `--shots-tutorial` uses test-only placements and a Wood piles grant to reach later steps.

| Issue | Impact | Status |
|-------|--------|--------|
| **Real PSP: black screen** (user test 2026-10-19 19:11) with `cpp/platform/psp/deploy/MINIDAYZ`: deploy EBOOT/pak were stale (17.10). Root-cause candidate found in code: `platform_psp.cpp` passed the **RAM** backbuffer pointer to `sceGuDrawBuffer/sceGuDispBuffer` (which take VRAM *offsets*) and to `sceDisplaySetFrameBuf` (display controller cannot scan out of main RAM) → black on hardware. Fix 19:14: two 512×272 RGB565 VRAM buffers at 0x44000000 (uncached) + flip via `sceDisplaySetFrameBuf(...NEXTFRAME)`, CPU frame memcpy'd each `end_frame`; GU offsets 0 / 278528; heap `PSP_HEAP_SIZE_KB(-1024)` (all-but-1MB) instead of fixed 18MB; boot log `ms0:/PSP/GAME/MINIDAYZ/log.txt` (open/append/close per line) for boot, chdir, platform init, each file read result, pak load, free memory before/after `new Game`, init ok/FAILED, first 3 frames; coloured splash with text before asset load (red splash 10 s on init failure). EBOOT rebuilt from current sources with WSL pspdev (psp-gcc 15.2.0, `make clean && make`, exit 0) and deploy/sprites.pak regenerated 19:14 | Resolved: user report 2026-10-19 20:10 — the game boots and runs on real PSP hardware | **Протестирована** (real PSP, user report) |
| **PSP 15 FPS (target 30)** (user report 2026-10-19 19:26, i.e. the 19:14 EBOOT now boots on hardware). Plan approved by the user: per-stage timing in log.txt; GU hardware sprites/tiles with ≤512² textures + batching; ground tile cache in an offscreen VRAM buffer; 16-bit / palettised textures in sprites.pak; optional half-res world render (runtime toggle, default OFF, HUD full-res). NOT doing: CPU overclock, logic-rate change | Playability on PSP  **Done 19:49 (EBOOT rebuilt, make exit 0, deploy regenerated):** (1) GU hardware renderer (batched GU_SPRITES, ≤512 sub-textures via tbw, rotated sprites as strips, T8 font); (2) ground cache: two 480×272 offscreen VRAM ping-pong buffers, previous frame shifted by camera delta with sceGuCopyImage, only exposed edge strips redrawn under scissor; (3) sprites.pak v2 `MDZSPR02`: 1113 textures T8+CLUT8888 (lossless), 9 RGBA5551, 15 RGBA4444, pak 1.78 MB; (6) half-res world: Pause → **HALF RES: ON/OFF**, saved as `half_res=` in `settings.cfg`, default OFF, HUD/text always full-res. Profiling: µs via `sceKernelGetSystemTimeWide`, every 120 frames log.txt line `perf us avg/max ...: logic / ground / objects / hud / gpu(finish+sync) / vsync / frame`; on-screen overlay Pause → **MS OVERLAY: ON/OFF** (`perf_overlay=`). Pre-existing (not added by me) `scePowerSetClockFrequency(333,333,166)` in main_psp.cpp left as is. PPSSPP headless (software GPU) = boot/render check only: menu, gameplay, half-res and overlay render correctly; its timings are NOT performance numbers. Cache vs nocache A/B at the same emulated time: 448 isolated tile-corner pixels differ (~0.3%, 11-px grid) — open, see next row. Real-PSP numbers need the user's log.txt **Hardware result (user report 2026-10-19 20:10): the 19:49 EBOOT runs at a stable 60 FPS on real PSP even with HALF RES OFF; Pause → HALF RES / MS OVERLAY are present and work.** | **Протестирована** (real PSP, user report; no log.txt numbers received) |
| **Ground cache A/B: 448 isolated pixels on the 11-px tile grid differ from direct draw** (PPSSPP software renderer, 2026-10-19 19:52; tile-corner overlap rasterised differently when only strips are redrawn). Visually negligible; workaround: autoplay/dev `nocache`. Needs check on real PSP | Single off-colour pixels at tile corners | Исследована |
| **Ground: large rectangular patches of different shade** (user/parent report 2026-10-19 19:53). Cause: port-side hash picked ground_lvl1..4 per 4×4 cells plus random detail tiles. data.js (Map generation): one ground tilemap per map cell, type chosen by global **CurrentLevel** (0→ground_lvl1 … 3→ground_lvl4); CurrentLevel=0 on Restart_game, +1 only on Level_change (not ported); the tilemap template (Map layout instance) is tile 6 = atlas cell (1,1) on 5975 of 5985 cells. Fix 20:08: whole map uses ground_lvl1 cell (1,1). Shot: /workspace/q2_explore1.png | Patchwork ground | **Сверена с оригиналом** (data.js) / Протестирована (desktop shot) |
| **Minimap black** (19:53). Cause: port-side "visited fog" darkened every unvisited sector at 82% black (no fog in data.js) over a dark 95% backdrop, and only frame (1,1) of minimap_tile was drawn. Fix 20:08: per-cell minimap_tile frame from the C2 Map-generation mapping (cell value 0→frame 0, 6→6, 9→4, 11→8, 12→2, 13→1, 14→3, 15→9; 29 frames from data.js), no fog, POI marks for all locations. C2 draws the tiles on the transparent GUI_elements layer at 20 px/cell; the port shrinks the 16×16 map to 56×56 and adds a light backdrop (port-side) so the black/alpha icons are readable. Neighbour-road frame overrides (511 conditions) not ported. Shot: /workspace/q2_minimap_crop.png | Map unreadable | Протестирована (desktop shot); neighbour overrides Исследована |
| **NPC forest collision** (zeds/bandits/wolves/dogs/deer walked through forest). data.js: their movers are Bullet behaviours with "Bounce off solids"=1 and tilemap_forest is Solid. Fix 20:08: `Game::npc_forest_block` after each AI update (same per-tile forest polygons as the player; move undone per axis instead of a Bullet angle reflection; NPCs spawned inside may walk out; crows excluded) | NPCs inside forest | Реализована (approximation of Bullet bounce), test_queue_q1 |
| **18 bodiless Game functions**: audit found **17** member functions declared in game.hpp with no definition and no callers (bodies lost before this session; no git history, no backup). 20:08: `hurt_zombie`, `hurt_zombie_bullet`, `hurt_animal`, `hurt_bandit`, `pack_loot_view`, `apply_loot_view`. **20:30 (step 1), from data.js events:** `tick_molotov_burn` (Molotov params + Molotov_damage: NPCs overlapping the 250 px light_sprite (~92 px radius) burn 5 s, 10 dmg every 0.5 s, tank 15; zeds/bandits/wolves/tank only; **molotov blast damage removed**, data.js has none), `zed_check_bunker` + `spawn_bunker_zeds` (every 3 s in bunker, spawner 800–1500 px away, once per spawner; choose(1,2,3,4,1,2,3,4,5) → 5 fast / 7 army / 3 shooter / 1 shooter+4 army / 1 tank, ±200/±100 jitter; C2 px scaled 8/30 to the port bunker; **the fixed 4–7 zeds spawned on bunker entry were removed**), `notify_weapon_unlock` (Show_unlock_wpn toast 3 s + sound; now fed by zed kills via weapon_unlock_on_zed_kill and Veteran bleeding seconds → VSS — both existed but were never called), `on_quest_completed` (pad complete: item/case slot removed, reward box, Karma +10/+10/+20), `maybe_spawn_quest_bandits` (Generate_quest_bandits → Spawn_bandit_camp once at the quest anchor), `try_build_fence` (build_fence1/2: 5 sticks + woodpile slot cleared → obst_barricade at player.x+50, no lifetime, only zed_tank breaks it; replaced the invented "woodpile + 2 sticks, 480 s, zeds batter 1 %/frame" fence). **20:36:** `sync_location_stream`, `spawn_loot_for_poi`, `spawn_hostiles_for_poi` ported from the recovered pre-loss game.cpp (provenance in PORTING_STATUS) and wired to the C2 Location loader timing. **Not ported:** `maybe_add_hammer_loot` — the recovered body (3.5 % sledgehammer in any pile) is invented; the C2 hammercrash (Humvee wreck, 3 type-18 loot points) lives in create_map_location, whose element/loot-point layout is still the port's per-type pile model | Lost features | 16/17 Протестирована (test_queue_q1); maybe_add_hammer_loot Исследована (needs create_map_location) |
| **dmg_hint never shown** (DmgHintSystem existed, nothing spawned or drew it). data.js: function creates obj529 Text on layer Rain at the target, Bullet angle = angle(player → target), text "-"+dmg, colour (255,196,68); param2==1 → size 18, red. Fix 20:08: spawned by hurt_*; drawn in the world pass. Fade alpha and the 18 px size are not reproduced (fixed 5x7 font, no alpha in draw_text); crit flag never set by the port | No damage numbers | Реализована, но не проверена visually (test checks spawn) |
| **Death animations**: fixed again 2026-10-19 (item 7). Frames now come from the *_dead objects (die_anims.inc), the last frame stays; deer/rabbit/dog included. Open: the corpse angle is 0 (zed facing at death not applied); the tank shows frame 0 of its dead Default anim; bandit corpses still use the static bandit_dead frame | — | Протестирована (unit + --shots-death) |
| Small fonts: data.js Text objects use system/web fonts (Arial 8–18pt, Pixel Cyr 8pt, Tahoma, Calibri Light); there is no bitmap font in the assets, the port uses its 5x7 fallback ("HUD font: 5x7 fallback"). No change made — needs a decision on target size | Text style differs | Исследована |
| Found while restoring step 1 (not changed): quest completes automatically when the condition is met (C2: player presses the quest-pad complete button); C2 also adds expierence_points +30/+40/+50 and char_unlock_15_progress (no XP/character-unlock system in the port) | Quest flow differs | Исследована |
| Bunker spawners: the port table `kBunkerZedSpawners` (21) comes from the Polygon test layout; on Map, Bunker_generate creates 5 / 13 / 21 spawners per bunker level at HotspotX/Y offsets (C2 30 px tiles, level 1..3), and the port has one bunker map at 8 px/tile. Spawn logic is faithful, positions are approximate | Spawn positions approximate | Исследована |
| Item stack sizes are a name heuristic (item_db.cpp classify); sticks (38) do not stack, so build_fence counts 5 sticks over all slots (C2: one stack >= 5) | Inventory differs | Исследована |
| Molotov also places a short campfire (port visual for the flame_medium/flame_small sprites); zed kills drop loot_.spawn_random — both not yet checked against data.js | Possible deviations | Не исследована |
| ~~Invented mechanics: crows scavenge zed corpses, zed corpse life ×1.15, animals take 0.8× explosion damage~~ — **removed 2026-10-19 (step 2)**: on_zed_killed spawns no crows, corpse life = Bot_corpse_stay_time, explosion damage to animals unscaled. Tests: "zed kills spawn no crows", "zed corpse life = Bot_corpse_stay_time" | — | Протестирована |
| Same seed (140), different map per platform (desktop helicrashes (15061,8125) (8367,10625) (1673,13125) vs PSP (7252,16875) (9483,9375) (11714,1875)). **Cause found (step 3):** not RNG/float/int width — C++ leaves the evaluation order of function arguments unspecified. `add(t, rng.next_int(..x..), rng.next_int(..y..))` and `Vec2(x + rng.range(), y + rng.range())` drew y first on MinGW/x86 and x first on psp-gcc (the swapped x/y is visible in the coordinates). **Fix:** `Rng::int_pair` / `Rng::range_pair` (sequenced, x first); mapgen place/bunker entrance + 25 jitter sites in game.cpp, bandit.cpp, spawn_global.cpp, spawn_params.cpp rewritten; a scan finds no statement left with two rng calls in one argument list. `MapGenerator::poi_hash()` (FNV over type/sector) is logged on new game ("poi hash ...") on both platforms. Test: prints the full seed-140 POI list + hash and checks POI #0 equals a hand replay of the x-then-y draws. Note: the map for a given seed changed on desktop (old saves made on desktop load into a different world) | Cross-platform | **Протестирована** 20:36: PPSSPP log and desktop stdout of the same build both give helicrash (7252,16875) (9483,9375) (11714,1875), poi hash 7cbc5967 |
| Helicrash locations are logged on new game (`helicrash #n at world (x, y) sector (sx,sy)`, stdout / PSP log.txt). Debug teleport was added and then removed at the user's request (20:09) | — | Реализована |
| **USE acts on first item, not selected slot** (user report 2026-10-19 19:20). Cause: `Game::use_equipped_item(true)` (△ / USE in play) (a) scanned the WHOLE bag for gasoline 53 / toolbox 59 before looking at the selection and (b) when the selected item was not consumable (weapon, tool, empty slot) fell back to the FIRST consumable in the bag; it also played the use anim/sound when nothing was used. Fix 19:27: USE acts only on `equipped_slot_` (inventory Cross sets it to the focused slot temporarily), fallback removed, vehicle items only from the selected slot; craft index clamped in `update_craft_menu`; save-loaded `equipped_slot_` clamped; hotbar switching via `Game::next_hotbar_slot`. Audited, already correct: drop (focused/equipped), equip/to-hand swap, reload (ammo for selected gun), craft (selected recipe), inventory focus clamp + open-sync to equipped. Loot transfer has no focused-item UI (interact takes the whole pile, as before) — no selected-index path to fix. Not changed: F throw prefers selected grenade/molotov/stick, else scans the bag (dedicated hotkey); auto heatpack at <18° scans the bag. Tests: `tests/test_selection.cpp` 21 checks, `mdaz_tests.exe` ALL TESTS PASSED 19:27 (the vehicle-refuel branch is not exercised: no vehicle in the test) | Wrong item consumed/used | Протестирована (desktop) |
| **HUD vitals icons wrong** (user report 2026-10-19 19:23). Cause: `draw_vital_bar` drew `gui_panel-sheet0` frame (1,1) — the HEART — for all four vitals, squashed 60x20→64x14, with the bar at a guessed inset overlapping the icon; warning icons drew the whole 32x32 sheet scaled down (blank frame 0 included) in a row under the bars. Fix 19:33 from data.js: gui_panel type t0 has 5 frames = blood(heart) sheet0(1,1) / water(drop) sheet0(1,23) / food(fork+knife) sheet1(1,1) / **temperature (thermometer)** sheet1(1,23) / radiation sheet2(0,0); GUI_layout uses frames 0,1,2,3 in that order (4th = temperature, not stamina). Drawn at native 60x20 (PSP 1x of the 2x C2 layout), bar strip (C2 1x5 gui_*_bar) at native (22,7) 35x5 = track interior; colours from the strips (blood #DA4849, water #5DB5B7, food #60A057, heat #A4951E). Warning overlays use frame 1 (red silhouette) at GUI_layout offsets: thirsty/starving/heat over their panel icon, bleeding+sick over the blood bar; blink ~2 Hz is an approximation (C2 toggles frames from events, not decoded). Verified on `--shots` explore1 (copied to /workspace/mdz_hud_vitals_explore1.png) | Misleading HUD | Протестирована (desktop shots); PSP not yet checked |
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

## Building walls (2026-10-19 20:50)

- Solid polygons from data.js now drive building collision (fidelity #1). Zombies, animals and bandits now collide with building polygons too (their C2 Bullet movers have bounce-off-solids = 1). Port slides along the wall instead of reflecting the Bullet angle. Vehicles use only a point-in-wall test. Port guard: a player who starts a step already inside a wall polygon (for example an old save) can walk out.
- Which exterior/interior pair a POI shows still comes from the invented `poi_layout.cpp` pair layout (Buildings_base prefab XY + create_map_element is item 5) — only one building per POI (the drawn one) is collided now. The second `poi_layout` part was never drawn; its invisible cell blockers are removed.

## Weapons / NPC stats (2026-10-19 21:05)

- Not ported yet: headshot/crit roll (random(100) < 20 + bullet IV9 + perk bonus gives x3 damage on certain zeds), fast-mag attachment (IV18) that shortens reload, and scope-dependent IV9 values.
- Bow and crossbow: C2 picks arrow_bullet (50) or arrow_bullet_c (75) from the loaded arrow type (IV11). The port ties bow to 50 and crossbow to 75.
- Weapon range, fire interval and dispersion are still port values. C2 bullets carry IV3=400 (Не исследована).
- Zed/animal speed random(lo, hi) is rolled from a hash of the entity id, not the game RNG. Warn/idle speeds (b_warn 80–90, speed /4 modifiers) and aggro radii are not ported. Crow HP 10 is a port value.
- C2 weapon slot ids 9, 15 and 20 have reload entries but no pickup name, so they are unmapped.

## Inventory slots (2026-10-19 21:05)
- Container sizes for Bulletproof vest 400 / Kevlar 404 not found in data.js -> 0 slots; Molle vest (t119, 3 slots) has no port item id.
- C2 drops a container's contents when the piece is taken off/replaced (Cut_arrays); the port keeps items in the flat 24-slot array, slots above capacity() are just inaccessible.
- Pickup still merges into existing stacks (port); C2 Check_space_* writes into an empty slot. Ammo stack_max bandolier bonus still from the port (not verified).
- Only Player_skin_type 1 starter ported (skins 2..15 have other Spawn_drop sets; no skin selection in port). Starter perks per difficulty are still a port scaffold.

## Trees (2026-10-19 21:05)
- Trunk collider uses obst_base frame 10 at its natural 15x17 size. C2 created objects take the size of the layout template instance (placed obst_base instances are 61x93 = frame 9 size); if that applies, the real collider is ~4x larger. Unverified.
- Bag screen still draws the port's ClothingContainers grids (4 invented sub-slots per region); capacity counter (e.g. 1/3) is the C2 value.
- Tree placement still invented (scatter around POIs), pending item 5 (map_element types).

## Pickup / gear (2026-10-19 21:20)
- Port keeps one flat slot array: on gear swap the items beyond the new capacity drop next to the old piece instead of staying inside it (C2 per-item container arrays). Re-wearing does not restore them.
- Gear already in the bag (old saves) can still be worn via wear_clothing (swap into the bag slot).

## Building interiors / location loader (2026-10-19 21:20)
- Done: per-building Solid props + spawn_loot_point positions (36 building subtypes).
- Not done: create_map_location element program (array 518) -> real building/tree/obstacle/Humvee (hammercrash_point) placement; port world gen (location types/counts) is still its own model, so prefab XY and tree placement remain port-made. Needs generate_array_locations port first.
- Loot per point: Trigger_spawn tables (per spawn_loot_point IV0) not ported; pile count still the port's.
- Bench (autoplay) draw times drop to 0 at ~42 s because the bench character is killed (last_death.txt: reason=Bullet) -> death screen. Not a renderer bug.

## Walls vs bullets / sight (2026-10-19 21:35)
- Ported: bullets stop on building walls, interior props, tree trunks; NPC sight/chase/melee/shots need LOS (buildings + big obstacles).
- Not ported: C2 noise ("noice") still lets zeds walk toward a sound without LOS in the original; port aggro = hearing radius gated by LOS (zeds behind a wall stay put). Bullet bounce-off-solids for NPC Bullet movers not modelled (bullet is destroyed, as the Hitting_obstacles event does).
- Tank rush into a wall -> stun (zed_tank_eyes vs t336/t339/t927) not ported.
- Player melee is instant in the port (C2 melee_swing is a t910 bullet and is stopped by walls).

## Noise (noice)
- Ported: gunshots and F1. Not yet: flare 250, burning zed 100, Public enemy 1500, deer/rabbit flee, wolf move +-100, the random-angle warn variant, the idle-only (IV0==0) gate (the port warns any zed not chasing by sight).
- For bullet types with two radii (179, 461, 177, 235) the port uses the larger; the variant mapping hasn't been researched.
- Random rolls come from a hash, not the RNG (deterministic).

## HUD hotbar
- The cell layout is the port's own (one row, at most 8): the original groups cells by container (prototypes in GUI_layout/Rust_layer 7+3+3+3+1). The visual grouping and hider sprites aren't ported.

## HUD portrait
- Always the 'White' skin frame 0; per-skin anims (Black/Yellow/...) and blinking frame 1 aren't ported. On PSP, if sprites.pak has no gui_btn_perks the old skin sprite is the fallback (same position).

## Map generator (c2_mapgen)
- Not yet wired into the game: buildings (cat 7) are still placed by the old POI list (invented types Police/Church), along with obstacles cat 4, poles cat 5, cars cat 6, bushes/pumps cat 3, zed_resp spawners and the Humvee/heli from C2Map. The tilemaps of create_map_location (roads/water/forest SetTile) aren't ported.
- Trees overlapping helicrash/hammercrash/t426 aren't removed (C2 removes them on overlap).
- Its own Rng stream (seed*2654435761+0x9e37); the original uses Math.random, so maps can't be compared one-to-one.
- Tree draw order isn't sorted by Y relative to the player (same as before).

- Интро: на PSP до загрузки sprites.pak всё ещё показывается старый текстовый "LOADING..." boot_splash (в jacksmith сплэш виден с первого кадра GU). Ассет в паке весит ~130 KB T8 после декомпрессии; на реальном железе не проверено.

- B2a: b_deer_stand (cat 7 sub 1, 93 rows on seed 140) is not drawn/solid yet; loot piles inside buildings are drawn above the roof; kMaxLootPiles=96 may fill up with many active buildings; minimap sector size 1115 px vs C2 cell 1020 px.

- B2b: not ported yet - car_loot_point / spawn_loot_point of props (car loot), light_tower night lights on pillars 12-14, berry-bush harvesting tied to the bush object (old bush search is still the port's random chance), trees destroyed when overlapping heli_crash / b_dot / b_dot_int (create_map_element c108 checks), layer order (props drawn under trees, no Y-sort).

## B2c known issues (2026-10-19)
- Humvee loot (C2 spawn_loot_point type 18) and heli loot (type 15) use the port's placeholder tables
  (spawn_military / spawn_helicrash) until Trigger_spawn tables are ported (item 11 area).
- Spawn rejection checks Solids only; water (199) / ground family 202 overlap waits for B2d tilemaps.
- Zed_check uses CurrentLevel = 0 (tank branches need level > 1).
- ach_heli_counter (achievement) not ported. zed_buried_hidden (1/5 per cell), crows, bot teams and
  bandits from create_map_location are still not ported.
- dec.py prints the event OR-block flag as "else"; real else events use condition c70. Earlier
  hand-ports that read "else" from dec output should be re-audited.
- Invented vehicle placements (car/boat/bike near start) remain.


## B2d known issues (2026-10-19)
- C2 Wait 0.5 actions inside create_map_location are ignored (tilemap edits applied immediately).
- Ground templates 681/682/771 are 17x17 (last half column empty) like the layout data; t475 is 18x18.
- Water tilemaps are drawn but not yet used for zed spawn rejection, wading or fishing.
- Road edge appearance not yet compared side by side with the original.
- PSP: 40 slots x ~1.9 KB; performance on PSP not measured.


## c90 = For each (2026-10-19)
- Any earlier port code that treated System c90 as "pick random instance" must be re-audited.


## 2026-10-20 00:40 (Baku)
- Dogs in the port still roam/chase like hostile animals but deal no damage; the original dog is a player companion (gui_dog_control). Not ported yet.
- Wolf vision cone (180°) not implemented: Animal has no facing field. Zombie wandering, bandit aim/shoot timing and screamer audit still pending.

## PSP hint wording (deliberate, user-requested deviation, 2026-10-20)
The user explicitly asked that hints naming a keyboard/mouse/touch control use the PSP buttons. Original text (l_eng_ui.xml) -> port text:
| id | Original (C2) | PSP port |
|---|---|---|
| 33 | Movement: By tap | Movement: analog stick / D-pad |
| 230 | ...Shoot by using the button... | ...Shoot by pressing Cross... |
| 454 | ...Use items by tapping. | ...Use items by selecting them with the D-pad and pressing Cross. |
| 456 | Move by tapping/drawing | Move with the D-pad |
| 457 | Movement with the stick | Movement with the analog stick |
| 460 | Choose one perk and tap on the plus button. ... | Choose one perk and press Cross to take it. ... |
| 461 | Use the interact button to pick up items and interact. | Use the Circle button to pick up items and interact. |
| 462 | ...Use this button to shoot. | ...Press Cross to shoot. |
| 463 | You can reload your weapon by tapping this button. ... | You can reload your weapon by pressing Square. ... |
| 464 | Switch between melee and ranged weapons mode. | ... mode with L + Left/Right. (original named no button; the C2 tip highlighted one) |
| 467 | Use drag and drop to manage items in the inventory. | Use the D-pad to choose items in the inventory: Cross uses, Triangle puts in hands, Square drops. |
Open/ambiguous:
- 422 "Push this button when ready for next wave." (wave mode) not changed: the mode is not ported, so there is no PSP button to name.
- 458 "Choose the movement controls mode" still shown, but the port has no mode choice (Cross only closes the tip). Not reworded: it names no control. Needs a decision.
- 460: the port has no perk-choice UI; Cross only closes the tip.
- Desktop builds show the same PSP wording.

## Car trunks (2026-10-20)
- PSP adaptation: the touch interact button (gui_btn_interact frame 3) is drawn as a 25x25 icon left of the minimap; Circle presses it; hint bar text "TRUNK".
- walk_marker cancel rules are not decoded: the port cancels on any move input or after 5 s. Walk uses the port walk speed (C2 AltMove SetSpeed 100).
- Spawn_drop count rules (ammo amounts etc.) are not applied: 1 item is dropped.
- The walk_marker branch lists (sid7711324247664447) were assumed identical to the direct-open branch; only its first list was compared.
- t920 (overlapping wreck marker frame 1 / map_element IV3 = 1, i.e. looted state for save/reload) is not ported; trunk state is not saved.
- Precedence vs other avalible_interaction values (pump 4, obstacle 7, panel 8, loot) is not applied; in the port, loot pickup runs first.
- The port's drivable-vehicle "stash" (drop near car stores items) has no data.js source (Trunk_count is never used in events). Left as is; needs a decision.
- **Rng::next_int bias (whole port):** LCG `next_u32() % span` uses the low bits; `next_int(0,1)`/`(1,2)` alternate per call. Affects every choose() port (berry frames, loot rolls...). Not changed globally (it would change the seed-140 map hash); needs a decision.
- Original-vs-port gameplay screenshot not captured (the browser original needs manual play to reach a car); compared against the data.js sprites (car_loot_point-sheet0/1 white outlines, gui_btn_interact-sheet0 frame 3).


## Rng / stash (2026-10-20 01:20)
- RESOLVED: Rng::next_int low-bit bias (fixed globally, poi hash now 438baa57; old saves load into a different world).
- RESOLVED: invented vehicle stash removed.
- Trunk findings still open (next item): car frame 1 (trunk lid raised) on open via family t920; two-stage walk (wp1/wp2 nearest imagepoint IV0=7, then loot point IV0=4, speed 100); walk branch lists differ from the direct branch (frame0 list: 24 vs 26; frame1 list differs); tutorial frame 1 uses choose(2,8,9,4); sound "open" at -5 dB; ach_gta +1 when SEED_RUN=0; Spawn_drop quantity rules; save state.


## Car trunk leftovers (2026-10-20 02:05)
- avalible_interaction order (sid3971809969300003): 5 (items t912) / 6 (t909) suppress all; then 1 plants, 2 bush, 3 trunk, 4 pump, 8 panel, 7 obstacle (later wins). Port: loot pickup runs first; trunk vs pump/panel/obstacle arbitration to be finished with queue items 4 (pump) and 5 (obstacles).
- Spawn_drop non-count attributes (weapon IV6 condition / IV5 / mag sub-events, clothing containers, knife/canteen/battery IV1 fill or charge) are not carried by ground loot piles in the port; items spawn with count 1. Needs a loot-pile attribute field (later item "loot tables").
- Check_ach_unlocks / achievements UI not ported; only the ach_gta counter is kept and saved.
- walk_marker collision: player_base instance assumed 2x2 (sprite size) vs walk_marker poly 4x4 -> arrival window +-3 px.
- No explicit walk cancel in data.js; PSP: stick/D-pad input cancels (documented adaptation).
- Original-vs-port gameplay screenshot still not taken (browser original needs manual play); car frames compared with car_*-sheet0 frames 0/1.

## House_transparenting not fully ported (found during item 2)
C2 group House_transparenting (Every 0.1 s, player_collision_base overlapping t928): when not in the warmzone, the interior plays anim "out"; if player Y < exterior imagepoint 1 the roof goes to opacity 25 and the interior moves to Buildings_base_top with Tint + outline (t943 Outline param 3 = 1); otherwise opacity 100. Exterior layer: player Y >= imagepoint 2 -> Buildings_bg, else Buildings (above player). door#340 opacity 100/25, rain_roof/rain_loop audio swap. Leaving t928 (TriggerOnce) resets opacity 100, Buildings_bg, anim "in". The port has no exterior imagepoints 1/2 in BuildingShape yet, so the 25 % roof and the layer swap are missing; scheduled with item 7 (global depth sort).

## Car hitbox notes (item 3)
- Hatchbacks (subs 15..17) did not spawn on the seed used by --shots-carhit, so their in-game screenshot is missing; they are covered by test_car_hitbox only.
- Other SetSize obstacles not from c1/c2 still use native size: obst_barricade (SetSize from its own image points 1/2, expression not evaluated by gen_base_sizes.js), b_gas_station_fueling_int (constant 21x100), door SetWidth. Not cars; to check in the KNOWN_ISSUES pass.
- Player is drawn over the car sprite when south of it: depth sort is item 7.

| **PSP: analog stick dead + Map instead of Tutorial (user report 2026-10-20 02:25/02:31)** | Probable cause: a leftover autoplay.txt on the memory stick. Scripted input replaced the pad, and dev_skip_tutorial skipped the Tutorial. Fixed: a banner is shown and any pad input cancels autoplay. The Makefile also lacked header deps, which left a stale game_tutorial.o in the 02:17 EBOOT. walk_marker (trunk) checked: any stick/D-pad input cancels it, so it cannot latch. If it happens again with no autoplay.txt, unconfirmed transient: send log.txt | Fixed, not confirmed on hardware |

| **PSP sound vs C2** | (1) The HRTF panner is replaced with equal-power panning. (2) Which game events loop sounds, use tags or set dB volumes is still the port's existing call sites, not checked 1:1 against data.js Audio actions (Play looping / PlayAtObject / SetVolume / Stop tag); ambient/rain loops are played once. (3) The listener is the player (C2 listener object not checked). (4) The first play of a short clip decodes on the main thread (short stall). (5) Not listened to on hardware or in a PPSSPP window | Open |

## Звук 1:1 (2026-10-20, аудит docs/AUDIO_AUDIT.md)
- Day&Night ставит `CurrentLevel>=2 ? ambient/night_loop : MD_winter_amb/MD_Winter_night_amb`. В порту CurrentLevel = 0, поэтому, как и в C2, на первом уровне играет зимний эмбиент. Это буквальное воспроизведение, не ошибка порта. Bunker_erase проверяет `CurrentLevel==1`, и это тоже повторено.
- Слоя Login (StopAll + ambient + SetMasterVolume(lastaudio)) в порту нет: запуск идёт Loading → Menu, и ambient −10 дБ стартует в Menu_init.
- Тайлмапа песка t411 порт не генерирует, поэтому sand_run не звучит никогда. Шаги в зданиях без warmzone IV1 и в бункере (вне t945) молчат, как в C2.
- SetPlaybackRate двигателя = 1+abs(Speed/MaxSpeed). Машина порта либо стоит, либо едет на максимальной скорости, поэтому rate бывает только 1 или 2.
- Тег fireplace_loop строится по слоту костра (в C2 по UID). Огонь молотова считается не камином.
- Не перенесено (нет исходного объекта или состояния): крик танка −10 дБ (нужен автомат состояний танка), ground_hit (t755), body_1/2 −15 дБ при попадании пули в t472 (не установлено, какие объекты входят в t472), звуки тела при ударе в ближнем бою.
- Гранаты/молотов: в механику броска звук не вносился. Звуки — "gren pin" (item_activate 21) и punch_swipes_0 +10 при создании молотова.
- Длинные файлы (>96 KB) стримятся. Одновременно декодируются не больше 4 потоков (kMaxStreams), лишние становятся виртуальными голосами: тег сохраняется, звук молчит, ближайшие получают декодер первыми. Это ресурсная адаптация под PSP.
- Свободный ящик Pick_player_item в окне одежды всегда играет gearpick (оружие через этот путь не берётся).
