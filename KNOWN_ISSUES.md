# Known Issues & Limitations

Last updated: 2026-10-17 (Death_guide decode, MDAZ v18 loot ext, mapgen toelements; PSP EBOOT still unverified)

## External / environment

| Issue | Impact | Status |
|-------|--------|--------|
| No Construct `.capx` — only HTML5 export | Some editor-only data unavailable | Permanent for this repo |
| PSPSDK not verified on this PC | No successful EBOOT yet | **Do not claim PSP support** |
| Online login / DeltaDNA / ads in original | Not ported | Intentionally stubbed offline |

## Incomplete / approximate logic

| Issue | Detail |
|-------|--------|
| Item numeric stats | Most eat/drink IDs sverka from `item_activate` (`item_activate_iv_sverka.json`). **Morphine (45) blocked** — no `CompareParam` 45 in C2 export (`consumable_item_activate_sverka.json`). Raw meat/fish cook-only; mushroom easter egg. |
| Firearm base damage | **Sverka:** Fist **10**/range **45**, FNX (151) bullet IV0 **23** via `Game_events` spawn chain (`tools/_scan_bullet_spawn_iv0.js` → `weapon_bullet_iv0_sverka.json`), sword **15**, hit variance ±25%, bleed start `random(3)>=1.5`. All other firearms use **`kGunDmgPlaceholder` (25)** in `weapons.cpp` — not sverka. Do not promote `extract_gun_iv0_deep.js` candidates without the IV1←172 + IV0 literal path. |
| Mag capacities | Approximate; not fully extracted from C2 reload events. |
| Survival / weather timebase | `Timer_ALL`/`Day_Night_speed`/`Rain_speed` drive phase + rain toggle; cold globals applied when non-zero. Still approximate vs full C2 Day&Night event tree. |
| Molotov / bow | **Sverka (partial):** molotov throw speed **300**, ~**5s** flame, **0.5s** `Molotov_damage` ticks (10/9/8/7/6 by target class). Bow **163**: **V** nocks arrow (**2s**, `Bow_reload`/`Mag_reload` path); crossbow uses mag reload. |
| Death screen | Native `dead_screen` + stats; `last_death.txt` uses `Death_log_*` field names. **Partial:** `gui_tutor_tip_bg` / `bot_tutor_*` after `deathtime` delay; UI **#542** “You are dead”; `tutor_death` audio. **Decoded:** `Death_guide` is **tutorial-only** (dict **#466** fake-death text, `Deadscreen` layout) — see `cpp/docs_gen/functions/Death_guide_decoded.txt`. Sandbox death has no decoded tutortext panel. |
| Map generation | POI counts from `balance_globals`; static Map-layout borders; runtime dirt **tile 15** POI footprints + **generate_array_roads_1/_2** sector markers (**12/13/14** between paired POI cells on rows/cols) stamped as dirt corridors; heli water **tile 4**. Sector grid tags **village/city/military** as **6/9/11**. **generate_location** → **72** forest SetTile stamps (`map_decode_tables.hpp`). **`generate_mapgen_toelements`** **verified:** ForEach obj507 → `generate_location`. **`create_map_element`** **partial:** zed_resp tile → obj501 **IV10** (`create_map_element_decode.hpp`); **`map_element-sheet0`** at POI forest stamp tile-0 when sprite loaded; zed_resp grid + IV10→prop frames **blocked**. **River** seed-only. |
| Bunker | RLE load + walkable dens; typed bunker/mil/hosp loot + multi-type zeds — still not full C2 bunker event tree. |
| Crafting | Partial recipe set. **`Craft_1`/`Craft_4`/`Craft_5`** ported (bag/pack/bow via `Spawn_drop`). Event params `[[0,308]]` etc. are **object-type table indices** (e.g. 308=`test_bullet_hit`, 389=`composite_arrows` sprite), not item IDs — see `cpp/docs_gen/craft_uid_map.json`. **`Craft_21`/`Craft_22`** (tab **310** `obst_fence`): UI gates on knives **42–44** and/or sticks **38**; On-function **`Crafted` + `pickpile`/`checkpile`** stick yields via helper counts (2/8/10/12/18/20), not inventory `Spawn_drop` IDs — see `cpp/docs_gen/craft_decode_Craft_21_33.json`. **`Craft_31`/`Craft_32`** (tab **311** + loader **334**): **`build_fence1`/`build_fence2`** world placement + `craft_ok`; menu checks **50** and/or **38+50**. **`Craft_33`** (tab **311**): menu **50** or **26+50**; same pickpile/`craft_ok` family. Audit strings only in `craft.cpp`; fence-from-pile gameplay uses equip woodpile → `try_place_trap(2)` separately. **`Craft_3`/`Craft_34`** still blocked similarly. |
| Reload / ammo | **Sverka:** `Mag_reload` sets `Reloading_mag`, pistol path uses `reload_pistol` when `player_weapons` IV93==0, rifle uses `Reload_rifle`; `Reloading_time=1` on many pistol paths; `+19` ticks added in `Mag_reload_body.json`. Port: R/V → `try_mag_reload`, `consume_ammo_for_reload` (bag then IV19→17→11→9 cascade), pistol 1s reload. **Partial:** clothing IV93 containers — 4 sub-slots/region, gates from worn piece; per-garment cargo volume (Array **536** col **301** / IV10 UI in `Items_update`) — table not in export, not wired. Exact per-weapon IV0 reload ticks still open. |
| Bunker zeds | **Partial C2:** `Zed_check_bunker` → `spawn_bunker_zeds(need)`; **21** `bunker_zed_spawner` XY from export (`bunker_zed_spawners.json`, `%` layout **approx**) then random walkable fallback. |
| Bunker entrance | `ensure_bunker_entrance` when **CurrentLevel≥1** (new game uses level 1); C2 `items_on_ground` instance / full spawn sheet not decoded. |
| Quests | Chain zeds→…→church→military→helicrash→day2 — not full C2 tutorial tree. |
| Farming | Seeds plant → grow (~35–50s) → E harvest; rain/night modifiers; plots in MDAZ **v8**. Not full C2 garden plots. |
| Placed traps | Bear/barbed/fence/mine/claymore persist in MDAZ **v9**. |
| Campfires / tent | Active fires + tent warmth timer persist in MDAZ **v11**; adrenaline in **v12**. |
| Tree chop | Fell trees leave stumps that regrow ~90–150s. |
| Airdrop timer | Uses `Airdrop_Timer_ALL`=360 (was recharge 400); radio ETA HUD shows estimate. |
| Vehicle stash | 4 slots via G near vehicle; persisted in MDAZ **v7**. |
| Drivable cars | **Partial C2:** UAZ/Volga/Truck spawn; fuel max **100**, gas **+40**; crash hp **15/30**; UAZ/Volga **600s+1s** respawn (**verified** in `Drivable vehicles_decoded.txt`). IV284 **−8/0.1s** tick, **boat** fuel, **truck** respawn **blocked** in `data.js` export (`vehicle_fuel_truck_decode.txt`). **GUI_draw_vehicle** partial: `gui_car_bg`, vertical **bar_fuel** / **bar_hp** (IV284/294), nofuel/disable icons, car button sprites; hidden when not driving (**GUI_erase_vehicle** / States). **Sverka (partial):** mount flow mirrors **AltMove** + **Pin** (t184/t946) from `Drivable vehicles` decode; **`kDriveSpeedApprox=160`** — custom AltMove/Vehicle behavior JS missing from export, so px/s not decodable. Full **Vehicles UI** / dpad, truck respawn, boat fuel tick **not** verified. |
| Zed / animal HP | **NPC_get_damage** wired (`npc_apply_damage` + kind map 0–11); HP bar uses C2 `min(30, hp×scale)`. Max HP / speed still stubs pending full IV extract. |
| Difficulty NPC dmg | Novice/Regular/Veteran tables for Zed/Army/Wolf from C2 States assigns; Zed_fast Novice/Veteran interpolated (4/8). **Wolf→player** hits use **Zed_dmg** in `Wolf_dmg_deal` (not `Wolf_dmg` global); player→wolf still uses `Wolf_dmg` via `NPC_get_damage` kind 0. |
| Bullets_dmg_deal / armor | **Hit zones:** bullet IV2 1/2/3 vs player **blocked** for head/body/legs mul (`bullet_hit_zones_decode.txt` — branches are kill-cam/IV8). **Sverka (partial):** rolled dmg ±25%; `NPC_get_damage` kinds 0–11; player bullets skip vest/helmet **mul** (`Player_get_hit` bullet path). **IV6** wear on bullet + melee hits (`dmg_randomizer` 3–7 → helmet/vest/outer/pants/backpack slots); broken armor (IV6≤0) drops mul. **IV8** second hit on army/tank zeds for mosin/SKS/SVD/repeater/SV-98/sawed mosin when `dmg < random(0,100)`. Melee **def_percent** block roll (IV4 tiers 5/15/25/35 + `perk_meleedef`). **Hit_effect** layout flash (100% / 0.2s) on player damage. **`dmg_hint`** decoded as floating dmg text (obj529), not hit sprite frames — not ported. **IV6 max** per armor item: `clothing_iv6_max()` approx (no proven Spawn_drop IV6 init path in export). **Bullet IV2:** travel decay 3→2→1 @ 100/220px (`Range_less_power`); player hitbox zones (IV2 1/2/3 vs obj172) / kill cam still open. |
| Wolf attack | **Sverka (partial):** range **50**, cooldown **3s** (ScrollTo), SFX `wolf_attack`/`_2`/`_3`. Animation/interactor overlap not simulated — distance check only. |
| Loot save/load | MDAZ **v18:** **96** ground piles (32 legacy + 64 ext) + `loot_respawn_t`. C2 **Save_loot/Load_loot** CSV decoded (`save_loot_csv_decode.txt`: obj909 ForEach, token indices 6–12) — GUI CSV not ported. **checkpile** = inventory/craft stick UI, not save format. Per-pile **LOOT_respawn** stagger (**3/4/5s**) unchanged. |
| Pickup space | **Partial C2:** `Pick_inventory_item` → `pick_inventory_item` / `Check_space_inventory` then `Check_space_pants`→outerwear→vest→backpack when bag full (`clothing_containers.cpp`). C2 space checks use IV93 **slot** Arrays (`Contains` / empty cell), not col 301 volume sum. |
| Inventory UI | **Partial C2:** Tab toggle/hold opens bag (`Inventory_opened`); **Esc** / Tab-close / **V** mag reload / death call `close_inventory`. Hotbar **1–8** + **C** “Using inventory items” on equipped slot. **`Pick_player_item`** / **`init_item_activation`** decoded (GUI → `item_activate`); not ported headless. |
| POI loot tables | Typed tables (mil/hosp/gas/heli/police/fire/church/secret) + weighted Spawn_drop — not full C2 Check_loot trees. |
| AI | Zed/animal/bandit behaviors functional but incomplete vs C2 event trees. **`bot_reloading`** ported for firearm bandits (IV9 mag, IV10 reload, IV14 mag table + wait 2/2.5/3s in `bandit.cpp` / `update_bandit_guns`). Bot shooter bullet params / full obj243 spawn sheet still partial. |
| Audio | Original OGG tags mapped for common cues; not every C2 audio tag. |
| Starter inventory | Lean survival kit by difficulty in `game.cpp`. `starter_inventory_decode.json`: Start events hints only (ids 1–4 counts), no Spawn_drop seed table — **blocked**. |
| Plus / Reloaded | Out of scope (baseline 1.4.1). |

## Not confirmed in original (possible invented mechanics)

Audit 2026-10-17 (`node tools/audit_port.js` + keyword search in `original_docs/data.js`): no evidence in the C2 export for the following. They violate "не реализуй отсутствующие в оригинале механики" unless proven — candidates for removal.

| Feature | Evidence |
|---------|----------|
| Craft_heatpack / Craft_flare / Craft_baseball_bat | 0 hits in data.js; real Craft_* ids are Craft_1/3/4/5/21/22/31/32/33 + arrows |
| Craft_3 / Craft_21 / Craft_22 recipe contents | Ids exist, but ingredient UIDs (308–311) not resolved — recipes are guesses |
| Tree stump regrowth | 0 hits (regrow/stump) |
| Overencumber slowdown | 0 hits (encumb/weight) |
| Drinking from shore | 0 hits (drink_water/pond/well) |
| Limp / leg fracture | 0 hits (limp/fracture/legs) |
| Radio shortens airdrop by 45 s | not found in Airdrop_* events |
| Night aggro ×1.25 (zeds/animals), wolves fear fire | 0 hits (fear); not in Zombies/NPCs groups |
| Rain extinguishes torch | not found |
| Shovel caches | not found |
| Typed zed-corpse loot | not found in Spawn_drop |
| Quest steps Military / Helicrash | C2 quests use Q_task_id/Q_target_sid — order not extracted |

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
