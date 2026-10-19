# Porting Status

> Log policy (2026-10-19): this file and KNOWN_ISSUES.md are updated right after **each** fix/step, with the honest status of that step.

Last updated: 2026-10-19 (steps 1–3: restored bodies, invented mechanics removed, cross-platform map determinism)

| 2026-10-19 | **Build repair:** `game.cpp` builds again after the lost-code incident (backups `game.cpp.pre_fix_bak`, `game.cpp.bak_render`, `game.cpp.bak_ui`). 22 `Game` methods were declared but undefined; they link because nothing calls them yet (list in KNOWN_ISSUES). Status: **Протестирована** (`cmake --build build -j` OK, `mdaz_tests` ALL TESTS PASSED, `--shots` 13/13 saved). |
| 2026-10-19 | **Ground atlas fix:** `ground_lvl1..4_tilemap` are 300×300 atlases of 5×5 60 px cells; cells (3..4,2..4) are empty (transparent) and (0..2,0..2) is a dirt→grass autotile. Old code tiled `gx%5/gy%5` over the whole atlas → transparent holes and stretched patches. Now the seamless grass centre (1,1) is used, with rare detail cells (1,4)/(2,4)/(2,3). Biome choice per 4×4 cells is a port-side hash (the C2 runtime ground fill is not decoded). Status: **Реализована, проверена по скриншотам**; not **Сверена с оригиналом** (biome layout). |
| 2026-10-19 | **Tile seams:** tilemap cells are drawn at floored screen positions with `ceil(size)+1` px, so the 0.55 world scale leaves no 1 px gaps between tiles (`WorldTilemaps::draw`). Status: **Протестирована** (shots explore1/explore2, no seams visible). |
| 2026-10-19 | **Render bug (explore1 "huge stretched sprite"):** POI buildings were drawn with a 48–140 "max_dim" passed as the *scale* → one building sheet blown up ~100× (flat colour squares over the screen). Now drawn at native data.js frame size × world scale (0.55); `spawn_loot_point` editor marker no longer drawn as loot. Status: **Протестирована** (shots explore1/explore2). |
| 2026-10-19 | **Ground loot size:** clothing/weapon piles used the 30×30 worn/held frame ×1.5 (the "giant blob"). `find_item_ground_icon()` now uses the C2 `on_ground` animation frame (98 types, `src/ui/item_ground_frames.inc` generated from data.js; e.g. hunter backpack 15×10, AKM 19×7, jeans 14×9) at native size; small items keep their single frame. Inventory/hotbar icons use the same frames, integer-scaled to fit. **Протестирована** (shots). |
| 2026-10-19 | **View-rect culling:** all world passes (ground 60/30 grids, water fringe, berry bushes, blood, corpses, trees, POIs, loot, zeds, animals, bandits, vehicles, fires, traps, crops, bullets, fx) cull by the 480×272 screen rect / scale + per-sprite margin (`ViewRect` in `Game::render`); grid loops derive index ranges from the view. Replaces fixed dist²/±N cell windows. **Реализована, проверена по скриншотам.** |
| 2026-10-19 | **Forest collision:** `tilemap_forest` (t202) has the **Solid** behaviour in data.js; per-tile collision polygons (t202[13], `include/mdaz/forest_collision.inc`, null = full box) → `WorldTilemaps::forest_solid_at`; player slides along edges, car crashes. Forest is now drawn from the real Map-layout tilemaps instead of the invented 55 %-alpha canopy over every cell. Tree sprites (tree_pine etc.) are **not** Solid in C2; the existing 14 px trunk push-out is kept and the pine is now anchored at the trunk base (Imagepoint 1, 0.53/0.924) so it matches the drawn trunk. `test_map_fidelity`: "tilemap_forest Solid per-tile polygons". **Протестирована** (unit test); visual check of forest edges pending (forest only at map borders in the layout). |
| 2026-10-19 | **20:08 queue:** ground = one type (ground_lvl1 tile (1,1), C2 CurrentLevel 0); minimap per-cell minimap_tile frames, no fog (was black); NPC forest collision (Bullet bounce-off-solids); restored hurt_zombie/_bullet/hurt_animal/hurt_bandit + dmg_hint + die1/die2 anims, pack/apply_loot_view (save keeps loot); helicrash coords logged. 11 of the 17 bodiless functions still missing. `mdaz_tests` ALL PASSED (test_queue_q1 20 checks). EBOOT rebuilt 20:11 (make exit 0), deploy regenerated. Status: see KNOWN_ISSUES rows |
| 2026-10-19 | **20:36 batch close:** (a) **Location stream** — `sync_location_stream`, `spawn_loot_for_poi`, `spawn_hostiles_for_poi` ported from the **recovered pre-loss game.cpp** (`C:\Users\abdur\mdz_history\recovered`, replayed from agent transcripts, not verified byte-exact; reviewed against data.js Location loader / Reload_locations*): new game creates POIs within respawn_radius2+1000 (was: every POI at once, and none were ever `active`, so POI collision/footprints were never applied); every 1 s, moved > 400 px and not in bunker → Reload_locations (erase > 2000, create ≤ 2000; loot seeded once, hostiles per creation); load → Reload_locations_aftersave. `reload_radius_for` start/aftersave fixed to r+1000 (C2 add; was min(r,1000)). Not ported from the recovered code: `maybe_add_hammer_loot` (3.5 % sledgehammer roll — invented; C2 hammer = hammercrash_point) and the `helispawned_ >= 3` cap (C2 uses 0/2/3/4 per CurrentLevel). (b) **PSP parity verified:** EBOOT 20:34 (make exit 0) in PPSSPPHeadless logs helicrash (7252,16875) (9483,9375) (11714,1875), 28 locations, **poi hash 7cbc5967** — identical to desktop stdout of the same build. `mdaz_tests` exit 0 (259 OK lines). Deploy regenerated 20:36. Status: **Протестирована** (desktop tests + PPSSPP boot log; not on hardware) |
| 2026-10-19 | **Step 3 – same seed, same map on PSP and desktop:** cause = unspecified argument evaluation order (two rng calls in one call → x/y drawn in opposite order by MinGW vs psp-gcc). `Rng::int_pair/range_pair` (x first) in mapgen + 25 spawn-jitter sites; `poi_hash` logged on new game; test prints the seed-140 POI list (28 POIs, hash 7cbc5967) and checks POI #0 against a hand replay. test_systems "Reload_locations_start" probe moved to a real POI (it depended on the old layout). Status: desktop **Протестирована**; PSP see next row |
| 2026-10-19 | **Step 2 – invented mechanics removed:** crows at zed corpses, corpse life ×1.15, animal explosion ×0.8 (none in data.js). **Протестирована** (test_queue_q1) |
| 2026-10-19 | **Step 1 – 7 more bodies restored from data.js:** tick_molotov_burn, zed_check_bunker, spawn_bunker_zeds, notify_weapon_unlock (+ unlock progress wired to zed kills / Veteran bleeding), on_quest_completed, maybe_spawn_quest_bandits, try_build_fence. Removed with them: molotov blast damage, fixed bunker-entry zeds, invented fence recipe/lifetime/battering. 4 remain blocked on the location loader (KNOWN_ISSUES). `mdaz_tests` exit 0 (test_queue_q1 +18 checks). **Протестирована** (desktop unit tests); in-game visual not captured |
| 2026-10-19 | **20:10 user report (real PSP):** 19:49 EBOOT boots, stable 60 FPS with HALF RES off, pause HALF RES / MS OVERLAY work → black screen and 15 FPS rows **Протестирована** on hardware |
| 2026-10-19 | **PSP perf (19:49):** GU renderer + batching, ground VRAM cache (edge redraw), sprites.pak v2 (T8/5551/4444), reversible half-res (Pause → HALF RES, default OFF), per-stage µs profiling (sceKernelGetSystemTimeWide, avg+max every 120 frames: logic/ground/objects/hud/gpu finish+sync/vsync/frame) + Pause → MS OVERLAY. Desktop: tests ALL PASSED, explore1 shot pixel-identical to the pre-change one. PPSSPP: boot/render only. Status: **Реализована, но не проверена** (hardware log.txt pending). |
| 2026-10-19 | **HUD vitals (19:33):** 4 panels use the correct gui_panel frames (heart/drop/fork-knife/thermometer), native 60x20, bar strip at (22,7) 35x5, warning overlays frame 1 at GUI_layout offsets. Status: **Протестирована** (desktop shots), **Сверена с оригиналом** for frames/offsets/colours; blink timing approximate. |
| 2026-10-19 | **Selected-slot fix (19:27):** USE acts on selected slot only (no first-consumable fallback, no whole-bag gasoline/toolbox scan); craft index + loaded equipped index clamped; `Game::next_hotbar_slot`; new `tests/test_selection.cpp` (21 checks: use, inventory use, drop, equip/swap, reload, consume-then-select, focus clamp, craft selection/clamp, quick-slot switching). Status: **Протестирована** (desktop tests); PSP not run. |
| 2026-10-19 | **PSP black-screen fix (19:14):** framebuffer moved to real VRAM double buffer (was RAM pointer handed to GU/display), heap −1024 KB, early boot log `ms0:/PSP/GAME/MINIDAYZ/log.txt` + pre-load splash; EBOOT rebuilt from current sources (WSL pspdev, exit 0), `deploy/MINIDAYZ` + `sprites.pak` (1137 sprites, 1.94 MB) regenerated. Status: **Реализована, но не проверена** (no emulator/hardware run yet). |
| 2026-10-19 | **PSP UI rewrite:** new `mdaz::ui` toolkit (`psp_ui.hpp/.cpp`: text fit/wrap, panels, focus frame, PSP glyphs ×○△□ L R START SELECT D-pad, hint bar, list menu); HUD (`hud.cpp`) re-laid for 480×272 (vitals row, portrait, day/FPS, quest panel 2 lines, compass, hotbar 8×22, minimap 56×56, context hint bar), no touch chrome; full-screen inventory and craft panels with D-pad focus; main / pause / death / controls screens (`screens.cpp`). `InputState` gained `back_pressed` + `nav_*` (auto-repeat in `Game::compute_nav`). PSP Makefile lists `psp_ui.o`, `screens.o`. Status: desktop **Протестирована** (build, tests, shots); PSP **Реализована, но не проверена** (EBOOT not rebuilt). |
| 2026-10-19 | **Restored Game methods (UI part):** `close_inventory`, `sync_bag_overlay_from_ui`, `toggle_craft_menu`, `try_craft_menu_recipe`, `try_cancel_quest` (+ new `update_pause`, `update_inventory_ui`, `update_craft_menu`, `use_equipped_item`, `use_bag_slot`, `equip_bag_slot`, `try_drop_slot`, `save_game_to_disk`, `load_game_from_disk`, `interact_hint`). Uninitialised members now set in the constructor (fps, craft/controls flags, molotov timers, unlock toast, map_prop_count, …). Square no longer also crafts. |

Last updated: 2026-10-17 (dmg_hint + no invented consumable restores)

| 2026-10-17 | **Fidelity purge:** craft → only `Craft_1/4/5/fishing_rod` (+ fence 31/32); removed invented Cook_*/Craft_rags_*/jam/break; wetness meter forced 0 (no Wet strings in export). Prior: `dmg_hint`; no invent consumable restores/infection-on-hit. `mdaz_tests` ALL PASSED |

Last updated (prior): 2026-10-17 (PSP pad map + GU fb + FPS HUD; EBOOT still unverified)

| 2026-10-17 | **PSP:** `platform_psp.cpp` full `InputState` mapping, soft 565 framebuf + `sceDisplaySetFrameBuf`, ms0 file I/O; Makefile lists all core objs; **FPS** counter in HUD (`Game::fps`). EBOOT **not** built here (no pspdev). |

Last updated (prior): 2026-10-17 (POI spacing, menu camera_roll_menu, crop grow 20s)  

| 2026-10-17 | **Map/menu/farming decode:** verified create_map_location **±300** pair spacing; **camera_roll_menu** pan (`menu_camera.cpp`, distance **100**); crop **grow** timer **20s** from item_activate; Buildings_base prefab XY **blocked**. `test_map_fidelity` + `mdaz_tests` |

Last updated: 2026-10-17 (audio tags, bot firearm bullets, Show_unlock_wpn toast)  

| 2026-10-17 | **Audio:** `audio_sdl.cpp` C2 media tags (weapon leftovers, zed attack_9, weather/UI consumables, bot SKS/repeater, unlock SFX). **Bots:** bandit827 IV11 fire SFX + `m4_bullet_bot` sprite. **Show_unlock_wpn:** toast + `md_achievment unlocked.ogg`. `mdaz_tests` |

Last updated: 2026-10-17 (inventory bag GUI + Pick_player_item + quest l_eng_ui strings)  

| 2026-10-17 | **Inventory UI:** C2 `gui_inventory` panel, worn portrait icons, clothing IV93 4-slot rows, bag grid; desktop click/E `pick_player_item`. Quest HUD uses `l_eng_ui.xml` ids 371–377. `mdaz_tests` |
| 2026-10-17 | **Visual:** worn helmet/vest/jacket overlays (`player_worn_visual`, `draw_player_worn_layers`); minimap tiled `minimap_tile`, road sectors, POI marks; no `bitmap_font` PNG in mirror → 5×7 fallback. `test_player_visual` + `mdaz_tests` |

Last updated (prior): 2026-10-17 (create_map_element IV10 + map_element props; decode batch)  

| 2026-10-17 | **create_map_element:** zed_resp tile→IV10 header; `map_element-sheet0` at POI stamp tile-0 props. **Mag capacities**, **bullet hit-zone mul**, **boat fuel / truck respawn / car fuel tick** decode → blocked docs. `test_map_fidelity` + `mdaz_tests` |
Baseline: Mini DAYZ **1.4.1** (`original_docs/`)  
Executable game: **C++ only** (`minidayz_desktop.exe`) — does not run `c2runtime.js`.

| Subsystem | Status | Notes |
|-----------|--------|-------|
| Source / architecture | Протестирована | Construct 2 export mapped; see `PORTING_PLAN.md` |
| Balance globals | **Сверена с оригиналом** | `balance_globals.hpp` |
| Survival tick (States) | **Сверена с оригиналом** | Every 5s / bleed 1s / cold on difficulty |
| Item catalog (200) | Протестирована | JSON + icons for all 200 IDs |
| Item consumable numbers | **Сверена с оригиналом** (partial) | `item_activate_iv_sverka.json` wired; unknown food/drink refuse use (no invent); morphine **45 blocked** |
| dmg_hint floating text | **Сверена с оригиналом** (partial) | Spawn/drift/color from decode; crit when IV8 second-hit exceeds roll; font size 18 not distinct on 5×7 |
| Item ground/hotbar icons | Реализуется | `item_sprites.cpp` full ID map |
| Map fog / visited | Реализуется | 16×16 sector visited + minimap fog |
| Weapon defs (catalog) | Реализуется | IDs 151–187 + melee; most dmg **placeholder** |
| Weapon base damage | Исследована | Fist **10**/45; **only FNX (151) IV0=23** sverka (`weapon_bullet_iv0_sverka.json`); other guns **kGunDmgPlaceholder** |
| Bullet hit variance | **Сверена с оригиналом** | ±25% |
| Fist melee range | **Сверена с оригиналом** | 45 |
| Combat zed/wolf dmg | **Сверена с оригиналом** | Regular 9/6/13/10; wolf **→player** uses **Zed_dmg** (Wolf_dmg_deal); wolf range **50**, cd **3s**, attack SFX trio |
| NPC_get_damage | **Сверена с оригиналом** (routing) | Kind 0–11 → IV slot; bullets use `zed_apply_bullet_hit_damage` + IV8 on army/tank |
| Player bullet armor | **Сверена с оригиналом** (partial) | Bullet skips mul; **IV6** wear + broken armor; melee **def_percent** + IV6 wear |
| Player_get_hit bleed | **Сверена с оригиналом** | ≈50%; Bleed 60s |
| Dispersion | **Сверена с оригиналом** | Persistent state |
| Timers loot/zed/airdrop | **Сверена с оригиналом** (constants) | |
| Day/night + rain | **Сверена с оригиналом** (partial) | Timer_ALL +1 / 1.5s, wrap 1440, Day_mode hrs 6–23, Rain_speed toggle |
| Loot piles | Реализуется | Typed POI (incl. fire/church) + weighted Spawn_drop + airdrop |
| Zombie AI types | Реализуется | Buried/screamer/army/tank/… |
| Animals | Реализуется | Howl/stampede/crow/deer SFX |
| Bandits / bots | Реализуется | Melee+gun; **`bot_reloading`** IV9/IV14/IV10; **`spawn_bandit_camp`**; **`Spawn_friend_bot`** ally; **`Spawn_team_camp`** / **Clear_team_camps** |
| Vehicles | Реализуется | UAZ/Volga/Truck + boat/bike; C2 fuel/respawn/crash; **GUI_draw_vehicle** HUD (bg/fuel/hp bars, controls); mount/drive |
| Quests | **Частично сверено** | Tasks 1–3 + `Q_reward_id` roll + `Quest_spawn_reward_box` ground drop + `Pin` minimap (task 2) + `Quest_canceled`/`Karma` (`karma_points`) |
| Map generation | Реализуется | **Reload_locations** streaming; dirt stamp on load; POI counts; river band; `generate_array_roads_1/_2` sector grid → tile 15 corridors; `poi_layout.cpp` |
| Bunker loader | Протестирована | RLE 225×74; tile atlas draw |
| Craft | **Частично сверено** | C2 names only: Craft_1/4/5, Craft_fishing_rod, **Craft_31/32** fence placement; removed guessed raincoat/bandolier/bandana/burlap/pile-firekit recipes (not in `data.js`) |
| Item use (drink/heal) | **Частично сверено** | `item_activate` decode: Whiskey IV14+20/IV16+40; Energy drink IV14+20 + 10s speed boost; Heatpack manual +25; Flare IV26 torch |
| Weapon unlocks | **Частично сверено** | Kill progress + `weapon_unlock_allows_loot_item` on **military/airdrop** spawn when unlock flags set |
| Save | Протестирована | MDAZ v16 (+weapon unlock progress); v15 worn IV6; v14 loot piles |
| Audio | Реализуется | C2 media tags (fn_fal_single_*, m4_single_*, attack_8–13 zed hits) + procedural fallback |
| Sprites / VFX | Реализуется | C2 sheet crops; muzzle/shell/blood/fish/grenade |
| HUD / menu | Реализуется | GUI crops; bag icons; night_overlay; amenu_bg |
| Campfire | Реализуется | Place/warmth/cook |
| Desktop SDL host | Протестирована | SDL2 2.30.9; 480×272 |
| Platform / PSP | Протестирована | GU renderer, ground cache, pak v2, half-res toggle, µs profiling + MS overlay; EBOOT built with WSL pspdev; real PSP: boots, stable 60 FPS (user report 2026-10-19 20:10) |
| Full parity | Реализуется | Mechanics first; presentation improving |

## How to run (desktop)

```
cd cpp/build
cmake -S .. -B . -G "MinGW Makefiles" ^
  -DCMAKE_PREFIX_PATH=../third_party/SDL2-2.30.9/x86_64-w64-mingw32
mingw32-make -j4
.\minidayz_desktop.exe
```

Controls: WASD move, Shift run, Ctrl crouch, Z/Space attack, E loot/chop/search/fish, G drop, V mag reload, C use (traps/kits/meds), R craft, T torch/fire, F throw, 1–8 hotbar, P pause, Esc menu/quit.

## Tests

```
mdaz_tests.exe → ALL TESTS PASSED
```

Includes survival sverka, fist dmg 10 / range 45, bullet ±25%, craft, bunker load, timers.

## Iteration log

| Date | Work |
|------|------|
| 2026-10-17 | **dmg_hint** port + strip invented consumable restores; tests; docs |
| 2026-10-17 | **Weapon IV0 path scan** (`weapon_iv0_path_scan.json`): Bullets_parameters / tag-window / New_bullet branches — **no new gun sverka** (FNX 151 IV0=23 unchanged). **Medical item_activate**: blood/saline/vitamins/tetracycline timers; **morphine (45) blocked**; Global_respawn **bunker** loot; `Check_loot_decoded.txt`; `mdaz_tests` ALL PASSED |
| 2026-10-17 | **`bot_reloading`** IV14 mag table + reload wait in bandit AI; **`Hit_effect`** screen flash; **`dmg_hint`** / bullet IV2 decode docs; bullet IV2 travel decay; KNOWN_ISSUES quest/rewards text; `mdaz_tests` ALL PASSED |
| 2026-10-17 | Craft audit vs `data.js`: removed 9 unverified inventory recipes; **item_activate** sverka Whiskey/Energy/Heatpack/Flare; stripped rain/barbed slow, auto heatpack, crow scavenging; `consumable_item_activate_sverka.json`; `mdaz_tests` ALL PASSED |
| 2026-10-17 | **char_anims** walk/run/attack fallbacks + reload `using_item`; **fence_horizontal/vertical** + **eff_fence_piece** (Craft_31/32 kinds 2/5/6/7); **walk_marker** quest Pin + **quest_case** reward pile; dog **dog_skin**; bandit **pistol_** fire pose; s_weapons PNG absent → hands overlay only |
| 2026-10-17 | **Items_update** capacity decode: Array **536** col **301** + IV10 (UI `%`); **IV181** clarified (not IV); `Check_space_*` slot logic; `Cut_arrays` / `init_item_activation` / `Pick_player_item` docs; tools `extract_item_array_cols.js`, `extract_fn_onfunction.js`; col **301** values **blocked** (table not in export); `mdaz_tests` ALL PASSED |
| 2026-10-17 | **Check_ammo_*** / **Check_space_*** clothing cascade (IV19→17→11→9, IV93 ×4 slots); `clothing_containers.cpp`, `consume_ammo_for_reload`, `pick_inventory_item` overflow; decode `clothing_containers_decode.md`; MDAZ **v17**; `mdaz_tests` ALL PASSED |
| 2026-10-17 | **Quest_spawn_reward_box** + `Q_reward_id` pools + Pin minimap + Karma cancel; **Craft_31/32** fence (5×38, +50); military/airdrop unlock loot filter; `quest_reward_table.json`; tests |
| 2026-10-17 | **Bleeding_guide** / **Inventory_guide** / **ach_move_text** audit anchors in `hud.cpp`; **Spawn parameters** group label in `spawn_params.cpp`; **Movement_stick**/**Movement_tap** + **tutorBleedingStopped** / **tutorMoveChoosen** / **tutorMovementType** in `tutorial.cpp` + `C2SessionGlobals` (desktop skips full tutorial overlay; WASD = STICK). `docs_gen/functions/*_decoded.txt`. `test_tutorial.cpp`. audit **175/175** (100%) |
| 2026-10-17 | **spawn1/2/3** + **zed_resp** → `spawn_params.cpp`; **Spawn_friend_bot** / **Spawn_team_camp** / **Clear_team_camps**; **TD_drop_weapons**; **item_for_remove_id** / **Recheck_inventory_cards** / **loot_checker2** gate; **Vehicle Layering** + **GUI_erase_vehicle** hooks. `test_spawn_params.cpp`. `mdaz_tests` ALL PASSED; audit **166/175** (95%) |
| 2026-10-17 | **Verified** fn_fal_single_1/2 (FN-CAL 175), zed attack_8/0–13 SFX, s_weapons layer anchors, Damage dealing / Deaths decode; `weapon_fire_sfx_tag`; docs_gen `*_decoded.txt` |
| 2026-10-17 | `C2SessionGlobals` wired: attachments (`Silencer_name_id`/`Scope_name_id`/`Belt_name_id`), `Melee_mode`, water/pickpile, craft removal, mapgen hotspots/`anotherX`/`Loop_secrets*`, `rad_dist`, minimap/menu layout approx (`char_select_gap`/`btn_pos_y_margin`); tests `test_c2_session_globals.cpp` |
| 2026-10-17 | `Weapon_unlocks` progress + v16 save; `molotov_reload` 15s throw gate; `Spawn_drop_from_player` on G drop; docs_gen `molotov_reload_body.json` / `Spawn_drop_from_player_body.json` / `Craft arrows_body.json` |
| 2026-10-16 | Initial C++ scaffold + docs |
| 2026-10-16 | ACE-decoded `States`/`Update_Stats`; rewrote survival; loot; mapgen; zed types |
| 2026-10-16 | Bullet variance + Fist range 45; SDL desktop host |
| 2026-10-16 | Full play loop: camera render, hotbar, melee/ranged, bullets, bleed, POI loot, bunker enter, craft, army shooters, screamer alert, difficulty select |
| 2026-10-16 | F5/F9 save-load; clothing insulation + bunker warmth stubs; infection chance; FNX IV0=23 / Fist 10 confirmed via extract |
| 2026-10-16 | POI collision; animal meat drops; cook craft; torch warmth (T); PSP Makefile refreshed; menu continue save |
| 2026-10-16 | BanditSystem; minimap; torch glow; cook recipes; updated BUILDING controls |
| 2026-10-16 | VehicleSystem drive stub; QuestState HUD objective |
| 2026-10-16 | Tab inventory overlay; Novice metabolism perk; day rollover; bag/car interact fix |
| 2026-10-16 | Clothing wear/insulation; IAudio hooks; test_systems; night vignette; difficulty perks |
| 2026-10-16 | Fishing stub (rod + E); cook perch; continuous play-loop polish |
| 2026-10-16 | SDL procedural audio; CampfireSystem; cook-near-fire; raw infection; tank spawn |
| 2026-10-16 | Quest chain; bush forage; save v2 (worn/equip/infection); tank melee polish |
| 2026-10-16 | Airdrop alert/crate/minimap; Spawn_drop loot IDs; ammo HUD; car fuel/crash; gasoline refuel |
| 2026-10-16 | Car toolbox repair (id 59); dry-fire click |
| 2026-10-16 | Medical polish (19/28/45/29); hotbar Q/X/wheel; animal respawn packs; minimap sector fog |
| 2026-10-16 | Save v3 visited fog; G drop; V mag-reload stub |
| 2026-10-16 | NVG/flare use; military barbed slow; food restore polish |
| 2026-10-16 | Screamer audio/HUD; buried ambush sprint; woodpile→sticks craft; NVG in save |
| 2026-10-16 | POI loot tables; F throw sticks |
| 2026-10-16 | Boat spawn; edge water wade; BUILDING controls refresh |
| 2026-10-16 | Bunker loot+zeds; zed AI in bunker; POI dens |
| 2026-10-16 | Bunker wall collision + walkable spawn; bike vehicle |
| 2026-10-16 | **Fix bunker RLE load** (data string parse); wall collision tests |
| 2026-10-16 | F1 grenade throw; bunker loot/zed draw+walls; partial pickup; WET HUD; aim line |
| 2026-10-16 | Grenade AoE; torch battery drain; T-shirt→rags craft |
| 2026-10-16 | Raincoat wet resist; sector [x,y] HUD; bunker dens visible |
| 2026-10-16 | NVG battery share; tent/molotov/gasmask; crow flocks; INV FULL; molotov craft |
| 2026-10-16 | Perks HUD (HNT/FRM); death log `last_death.txt`; hunter meat x2 |
| 2026-10-16 | Save v4; bandit loot; melee break; POI labels; night stars; rain drink |
| 2026-10-16 | Mag capacity+V reload; helmet/vest armor; morphine adrenaline; scream→wolves |
| 2026-10-16 | Military loot+armor; more craft; limp/headlamp; rain drink; POI compass |
| 2026-10-16 | Craft sverka: bow=ash+rope, arrows=sticks+knife, bag=burlap+rope |
| 2026-10-16 | Axe harvest wood; backpack capacity; energy drink; military armor loot |
| 2026-10-16 | Canned+knife; sprint zed aggro; duct tape melee; minimap POI colors |
| 2026-10-16 | POI interior search+warmth; INSIDE HUD; duct tape starter |
| 2026-10-16 | Car headlights; boat paddle from sticks; craft recipe sverka notes |
| 2026-10-16 | Bandit flee low HP; wolf pack howl; BUILDING controls refresh |
| 2026-10-16 | Wetness meter; matches+sticks fire; V RELOAD hint; deer stampede |
| 2026-10-16 | Zed/loot respawn prefer POI tables; typed military loot refresh |
| 2026-10-16 | Quest chain hospital→bunker→bandits→day2; quieter walk |
| 2026-10-16 | Save v5 mag/wetness; unvisited sector fog on world view |
| 2026-10-16 | Hand-drill fire (2 sticks, 25%); continuous play-loop polish |
| 2026-10-16 | Low-blood vignette; STARVING/THIRSTY/FREEZING HUD |
| 2026-10-16 | Gun jam when wet/low mag; rest-at-fire blood regen |
| 2026-10-16 | Jam clear with V + JAMMED HUD |
| 2026-10-16 | Auto heatpack when freezing |
| 2026-10-16 | Shooter aim spread; raincoat/burlap in civ loot |
| 2026-10-16 | Night sleep at fire advances dawn + day count |
| 2026-10-16 | Buried UNEASY HUD; SLEEP at night fire hint |
| 2026-10-16 | Sleep refreshes is_night; hospital rare armor |
| 2026-10-16 | Edge water tint; fishing requires water band |
| 2026-10-16 | Boat spawns on west water edge |
| 2026-10-16 | Fishing rod in starter kit |
| 2026-10-16 | Whiskey warmth+courage; veg food; sewing/cleaning kits |
| 2026-10-16 | Richer airdrop (med/armor/gun); POI bearings N/E/S/W |
| 2026-10-16 | Improvised splint craft (sticks+rags) clears limp |
| 2026-10-16 | Bear trap + barbed wire placeable; howl/airdrop audio |
| 2026-10-16 | Gasmask filter drain; crow scavengers on zed kills; dirt roads |
| 2026-10-16 | Fish quest; berry restores; secret POI loot; torch cone; cough |
| 2026-10-16 | Firestation axe loot; night flare longer; deer meat x2 |
| 2026-10-16 | Craft rope from 4 rags; secret on minimap |
| 2026-10-16 | Buried dirt mound; save v6 gasmask filter + fish |
| 2026-10-16 | Forest trees near POIs; screamer pulse ring |
| 2026-10-16 | Axe E fells nearby trees for wood |
| 2026-10-16 | Soft tree collision while walking |
| 2026-10-16 | Bike spawns near village POI |
| 2026-10-16 | Night denser wolf packs |
| 2026-10-16 | Bleed drips; bandits prefer military POIs |
| 2026-10-16 | Cooked meat/fish restore values |
| 2026-10-16 | Mil tank + secret screamer respawn; secret loot refresh |
| 2026-10-16 | HUD tips: splint craft + chop tree |
| 2026-10-16 | Headlamp torch drain reduced; rope craft test |
| 2026-10-16 | Dawn/dusk fog; rain hurts fishing |
| 2026-10-16 | Ctrl crouch: slower + quieter vs zeds |
| 2026-10-16 | Radio shows airdrop ETA (battery) |
| 2026-10-16 | RDS scope (76) tightens gun dispersion |
| 2026-10-16 | Farmer perk unlocks after 8 bush forages |
| 2026-10-16 | Low fuel HUD + warn beep while driving |
| 2026-10-16 | Vehicle damaged HUD when HP low |
| 2026-10-16 | Near-vehicle mount/refuel HUD tip |
| 2026-10-16 | Gas stations denser gasoline/toolbox loot |
| 2026-10-16 | Hospital denser meds + guaranteed bandage |
| 2026-10-16 | Proced ambient SFX when storming |
| 2026-10-16 | Woodpile->4 sticks; woodpile in starter |
| 2026-10-16 | HNT/FRM perk progress HUD |
| 2026-10-16 | Stronger bunker warmth/dry; HNT/FRM progress |
| 2026-10-16 | Clothing insulation by warm pieces; soaked chill |
| 2026-10-16 | Cook ruffe->big fillet; fishing ruffe chance |
| 2026-10-16 | Plant seeds E (tomato/pepper/zucchini); FRM progress |
| 2026-10-16 | Hospital tetracycline; village seed loot |
| 2026-10-16 | Bleeding increases zed aggro (blood scent) |
| 2026-10-16 | Infection fever DoT; wolves smell blood; mushrooms risky forage |
| 2026-10-16 | Tent warmth/dry alone; bandit bayonet+aim; crouch harder to hit |
| 2026-10-16 | Helicrash loot table; wood fence blocks zeds (C+pile+sticks) |
| 2026-10-16 | Storm thunder+lightning flash; startles zeds |
| 2026-10-16 | Loc_Police POI + loot; berry restores bumped |
| 2026-10-16 | Crouch braces aim; big fillet/ruffe food stats |
| 2026-10-16 | LR scope 98; pan prioritizes cook at fire; CROUCH AIM |
| 2026-10-16 | Quest VisitPolice; pan in civ loot; pan cook HUD |
| 2026-10-16 | Matches fail in heavy rain outdoors (kit/tent OK) |
| 2026-10-16 | Heavy rain extinguishes open campfires (tent shelters) |
| 2026-10-16 | Vehicle headlight beam cone + HUD |
| 2026-10-16 | Police zed/bandit spawn bias; KNOWN_ISSUES sync |
| 2026-10-16 | Ashwood craft from sticks; cough pulls zeds |
| 2026-10-16 | Cough alert window 2.5s for zed pull |
| 2026-10-16 | Gasoline C boosts fire or makes fuel puddle |
| 2026-10-16 | AUG/Saiga/MAC-10/Mare Leg weapon defs + loot |
| 2026-10-16 | Dry-fire click SFX + EMPTY HUD |
| 2026-10-16 | Drive into trees: vehicle dmg + fell wood |
| 2026-10-16 | Vehicle runover damages zeds |
| 2026-10-16 | Vehicle runover zeds/bandits/wolves |
| 2026-10-16 | Nota/Nuko/Energy drink restores |
| 2026-10-16 | Boat stays in map-edge water band |
| 2026-10-16 | BOAT BEACHED HUD when inland |
| 2026-10-16 | Starving/thirsty blocks sprint |
| 2026-10-16 | Bag in inventory expands capacity (pack 18) |
| 2026-10-16 | Buried zeds near villages/cities; pack cap 18 |
| 2026-10-16 | Buried zeds chance at village/city POIs |
| 2026-10-16 | Buried ambush pop SFX; village buried spawn |
| 2026-10-16 | perk_snowborn after 120s freezing; save bit |
| 2026-10-16 | perk_camel unlock after 100s thirsty |
| 2026-10-16 | hamster/regen unlock via starve/wound endure |
| 2026-10-16 | runner/metabolism unlock via sprint/bleed endure |
| 2026-10-16 | Perk HUD cleanup; endure unlocks for all core perks |
| 2026-10-16 | Claymore placeable proximity mine (C) |
| 2026-10-16 | Night wind ambient SFX |
| 2026-10-16 | Flare lures zeds to beacon |
| 2026-10-16 | Axe can break on tree chop (duct tape helps) |
| 2026-10-16 | Fishing bait (berry/apple/meat) + spinning rod bonus |
| 2026-10-16 | Silencer quiets gunshot + reduces zed hear pull |
| 2026-10-16 | Landmine placeable (pressure+AoE fire) |
| 2026-10-16 | PU/PSO/ACOG/LR scopes graded aim |
| 2026-10-16 | Cleaning kit maintains gun + dries |
| 2026-10-16 | Bandolier: +2 slots + double ammo stacks |
| 2026-10-16 | Papers+sticks->matches; piles+papers->kit |
| 2026-10-16 | Loc_Church POI + med/whiskey/papers loot |
| 2026-10-16 | Shovel E dig: uncover buried / forage dirt |
| 2026-10-16 | Compass includes Church + Firestation |
| 2026-10-16 | Crowbar pry boosts indoor loot |
| 2026-10-16 | Hacksaw cuts barbed wire / wood fence |
| 2026-10-17 | Original C2 PNG sprites via stb_image (player/zeds/animals/buildings/vehicles/trees) |
| 2026-10-17 | Night eyes/shadows, rain sprites, reload spinner, wetness/gasmask HUD, loot icons |
| 2026-10-17 | ground_lvl1–4 biomes; bunker tile index; POI interiors; 200/200 item icons |
| 2026-10-17 | Weapon-specific OGG shots; footsteps; eat/drink; bag icons; shell/blood/fish FX |
| 2026-10-17 | Worn gear → player_skin_* swap; night_overlay; quest panel; grenade/molotov sprites |
| 2026-10-17 | Docs refresh (PLAN/STATUS/KNOWN_ISSUES); gun IV0 deep extract tool; weapons 171–187 defs |
| 2026-10-17 | `mdaz_tests` + desktop rebuild verified this session |
| 2026-10-17 | Lean difficulty-scaled starter kit (removed debug mega-kit) |
| 2026-10-17 | PSP Makefile includes `item_sprites.o`; pspdev still absent on this PC |
| 2026-10-17 | Spawn_drop weighted table wired; firestation/church typed loot |
| 2026-10-17 | Zed_per_base density at POIs + respawn cap 4×; craft rod/rope/fence/burlap |
| 2026-10-17 | `mdaz_tests` ALL PASSED (spawn_drop + fire/church + ashwood+rope rod) |
| 2026-10-17 | Fix drink ID map (Kvas/waterbottle/canteen); civ+gas loot refresh |
| 2026-10-17 | Fishing catch mix herring/salmon/ruffe; cook→fillet recipes |
| 2026-10-17 | Difficulty NPC dmg tables (Novice/Regular/Veteran) from C2 States |
| 2026-10-17 | Save/load restores NPC dmg table from player.difficulty |
| 2026-10-17 | Spinning-rod craft; tetra medical cat; raw herring/salmon infect |
| 2026-10-17 | Bunker dens: spawn_bunker + multi-zed dens; animal packs (deer/rabbit/dog) |
| 2026-10-17 | Bandit melee + shooter/bandit gun shots scale with difficulty |
| 2026-10-17 | Corpse life uses Bot_corpse_stay_time=90; denser POI trees; animal packs at start |
| 2026-10-17 | Quest chain +firestation/church; clothing insulation IDs; hand-drill weather; sewing limp |
| 2026-10-17 | Gunshot hear uses Player_Hear_radius; silencer caliber-matched; mil Gorka loot |
| 2026-10-17 | Delayed crop grow/harvest; typed POI interior search; radio ETA HUD |
| 2026-10-17 | Magpull +2/+10 mag; Choke tightens shotgun; mil loot attachments |
| 2026-10-17 | Protector Case +3 cap / jam resist; fever heat; airdrop cycle 360; aim settle |
| 2026-10-17 | Vehicle trunk stash; death corpse loot; bandana craft; storm zed aggro |
| 2026-10-17 | MDAZ v7–v10 persist (vehicles/crops/traps/limp/airdrop); shore drink (E) |
| 2026-10-17 | Craft heatpack/flare; radio nudges airdrop −45s; airdrop attachment loot |
| 2026-10-17 | Shore E DRINK tip; bear-trap infection; overencumber slow |
| 2026-10-17 | MDAZ v11 campfires+tent; typed zed corpse loot; craft baseball bat |
| 2026-10-17 | Wolves/dogs flee campfire near player; night zed aggro ×1.25 |
| 2026-10-17 | Tree stump regen; rain snuffs torch; shovel buried cache; MDAZ v12 adrenaline; night wolves |
| 2026-10-17 | Bunker dens: chance of Tank + extra mil loot when Zed_per_base≥5 |
| 2026-10-17 | Quest mil+heli; Craft_21/22/3; MDAZ v13 bunker_looted + melee mul |
| 2026-10-17 | Accuracy audit (`tools/audit_port.js`): removed 15 mechanics not found in data.js (see KNOWN_ISSUES "Removed") |
| 2026-10-17 | Visual pass: Map-layout tilemaps decoded from data.js; original Menu layout scene + wooden buttons; 5×7 font; removed debug enemy spawn at player; no hostile POIs within 600 px of spawn; buildings at native size; dead screen at native aspect; `--shots` screenshot mode. Tests re-run: ALL TESTS PASSED |
| 2026-10-17 | Map stamps: runtime tile 15 dirt pads/roads + heli water 4; mapgen counts from balance_globals; minimap 3x3 fog; Mag_reload on R/V (craft on B); bunker zeds every enter; Craft_1/4/5 renamed; nearest-neighbor sprites. Tests: ALL PASSED |
| 2026-10-17 | Molotov_damage ticks + flame puddle; bow V-nock (Bow_reload 2s); weather Timer_ALL/1440 + Rain_speed toggle; death log fields. `mdaz_tests` ALL PASSED; audit **74/175** functions (42%) |
| 2026-10-17 | River tile-4 band from `river_R`; C2 quest task1 (`Q_task_id`/`Q_target_sid`); removed invented quest chain. `mdaz_tests` ALL PASSED |
| 2026-10-17 | Character animations: load C2 frame tables (char_anims.txt); player idle/run/axe/pistol/twohand/using_item; zeds walk/run/attack; animals/bandits run/idle. Tests: ALL PASSED |
| 2026-10-17 | **NPC_get_damage** decode → `npc_apply_damage` + kind map; combat paths use `hurt_zombie/animal/bandit`; zed HP bar `min(30,…)`; corpse `die1`/`die2` (char_anims + extract pass). `mdaz_tests` ALL PASSED; audit **76/175** C2 functions (43%) |
| 2026-10-17 | Vehicles: UAZ/Volga/Truck spawn + C2 fuel (−8/0.1s), refuel +40, crash 15/30, 601s respawn; audit **89/175** functions (51%). `mdaz_tests` ALL PASSED |
| 2026-10-17 | Wolf_dmg_deal decode (Zed_dmg, range 50, cd 3s, attack SFX); Check_space_inventory `can_add_item`; MDAZ v14 Save_loot/Load_loot subset + loot_respawn timer. `mdaz_tests` ALL PASSED |
| 2026-10-17 | Bullets_dmg_deal: bullet armor path + IV8 army zed double-hit; death_reason only when caller sets (`Bullet`/`Zombie`/`Melee`/`Fire`). `mdaz_tests` ALL PASSED |
| 2026-10-17 | Clothing **IV6** wear (bullet/melee); melee **def_percent**; Death_guide tutor sprites; MDAZ **v15** worn IV6. `mdaz_tests` ALL PASSED; audit **99/175** (57%) |
| 2026-10-17 | **GUI_draw_vehicle** / erase-on-dismount: `gui_car_bg`, vertical fuel/hp bars (IV284/294), car control sprites from GUI_layout. **generate_array_roads_1/_2** decoded → sector grid 12/13/14 + dirt tile 15 stamps (replaces POI-pair heuristic). `mdaz_tests` ALL PASSED; audit **99/175** (57%) |
| 2026-10-17 | **Call_sub_menu_craft** / **Check_craft** / **craft_ok** audit strings; B/R craft recipe HUD (`sub_menu*` sprites, `craft.cpp` recipes). Gun IDs 181–187 IV0 still absent in data.js — damages unchanged. `mdaz_tests` ALL PASSED; audit **102/175** (58%) |
| 2026-10-17 | Session: NPC_get_damage routing; death die1/2; UAZ/Volga/Truck fuel; wolf attack; loot v14/v15; Reload_locations; quests 1-3; clothing IV6; melee def_percent; Death_guide tips; vehicle HUD; generate_array_roads; craft submenu. Audit **102/175 (58%)**. Tests ALL PASSED |
| 2026-10-17 | Decoded **Craft_21–22** (fence tab pickpile/Crafted) and **Craft_31–33** (buildings tab fence build/pickpile); no new recipes (no verified Spawn_drop). `craft_decode_Craft_21_33.json` + audit anchors in `craft.cpp`. |
| 2026-10-17 | Weather sverka: C2 Every **1.5s** → `Timer_ALL` +1, wrap **1440**; `Day_mode` when **6≤Timer_Hours≤23**; `Rain_speed` toggles `Rain`; survival cold uses `Day_mode`/`Rain`. `test_weather.cpp`. **Casual_Respawn_player** / **Global_respawn** not wired (SEED_playcasual=0; Map/TD spawn only). Tests ALL PASSED |
| 2026-10-17 | **Spawn_bandit_camp** / **Spawn_firearm_bot** / **Spawn_puncher_bot** decode → `spawn_bandit_camp`; C2 audit strings **AltMove**/**Pin**/**bot_reloading**; vehicle `kDriveSpeedApprox` (AltMove JS absent). `mdaz_tests` ALL PASSED; audit **106/175** (61%) |
| 2026-10-17 | Inventory: **Pick_inventory_item** / **Check_item_inventory** / **close_inventory** / **Inventory_opened**; **Zed_check_bunker**; **generate_bunker_entrance** audit; weather group **Time cyles and weather**. `mdaz_tests` ALL PASSED; audit **115/175** (66%) |
| 2026-10-17 | **attack_up/down/left/right** audit + player **axe/pistol/twohand** fire pose; wolf **attack_*** anim; **LOOT_respawn** 3/4/5s per pile; **helispawned**≤3 / **hammerspawned**≤5; **Remove_bandit_bots** on quest-2 complete. `mdaz_tests` ALL PASSED; audit **125/175** (71%) |
| 2026-10-17 | **Bunker_erase** / **Bunker_generate** (`bunker.cpp`); **Global_respawn** + **Casual_Respawn_player** (`spawn_global.cpp`, SEED_playcasual=0 → dead screen unchanged); **Remove_bot_*** bot cleanup; **Weapon_unlocks** + `*_unlock_progress` audit strings; **LOOT Respawn** group string in `timers.hpp`. `test_spawn_global.cpp`. Tests ALL PASSED |
| 2026-10-17 | Firearm IV0 rescan (`tools/_scan_bullet_spawn_iv0.js`): **151 FNX=23** only sverka; others `kGunDmgPlaceholder`. Mapgen sector **6/9/11/15** for village/city/military/POI. No **Check_reload** in 1.4.1 export; **Reloading_time** 0/1 unchanged. Build OK; `mdaz_tests` (2 pre-existing inventory/save fails). |
| 2026-10-17 | Map fidelity: **generate_location** 72× forest SetTile → `stamp_location_forest` + pine scatter; **21** bunker_zed_spawner XY (**approx** `%` layout); **Bot_bandits_check_situation** + **Spawn_bandit_camp** IV0 `{9,11,17,14,21}`. `test_map_fidelity.cpp`. `generate_mapgen_toelements`/`create_map_element` **blocked** (no `data.js`). `mdaz_tests` ALL PASSED |
| 2026-10-17 | **Death_guide** decoded (tutorial-only; UI **#542** on dead screen). **Save_loot/Load_loot** CSV → `save_loot_csv_decode.txt`; MDAZ **v18** loot **96** piles. **generate_mapgen_toelements** verified (obj507→`generate_location`); **create_map_element** partial (`mapgen_props_decode.txt`). `tools/decode_death_loot_mapgen.js`. `mdaz_tests` ALL PASSED |
| 2026-10-17 | **item_activate** consumable sverka → `item_activate_iv_sverka.json` + `item_db.cpp` known_stats (88 handler ids). Starter seed **blocked** (`starter_inventory_decode.json`). Zed/animal max HP/speed **blocked** (no spawn IV literals). Craft audit **11/11 (100%)**. `mdaz_tests` ALL PASSED |
| 2026-10-19 20:50 | **Fidelity #1 building walls — Протестирована (unit-тесты); в игре на экране не проверена**: 35 Solid `*_int` типов (Buildings_base) → `src/world/building_solid.inc` (генерируется `gen_building_solid.py` из data.js: кадр, hotspot, полигон коллизии, warmzone w1..w2, парный внешний кадр). Старые выдуманные 3x3 блок-ячейки (`apply_collision_for`, ±150px пары) удалены; стены = рёбра полигона (`Game::building_blocks`, шаг пересекает ребро / ближе 1px = player_base 2x2), скольжение по осям; дверные проёмы свободны; внутри = warmzone-прямоугольник; спрайты рисуются по hotspot (как в C2, exterior в точке 0 interior). heli_crash без Solid. Тест `test_fidelity2.cpp` (10 проверок: 46-точечный полигон police, стена блокирует, дверь проходима, пол свободен, warmzone, hospital 512x404). mdaz_tests ALL PASSED на desktop. Не сделано: зомби/мобы пока ходят сквозь стены (учитывает ли их движение Solid в C2 — Не исследована), PSP не пересобиран. Backup: pre_b1_backup.tar |
| 2026-10-19 21:05 | **Fidelity #2 урон/магазины — Протестирована (unit)**: урон = IV0 дефолтного инстанса типа пули (data.js layout), формула Bullets_dmg_deal round(IV0±IV0/4) уже была; события-переопределения FNX/PM/PB=23, Colt=25. Оружие→пуля по fire-событиям (player_collision_base IV1/IV43 + имя предмета IV10). Магазин = литерал `IV3 < N` в Reload_*, время перезарядки = параметр Mag_reload без fast-mag (IV18). Дробовики: 5 shotgun_pellet × 20, у каждой свой разброс. Скорость пули = Bullet speed. Плейсхолдер 25 удалён; все 37 огнестрельных строк привязаны к типу пули (тест). Изменения: Bizon 64→40, IZH/Saiga/Remington — дробь, FNX reload 1→2 с (старый тест обновлён). |
| 2026-10-19 21:05 | **Fidelity #3 HP/скорость зедов и животных — Протестирована (unit)**: HP = IV0 скина при спавне (normal 60, fast 80, army 100, shooter 100/125/175 по скину, buried 60, screamer 50, tank 1000), скорость = speed Bullet-поведения "attack" при спавне (normal/army 90–100, fast 120–140, buried 90–110, shooter 20–30, screamer 30–40, tank 110/рывок 200). Животные: wolf 50 HP/90–110, dog 250/140, deer 80/110–130, rabbit 5/180–200. Удалены выдуманные множители (shooter ×0.35, buried ×2.4, tank ×0.85). |
| 2026-10-19 21:05 | **Fidelity #4 слоты инвентаря — Протестирована (unit)**: вместимость = 1 базовый слот (C2 массив inventory t381, 1x1x1) + размеры контейнеров надетых вещей (a198 при надевании: штаны jeans 2 / worker 3 / tracksuit 1 / gorka 3 / orel 2 / paramedic 2 / hunter 3; верх T-shirt 0 … gorka/hunter 3; жилеты press 1 / assault 2 / high capacity 3 / soviet 2; рюкзаки taloon 3 / hunting 5 / mountain 7 / school 2 / impr. bag 3 / impr. backpack 5 / tent 0 / tortilla 6). Удалены выдуманные 12 базовых слотов, бонусы рюкзаков 15–22, bandolier +2, Protector Case +3. Надевание теперь убирает вещь из сумки, одна вещь на регион (замена). Старт как в C2 (Player_skin_type 1): надеты T-shirt + Jeans, в базовом слоте Hunter knife (=3 слота); выдуманный стартовый набор (еда, спички, палки, рюкзак/топор по сложности) и отладочный «craft kit» (≈30 предметов: NVG, F1, палатка, противогаз…) удалены. |
| 2026-10-19 21:05 | **Fidelity #6 деревья — Реализована, но не проверена**: сами спрайты деревьев не Solid, но create_map_element спавнит у Imagepoint 1 каждого дерева obst_base (t336, Solid), кадр 10 (15x17, треугольный полигон). Выдуманное радиальное выталкивание 14px заменено этим треугольником (игрок + NPC через building_blocks). Размещение деревьев пока остаётся выдуманным (разброс вокруг POI) — Исследована: в C2 деревья = map_element IV2 1..N (create_map_element), относится к пункту 5. Сборка: тесты ALL TESTS PASSED, EBOOT 21:04 (1526120 байт), загрузка в PPSSPP OK (poi hash 7cbc5967). |
| 2026-10-19 21:20 | **Fidelity #4b подбор / снятие одежды — Протестирована (unit)**: C2 picky: снаряжение (id 250–499: одежда, жилеты, рюкзаки, головные уборы) надевается сразу с земли (Pick_player_item), старая вещь того же региона падает на землю (Drop_pants/_outerwear/_vest/_backpack); остальное — Check_space_*: в ОДИН пустой слот без слияния стаков. Предметы, не помещающиеся в новую вместимость, выпадают рядом (в C2 содержимое уходит вместе с вещью — массив привязан к ней). |
| 2026-10-19 21:20 | **Fidelity #5a интерьеры зданий — Протестирована (unit), частично**: из create_map_element (map_element IV1=7, 36 подтипов) сгенерирован src/world/building_interior.inc (gen: /workspace/mdz/item5/gen_interior.py): Solid-пропы obst_base/big_obstacle_base (кадр, точка привязки, полигон; без полигона = рамка кадра) и 82 точки spawn_loot_point с типом лута. Пропы блокируют игрока/NPC; кучи лута ставятся в точки spawn_loot_point (количество куч и таблицы предметов пока портовые — Trigger_spawn не портирован). Расстановка зданий/деревьев по программе create_map_location — Исследована (массив 518: колонки тип/подтип/X/Y, 101 тип mapgen), не реализована: требует порта generate_array_locations. |
| 2026-10-19 21:25 | **Подписи локаций — Сверена с оригиналом (удалены)**: в data.js нет строк/текстовых объектов с названиями локаций (нет "police"/"hospital" и т.п. кроме спрайтов и предметов); оригинал показывает локации только на миникарте. Удалены выдуманные мировая табличка над зданием и HUD-компас "POLICE E". Харнесс --shots-door (полицейский участок: упор в стену ~1.5 с, шаг к двери, вход, остановка внутри) — только вход с клавиатуры после одной тестовой расстановки. Тесты ALL TESTS PASSED, EBOOT 21:22 (1531224 байт). |
| 2026-10-19 21:35 | **Стены vs пули и зрение — Протестирована (unit)**: C2 "Hitting_obstacles": все пули (семейство t910 игрока/ботов, включая melee_swing, и zed_bullet t724) уничтожаются при столкновении со зданиями (семейство t927 = все 36 *_int), t202/t339/t755 и obst_base t336 (в т.ч. стволы деревьев). Все LineOfSight-поведения (зомби MainLook/ShortLook, screamer, волк/собака, боты Eyes/Gun) имеют препятствия t927 + t339. Порт: пули проверяют шаг prev->pos по стенам/пропам/деревьям; зомби, бандиты, волки/собаки не преследуют и не бьют без LOS; стрелки-зомби и бандиты не стреляют без LOS. Тесты: LOS через стену закрыт, через дверь открыт, пуля останавливается стеной, зомби снаружи не видит/не бьёт игрока внутри. |
| 2026-10-19 21:05 | **Враги и Solid — Протестирована (скриншот)**: у Bullet-поведений зедов/животных/бандитов bounce-off-solids = 1 → `npc_forest_block` теперь проверяет и полигоны зданий (скольжение по осям). Тестовый харнесс `--shots-building <dir>` (только desktop, не геймплей): игрок у полицейского участка упирается в южную стену, внутри — в западную; 4 BMP. EBOOT 20:54 (1527128 байт) собран, PPSSPP: загрузка, hash 7cbc5967, без падений (только boot-проверка, не FPS). mdaz_tests ALL PASSED. Бэкап pre_b2_backup.tar |

| Fidelity: noise (noice/warn_zed) | Реализована, но не проверена | ZombieSystem::noise(at, r): zeds within r+200 walk toward the point +-50 at 80-90 px/s for 2 s with no LOS. Wired: per-weapon gunshot radius (data.js Firearm shooting: 450/400/350/300/100, bow none) and F1 explosion 500. Unit test OK; not checked in play. |

| Fidelity: HUD hotbar vs capacity | Реализована, но не проверена | The original has slot cells (gui_item_backpack, IV12 = index) only for the capacity the worn containers give, and hides missing containers with gui_slot_hider_*. The port hotbar now draws min(capacity, 8) cells instead of a fixed 8. Screenshot run_c: 3 cells with 3 slots. |

| Fidelity: HUD portrait | Реализована, но не проверена | The original portrait is gui_btn_perks (29x32, per-skin anim, hotspot -4,-3) pinned with the gui_portrait_bg card, inside its dark window at card-local (5,3); the card is drawn below. The port drew a whole-body skin sprite that overlapped the bottom of the card. Now gui_btn_perks frame 0 (White) at 2/3 scale. Shots: portrait_before_after.png. |

| Fidelity: map generator (B1: grid + rows + trees) | Реализована, но не проверена | New src/world/c2_mapgen.cpp + generated generate_location.inc (codegen genloc/codegen.py from data.js generate_location, 4400 lines). Ported: generate_array_locations (per-level counts, 3x3 free block, codes 9/5/6/11/15), roads_1/_2 (first two POIs per row/column, 13/12/14), secret spot (2), generate_array_tomap (choose tables, gas 12->30/13->31, 32, 22->5), generate_location for all 256 cells (array-518 rows), helicrash/hammercrash (Humvee) sites, bunker entrance. In the game so far only category 1 is used: 2331 trees (6 tree sprites + tree_block) at their real positions with trunk / big_obstacle_base collision, replacing the invented POI scatter. Test c2 mapgen L0 (desktop MSVC and box gcc give the same 2331). |

## Интро Bonds Guru (порт из jacksmith-psp) — Протестирована (desktop кадры + PPSSPP boot)
- Источник (read-only): jacksmith-psp host_psp/gfx.c gfx_progress() + main.c; ассет work/splash_q.png (480x272, 123 цвета) -> cpp/data/intro/intro_bonds_guru.png.
- Поведение 1:1: полноэкранный сплэш, полоса 120x2 px в (180,239), фон #20262a, заливка #5ef2c3; 2% во время загрузки, затем 100% до 3.5 с от старта; без пропуска и без звука; затем главное меню.
- Реализация: Game::draw_intro / AppState_Loading (game.cpp), desktop берёт PNG из data/intro, PSP — из sprites.pak (pack_psp_sprites.js принимает доп. каталоги, pack_psp_deploy.ps1 передаёт cpp/data/intro).
- Харнесс: --shots-intro <dir> (кадры intro_f001/060/120/200, меню f215/230); остальные --shots режимы пропускают интро (номера кадров скриптов не меняются).
- PPSSPP headless: boot ok, sprites.pak 1138 entries, game init ok @665 ms, меню (save.mdaz probe) @3936 ms. Это только boot-check, не FPS.

## Map generator B2a: buildings from create_map_element category 7 - Implemented, not verified against original
- MapGenerator::generate_new_game no longer uses the invented POI list (place() of villages/cities/Police/Church at sector centres). One MapLocation per cat-7 row of the C2 generator (c2_mapgen), at the row position, with its *_int shape (subtype -> building_interior.inc -> building_solid.inc).
- Helicrash / bunker entrance locations come from C2Map heli/bunker points (L0: 0 helis, as in generate_helicrashes).
- Minimap/road sector grid = C2 array 511.
- Loot: one pile per spawn_loot_point of the building (item tables still the port's). Hostiles: per-cell budget on the first building only (temporary until zed_resp spawners).
- Seed 140 L0: 233 cat-7 rows -> 136 buildings (93 b_deer_stand rows have no *_int and are not drawn yet). Tests: ALL TESTS PASSED (MSVC).

## Map generator B2b: props from create_map_element categories 3/4/5/6 and 7:1 - Implemented, not verified against original
- New MapProps (map_props.hpp/.cpp, generated map_props.inc via cpp/tools/extract_props.py + gen_props.py from data.js): berry bushes (red/blue/yellow animation, choose(0,1) frame when IV3=0), waterpump, obstacles (fence/block/trash/lavka/fence & hesco h/v by IV6, ee_* objects), pillars (frame = IV2 when sub<12, else 12), cars (frame 1 when IV3=1), b_deer_stand.
- Solid obst_base / big_obstacle_base polygons at the object's image points (per object frame where C2 conditions on it; no polygon = bounding box) block movement and bullets (Game::building_blocks).
- Removed the invented hashed berry-bush grid. C2Map rows capacity 24000 -> 6000 (measured ~3.5k rows per map).
- Seed 140 L0: 688 props, 525 with a Solid base. Tests: ALL TESTS PASSED (MSVC).

## B2c — zombie spawners (zed_resp_point / Zed_check) + Humvee/heli wrecks (2026-10-19, 22:39 Baku)
Статус: **Протестирована** (unit tests + desktop shots; not yet compared frame-by-frame with the original).
- `Game::zed_check()` ports NPC_manager "Zed_check" (data.js event sheet): called at Start of layout and with
  every Location loader reload (1 s, camera moved > 400 px, not in bunker).
  - Cull: per zed/animal type one random instance beyond respawn_radius_npc (2000) is removed unless its
    keep flag (eyes IV6/IV7, horde spawns) is set; its zed_resp_point IV0 -= 1. Crows: all beyond 1500.
  - Spawn: ONE random zed_resp_point with 1500 < d <= respawn_radius_npc and IV1 false; type trees per frame
    0/1(IV2 0..2)/3/4/5(hordes_spawns_on)/6/7/8(IV2 1..5)/9 exactly as decoded; repeat (Zed_per_base - IV0);
    position point + random(500,-500) (frame 8: choose(750,-750)); rejected if inside a Solid.
  - zed_resp_point IV2 is now stored by the generator (C2ZedResp::iv2).
- Removed invented code: per-POI hostile budgets (spawn_hostiles_for_poi now a no-op), the timed
  ZED_RESPAWN zed spawner and the timed animal packs (wolves/deer/rabbits/crows/dogs).
- Zombie/Animal slots are reused after death/cull (was append-only, 128/64 cap filled up).
- Humvee: hammercrash_point -> Loc_Hummer: hammer_crash frame 1 sprite, big_obstacle_base frame 26 rect
  (w1..w2), 3 loot piles at image points 3,2,1. Heli: heli_crash frame 0 now drawn (was invisible),
  big_obstacle_base frame 19 polygon, piles at image points 3,4,5.
- Tests: Zed_check fill / no refill / cull; one wreck location per hammercrash/helicrash point.
- Harness: `--shots-wreck <dir>` (test-only placement). Shots: /workspace/b2cshots/b2c_grid.png
  (wreck_hummer.png, zr_spawn_t0.png, zr_spawn_t1.png).


## B2d — location tilemaps from create_map_location (2026-10-19, 22:48 Baku)
Статус: **Реализована, но не проверена** против оригинала покадрово (unit test + desktop shots).
- New `CellTilemaps` (include/mdaz/cell_tilemaps.hpp, src/world/cell_tilemaps.cpp): per mapgen cell
  ground tilemap (t475/681/682/771 by CurrentLevel, 60 px, at mapgen XY - 500, 1050x1050 from the
  first layout instance = template), ground_enviroment t221 (30 px) and optional Water t199.
- `src/world/create_map_location_tiles.inc` is generated by `cpp/tools/codegen_cml.py` from the decoded
  create_map_location event tree (all.txt 2987-8164): only tilemap actions (create, SetTile,
  SetTileRange, erase range) and their conditions (mapgen IVs, array 511, tile compare autotiling,
  random/choose, local VAR resets per event scope, OR blocks). Object spawns in that function are not
  emitted (handled elsewhere or pending).
- Streaming follows Reload_locations: cells within respawn_radius2 (+1000 at start / after save) are
  created, cells beyond are erased (40 slots).
- Removed invented drawing: stamp_from_mapgen / stamp_location_footprint dirt pads and road stamps, the
  procedural edge water/sand fringe and the hashed ground_enviroment scatter. The static Map layout
  tilemaps (ground_lvl1 strips, tilemap_sand, tilemap_water) are now drawn from world_tilemaps.txt.
- Test: all 256 cells build level-1 ground; road tile 15 in 64 cells, water in 15 (seed 140 map).
- Shots: /workspace/b2dshots/b2d_grid.png (spawn, explore1, explore2, run_c, wreck_hummer, zr_spawn_t1).

## Milestone after B2a-B2d (2026-10-19, 22:52 Baku)
- Desktop: ALL TESTS PASSED (Windows), shots /workspace/b2dshots/b2d_grid.png + wreck/zr shots.
- PSP: EBOOT rebuilt 22:50 (2 518 312 bytes), deploy packed (sprites.pak 1138 entries).
- PPSSPP headless boot check (not an FPS claim): platform init 269 ms, game init ok 665 ms,
  menu reached (~3936 ms), mem after game init: max free block 512 KB, total free 764 KB; no errors.
  The bench autoplay stayed in the menu, so in-game PSP memory with the new 40 cell-tilemap slots is
  not verified yet.

## PSP in-game memory check (2026-10-19, 22:58 Baku)
- Bench autoplay fixed: the NEW GAME presses moved to frames 330/390 (after the 3.5 s intro), walk from 480.
- main_psp logs memory 1 s and 20 s after AppState_Playing.
- PPSSPP headless (boot/emulation check, not an FPS claim): "new game: 137 locations" at 6607 ms;
  mem in game (1 s) and (20 s): max free block 512 KB, total free 764 KB (same as after init: Game and
  CellTilemaps are statically allocated, nothing grows at runtime). Ground and objects passes are
  active in the perf lines (ground ~1.1-1.4 ms, objects ~2.2 ms per frame, emulator timing). No OOM.

## B2c fix — Zed_check uses "For each" (2026-10-19, 23:01 Baku)
- Condition c90 is System "For each" (confirmed by the Tutorial start-of-layout events, which give every
  tree / building instance its obst_base / interior through c90). Zed_check therefore culls EVERY
  NPC beyond respawn_radius_npc (not one random per type) and fills EVERY eligible zed_resp_point
  (not one random point). Tanks are not in the original cull list and are kept.
- Earlier "pick random" reading of c90 elsewhere should be audited (KNOWN_ISSUES).


## Tutorial mode — C2 layout "Tutorial" (2026-10-19, 23:44 Baku)

Status: **Реализована, протестирована** (unit tests + desktop playthrough with screenshots + PPSSPP boot into the tutorial). Not yet **Сверена с оригиналом** side by side.

- Entry (Menu_Events): after the intro the tutorial starts when `settings.cfg` has no `tutorial_finished=1` (C2 localStorage `Tutorial_map_finished`). Tutorial_script_final sets it to 1 and returns to the menu. PSP `autoplay.txt` benchmarks skip the tutorial unless they contain `tutorial`.
- Layout (`tools/extract_tutorial.py` → `src/world/tutorial_layout.inc`, `data/tutorial_tilemaps.txt`): 6 buildings with interiors, 2 hammer_crash frame 0 (big_obstacle_base 0), 55 trees, 33 props, 10 ground items plus Spawn_drop 402/450, 3 zed points, crow, bots, b_exit boat, 3 pl12 zones, 8 tilemaps (Forests layer drawn above objects). mapgen is destroyed, so loot/airdrop/bandit respawn, quest bandits and location streaming are off.
- Steps (Tutorial_events): start chain (tip 3 at 2.5 s, encounters at 4.5 s), UI_guide (tips 3/6), Inventory_guide (tips 0/1/2/12), Firearm_guide (tips 7/8/9, zeds blind), Perks_guide (zone IV0=1 → tips 10/11, XP 316, crows), Bleeding_guide (zed contact → bleeding → tip 13), Tutorial_encounter (camera_roll ids 1–11, text 469–475/451/452/468, timescale 0.1/0), death guard (blood ≤ 5 → 100), boat Level_menu (42/41 or 46/44/45), Tutorial_script_final (bots, log 125–127, fade, 3 Ak74_1 shots, tutor_death).
- Tree chop now follows C2: 2 hits, then 2× Wood sticks (38) + 1× Wood piles (50) dropped at x±30, y+40+rand(10). Only tree types 1–6 can be chopped. The old random chop was removed.
- Tests: `mdaz_tests` (tutorial layout counts, tip 3/6 flow, movement block, death guard) pass on Windows.
- Desktop harness: `minidayz_desktop.exe --shots-tutorial <dir>` saved 25 shots covering every tip, 5 camera rolls, the boat menus and the final script.
- PSP memory check: PPSSPP headless with the tutorial loaded `data/tutorial_tilemaps.txt` (19 KB) at 4.2 s. The memory probe logged "max free block 512 KB, total free 764 KB" at 1 s and 20 s in game, the same as before game init. This is a boot check only, not an FPS claim.

## Item 7 — death animations from the *_dead objects (2026-10-19, 23:49 Baku)

Status: **Протестирована** (unit tests on Windows and the box; desktop `--shots-death` screenshots).
- Bug: corpse die frames were looked up in the living skin sheets (e.g. `wolf_skin-sheetN` = the living wolf; screamer living sheet). The original keeps die1/die2 in the separate *_dead objects (t210 zed_normal_skin1_dead, t396 fast, t213 army, t715 buried, t722 screamer, t470 wolf, t473 deer, t610 rabbit, t893 dog).
- New `tools/extract_die_anims.py` → `src/world/die_anims.inc` (exact sheets/rects/hotspots, 8 fps, no loop). The corpse plays die1 or die2 (random when both exist), then the **last frame stays**, drawn at native size with its hotspot. This replaces the invented "frame (1,1) of sheet0 at 0.9× world scale" static corpse.
- Deer, rabbit and dog now play their die1 (before: no animation). Tank: the dead object has only a 4-frame "Default" anim with no evidence of playback, so frame 0 is shown (KNOWN_ISSUES).
- Removed the invented crow stick drop that was still in `on_animal_killed` (already listed as removed on 2026-10-17).
- Tests: wolf frames come from wolf_dead, zed die2 = t210, last frame stays, deer 4 frames @ 8 fps = 0.5 s. The box-only "die anim" failures are gone.
- Doc fix: the tutorial heading time is 23:44 (commit time), not 23:50.

## Item 8 — craft audit against C2 sub_menu (2026-10-19, 23:53 Baku)
- Status: Реализована, но не проверена (recipes + unit test; no in-game screenshot).
- Source: `Call_sub_menu` drag pairs (gui_item_backpack onto pl19) and the t283 `sub_menu` recipe blocks.
- Already matching: Craft_1 (40+41→454), Craft_5 (37+41→163), Craft_4 (454+38→455).
- Added: Craft_3 (50+49→23), Craft_33/34 (26↔50→23), Craft_31/32 (sticks↔wood piles, first option "make campfire kit" t536[311] →23).
- Not done: Craft_21/22 "Craft arrows" (sticks + knife →51 ×Crafted). The Crafted count and the consumption expression `[5,X,[6,3,Crafted]]` are not yet resolved.
- Not done: the C2 craft wait (3/4/5 s with IV21=1). Crafting in the port is instant.
- Craft_fishing_rod result line not yet confirmed (port: 87).

## Item 9 — sickness (IV34) + day/night/rain timebase (2026-10-19, 23:59 Baku)
- Status: Реализована, но не проверена (unit tests only; no screenshots).
- Sickness, from the C2 States group:
  - While sick: −1 food and −1 water per 5 s stats tick, and no blood regeneration.
  - Gain: temperature ≤ 0 and random(50) > 28, checked each tick.
  - Cure: food, water and temperature all ≥ 80, not bleeding, then choose(1,2)==1.
  - Eating raw steak 60, rabbit meat 71, or herring/salmon/ruffe/perch 91–94: 50% chance to get sick.
  - Tetracycline 46 cures it.
- Removed invented sickness sources:
  - zombie-hit infection roll;
  - heavy-rain sprint roll;
  - digging roll;
  - rain-drinking roll;
  - blood DoT and fever.
- Clock:
  - +1 game minute every 1.5 s. Timer_ALL resets when it goes above 1440.
  - Hour 24 keeps the current Day_mode.
  - The clock now also runs in the bunker; C2 has no bunker condition on it.
- Rain:
  - The Rain_speed timer stops rain if it is raining. Otherwise, if not in the bunker and choose(1..4)==1, rain starts.
  - The invented rains_level increment is removed. rains_level is 1 at the start of level 0.
- Not done:
  - rains_level → 2 after Stats_time_lived ≥ 5000 (SEED_morezeds == 0).
  - AI event trees (zombie, animal, bandit): not audited in this step.

## Item 10 — quest pad strings + Bot_bandits_check_situation branch (2026-10-20, 00:11 Baku)
- Status: Реализована, но не проверена.
  - Unit tests pass, plus `--shots-quest` screenshots.
  - The pad screen itself is not ported; the strings feed the HUD quest panel.
- Decode method: C2 function indices were resolved with `c2runtime.js` getObjectRefTable. Examples: c78 = Sprite.PickDistance, exps 127 = Count, 533 = item-name table, 536 = UI strings.
- Quest strings, from the pad_list rows:
  - row1: 367 no task; 371 + newline + item name (task 1); 383 (task 2); 382 (task 3); 386 bandits eliminated; 387 failed.
  - row3: 381 + reward text for all 41 Q_reward_id values (21 has none), now `quest_reward_text`.
  - Time left: "Time left: m:s" (`quest_time_left_text`).
  - The HUD quest panel now shows row1. The invented "(have/1)" counter is removed.
- Bot_bandits_check_situation:
  - Runs every 400 + 2·karma s, only when CurrentLevel > 0.
  - Count ≤ 0 → Spawn_bandit_camp.
  - Otherwise, if not the quest camp (controller IV2, set for Q_task_id 2) and Q_status ≠ 0: when the nearest bandit is > 1200 from the player → Remove_bandit_bots.
  - Remove_bandit_bots removes all enemy bandits (no anchor/radius).
  - The port has only CurrentLevel 0, so this check never runs in the port, which matches C2 for level 0. It is not wired to a timer.
- Arrow recipe (Craft_21/22): decoded further, still not implemented.
  - Crafted = Arr.At(slot) of submenu item 1 (IV1 = the dragged backpack item).
  - Item 2's cell is set to At − 3·Crafted.
  - Output: Check_space_inventory(-1, 51, Crafted).
  - Which array holds counts vs condition for knives is unresolved, so no guess.

## Item 11 — Save_loot (2026-10-20, 00:13 Baku)
- Status: Исследована. Not applicable to the port: Level_change is not ported, and the port has a single level.
- What C2 Save_loot is: it is called only from `Level_change` (CurrentLevel += 1). It is not a ground-loot save.
- It builds a CSV text (pl17 t485) with:
  - `0,0,Difficulty,`, then layout name, round(x), round(y);
  - player IV15 blood, IV13 food, IV14 water, IV16 temperature, IV22 bleeding, IV34 sick;
  - every equipment and backpack slot as item id, IV6 condition/count and IV3 (zeros for empty slots), plus the attachment arrays;
  - Timer_ALL.
- `Load_loot` on the new level:
  - restores IV15/13/14/16/22/34 from fields 6–11;
  - then food −10 and water −10;
  - re-creates each item with Spawn_drop + picky at the player's position.
- The port's ground-pile persistence (MDAZ v14 `SavedLootPile`) is a port feature. Its comments wrongly called it Save_loot; they are now corrected.

## PSP rebuild + PPSSPP boot check after items 7–11 (2026-10-20, 00:17 Baku)
- EBOOT.PBP was rebuilt at 00:15 (2 548 328 bytes, make exit=0) and packed by pack_psp_deploy.ps1. The deploy has 12 files, 4.48 MB.
- The deploy contains no autoplay.txt and no settings.cfg. They were added only to the box test copy.
- PPSSPPHeadless, 60 s, autoplay "bench" (boot check only, not an FPS claim):
  - boot OK, sprites.pak 1138 entries, game init ok;
  - new game: 137 locations, 0 helicrashes, poi hash 438baa57. 0 helicrashes on level 0 matches the original (B2a finding);
  - mem in game: 512 KB max block, 764 KB total free, at 1 s and 20 s;
  - the bench bot then died (last_death.txt: reason=Bullet, days=1);
  - no crash; the log runs to the 60 s timeout.

## Removed invented effects (2026-10-20, 00:19 Baku)
- Status: Реализована, но не проверена (box and desktop tests only).
- Checked against data.js (decoded event dump):
  - no sleep/dig/forage events exist;
  - blood (IV15) is only raised by the States regeneration and by medical items;
  - shovel appears only as a melee weapon (condition wear, Drop_melee);
  - nothing drinks rain.
- Removed from game.cpp:
  - blood regen while resting at a fire, and the night "sleep" time skip;
  - shovel digging (buried zed pop + random dirt finds);
  - drinking rain;
  - random bush forage (sticks/berries/rags) and the forage-count farmer unlock.
- C2 berry bushes are not ported yet:
  - the bush shows frame 1 when it has berries;
  - the player walks to it, waits 3 s and plays "bushsearch";
  - it gives 27 red / 73 yellow / 74 blue berries, ×2 with perk_agronom.

## AI behaviour audit — zeds, animals, bandits (2026-10-20, 00:24 Baku)
- Status: Реализована, но не проверена (box and desktop unit tests; no in-game screenshots).
- Source: the behaviour properties of data.js layout instances (proj.pkl) plus the decoded Zeds, Animals and NPC event groups.
- Zeds (normal, fast, army, shooter, buried eyes):
  - MainLook LineOfSight range 335 (shooter 350) in a 180° cone; ShortLook 70 all round.
  - Every 1 s: no LOS → IV0 = 0 (stop chasing).
  - LOS in daytime → aggro.
  - LOS at night → aggro only if the player light overlaps the player and the distance is ≤ 175.
  - Ported as `c2_aggro` with a 1 s look timer. Facing = last chase direction (starts facing down, idle_down).
  - Removed invented hearing multipliers: run ×1.45, crouch ×0.55, bleeding ×1.35, cough ×1.2. data.js has no crouch.
  - Tank and screamer keep their port aggro radii (not audited).
- Wolves and dogs:
  - Chase range = LOS/Turret range: wolf 335, dog 200.
  - Removed the invented pack alert (range 520, ×1.15, deer stampede ×1.35) and the blood-scent bonus (+140 range, ×1.1).
  - The 180° cone is not ported.
- Deer and rabbits: flee when the 1 s LOS check finds the player within 300 (deer) / 250 (rabbit). The port used 250 for both.
- Crows: the invented ×1.15 vertical flee factor is removed.
- Bandits (bot_enemy_bandit/puncher/shooter): Eyes LineOfSight range 360, cone 360. aggro_radius is now 360; it was 380 with a gun and 260 without.
- Not audited: zed/animal attack damage and cooldowns, warn_zed wandering, the bandit Aim/Gun behaviours, and tank/screamer.

## Berry bushes (2026-10-20, 00:27 Baku)
- Status: Реализована, но не проверена (builds and tests pass; not exercised in play).
- C2 behaviour (data.js):
  - create_map_element cat 3, sub 1/3/4 places red/blue/yellow t287 bushes, frame choose(0,1); frame 1 = has berries.
  - When the player overlaps a frame-1 bush (and IV21==0): sound "bushsearch", frame → 0, wait 3 s.
  - Then Check_space_inventory(-1, 27 red / 73 yellow / 74 blue, perk_agronom ? 2 : 1).
- Port:
  - interact within 24 px of a frame-1 bush prop starts the 3 s timer;
  - the item is added when the timer ends;
  - perk_agronom is read from the balance globals (0; the perk is not a runtime unlock in the port).
- Gaps:
  - the C2 drop-to-ground when the inventory is full is not ported (add_item fails silently);
  - bush frames are not saved.

## Crouch removed (2026-10-20, 00:27 Baku)
- data.js has no crouch/sneak (0 matches in the decoded events), so crouch was a port invention.
- Removed:
  - the crouch inputs (SDL Ctrl and the PSP L hold, now always false);
  - the controls hint "Hold: crouch".
- Game::crouching_ is always false, which removes:
  - crouch move ×0.45, braced aim ×0.55 and faster dispersion settle;
  - the bandit spread +4 and the crouch sprite size/reach;
  - zed loudness.
- The PSP L button keeps its other combo uses.

## 2026-10-20 00:40 (Baku) — Attack audit (item 3, partial)
- Zed punch: all types fire on a regular 1 s bPunch timer (pl19 skins and zed_tank_skin). Port cooldowns 0.7/1.4 s replaced with 1.0 s. Status: Реализована, но не проверена.
- Tank damage = Zed_dmg×2 (data.js 46163/46222); it was army×1.5 (invented). Status: Сверена с оригиналом (code-level).
- Wolf: battack timer 1.5 s regular, damage Zed_dmg. Cooldown was 3 s (unsourced), now 1.5 s.
- Dog: t892/t894 is the player's companion (Dog_check_situation follows the player). It never calls Player_get_hit, so the invented dog→player damage was removed. The companion AI itself is not ported: Не исследована → Исследована.
- Not done in this step: wolf/dog 180° cone, zombie wandering (warn_zed), bandit aiming, screamer behaviour. All Не исследована/Исследована; see KNOWN_ISSUES.

## 2026-10-20 00:50 (Baku) - PSP hint wording (user request, item 1)
- User explicitly asked for PSP wording in hints that name touch/keyboard/mouse controls. Deliberate deviation, list in KNOWN_ISSUES "PSP hint wording".
- Port: `kUiPsp` override table in `src/world/tutorial_layout.cpp` (`tut_ui_text` returns PSP text, `tut_ui_text_original` keeps l_eng_ui.xml verbatim); `kPspOverrides` in `src/ui/ui_strings.cpp` for the same ids plus XML-only ids 33/230. Buttons taken from `platform_psp.cpp` poll_input and `Game::update_inventory_ui`.
- Evidence: `test_tutorial` prints C2 vs PSP line for 9 ids and checks that none of the 30 tutorial lines contains tap/drag/mouse/click/key/"this button" -> OK; ALL TESTS PASSED. Shots `cpp/shots_fix1/tut_*.png` (467 shown inside inventory matches the hint bar). Status: **проверено** (desktop unit test + shot).

## 2026-10-20 01:10 (Baku) - Car trunks (user item 2)
- Decoded with `tools/c2dump.js` / `c2path.js` (getObjectRefTable names, OR/Else kept):
  - create_map_element cat 6: when the car's IV3 = 0, it spawns car_loot_point (t232) at car Imagepoint 1 on layer Sunset_Dawn, with a per-car frame (0 regular, 3 vertical, 1 van, 2/4 police, 5 hatch; blue/red horizontal SetMirrored(0)). Btr and truck have none.
  - Prompt: avalible_interaction = 3 (nearest point < 50 px, doorchecker LOS = family t927 b_*_int) -> SetVisible(1) (white trunk outline) and gui_btn_interact frame 3.
  - Press: nearest point < 100 px and player_collision_base (30x30) overlaps -> open, else walk_marker IV0=4 (AltMove toward the point; opens on contact).
  - Open: sound "open"; Map choose(1,2)=2 -> Spawn_drop choose(list for the frame) at the "loot" image point + log 7 "I found something in this trunk.", else log 8 "There is nothing in this trunk."; Tutorial always (2,8,9,4) or 54 for frames 2/4; the point is destroyed.
- Port: `src/world/car_trunk.inc` (generated by `tools/gen_car_trunk.js`), MapProps trunk/visible bitsets (2 x 751 B), `Game::try_trunk_interact` / `open_trunk`, outline drawn (mirrored via new `IPlatform::draw_sprite_frame_mirrored`, SDL + PSP), HUD icon + Circle "TRUNK".
- Evidence: `--shots-trunk` log: placed 40 px away -> walk -> opened prop 365 (frame 2); 61 trunks opened, found 32 (~50 %); outline shown on the trunk, gone after opening. Shots `cpp/shots_trunk/`. Tests ALL TESTS PASSED.
- Found on the way: `Rng::next_int` uses LCG low bits (`% 2` alternates); 50 % rolls with next_int(1,2) gave 58/61. Trunk uses next_float. Status: **проверено** (desktop harness log + shots); PSP not run yet.


## 2026-10-20 01:20 (Baku) - Rng::next_int fix + vehicle stash removed (user decisions a/b)
- `Rng::next_int` now maps the range with the high bits (`(u64)next_u32() * span >> 32`); `% span` on LCG low bits alternated (bit 0 period 2). Every choose()/random() port is affected; seed-140 map changed: **poi hash 7cbc5967 -> 438baa57** (test prints it; no hard-coded hash). test_queue_q1 warn_zed check rewritten to "moved >= 30 px toward the noise" (the old +y check depended on the old police position). mdaz_tests ALL TESTS PASSED.
- The invented vehicle trunk stash (G near a drivable car stores/takes items) removed from `Game::try_drop_slot`; not in data.js (Trunk_count unused in events). Vehicle stash fields stay in the v7+ save layout (always 0) only for save compatibility.


## 2026-10-20 02:05 (Baku) - Car trunk finished 1:1 (data.js re-decoded with tools/c2chain.js)
- Open: family t920 (all 17 car types) IsOverlapping car_loot_point -> SetAnimFrame 1 = trunk lid up (car sprite frame 1); was missing. Sound PlayByName "open" -5 dB (0.5623 linear).
- Two-stage walk_marker: IV0 = 7 at the nearer car_loot_point image point wp2/wp1 (+5 y on vertical cars, IV1), AltMove (dx, dy) * 100, SetSpeed 100 -> on reaching it IV0 = 4 at the nearest point origin -> opens if player_collision_base overlaps. Player speed forced to 100 while walking.
- Lists per branch from data.js: direct (sid3302144065259174) frame 0/3/5 list has 26, walk (sid1074032691412307) has 24; frame 1 lists differ (direct ..18..13,63; walk ..24..166..58,58). Tutorial: (2,8,9,4) for frames 0/1/3/5, 54 for 2/4 (+ tutorCarTrunkOpened flag).
- Spawn_drop IV1 count rules generated from Function "Spawn_drop" (tools/gen_spawn_drop.js + gen_spawn_drop_inc.js -> src/world/spawn_drop_count.inc): ammo int(random(a,b)) + perk_farmer 3, flare/sticks/arrows/22lr/9x18/9x39 choose(), matches.
- map_element IV3 = 1 + save v19 (opened-trunk bitset 751 B + ach_gta); load re-applies car frame 1 and removes the point. ach_gta +1 per open (SEED_RUN = 0).
- Outline: the parent event sets every car_loot_point SetVisible(0) each tick, so the outline is drawn only for the current avalible_interaction = 3 point (was: stayed visible).
- Evidence: --shots-trunk log: f220 stage 7 -> f230 stage 4 -> f240 open "walk prop 365 frame 2 -> item 12 x10", car_frame 0 -> 1, ach_gta 1; 61 opens -> 31 found; counts e.g. 45acp x12, 9x18 x15. Test "v19 opened trunks + ach_gta saved". mdaz_tests ALL TESTS PASSED. Shots cpp/shots_trunk (trunk_0_near, trunk_1_walking, trunk_2_opened, trunk_3_lid_up). Status: **проверено** (desktop); PSP not run.
