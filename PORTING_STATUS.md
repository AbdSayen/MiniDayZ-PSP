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
