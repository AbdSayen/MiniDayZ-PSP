# Known Issues & Limitations

Last updated: 2026-10-17 (molotov/bow/weather pass; PSP EBOOT still unverified)

## External / environment

| Issue | Impact | Status |
|-------|--------|--------|
| No Construct `.capx` — only HTML5 export | Some editor-only data unavailable | Permanent for this repo |
| PSPSDK not verified on this PC | No successful EBOOT yet | **Do not claim PSP support** |
| Online login / DeltaDNA / ads in original | Not ported | Intentionally stubbed offline |

## Incomplete / approximate logic

| Issue | Detail |
|-------|--------|
| Item numeric stats | Many food/water/heal values are placeholders (`known_stats=false`). Whiskey and some cooked meats marked known where IV literals were clear. |
| Firearm base damage | **Sverka:** Fist **10**/range **45**, FNX IV0 **23**, sword **15**, hit variance ±25%, bleed start `random(3)>=1.5`. Other guns in `weapons.cpp` are **PLACEHOLDER** (heuristic / DayZ-like stubs). Deep IV0 scan (`tools/extract_gun_iv0_deep.js`) is noisy — do not promote candidates to sverka without event-path proof. |
| Mag capacities | Approximate; not fully extracted from C2 reload events. |
| Survival / weather timebase | `Timer_ALL`/`Day_Night_speed`/`Rain_speed` drive phase + rain toggle; cold globals applied when non-zero. Still approximate vs full C2 Day&Night event tree. |
| Molotov / bow | **Sverka (partial):** molotov throw speed **300**, ~**5s** flame, **0.5s** `Molotov_damage` ticks (10/9/8/7/6 by target class). Bow **163**: **V** nocks arrow (**2s**, `Bow_reload`/`Mag_reload` path); crossbow uses mag reload. |
| Death screen | Native `dead_screen` + stats; `last_death.txt` uses `Death_log_*` field names. Full C2 `Death_guide` tutorial overlay not ported. |
| Map generation | POI counts from `balance_globals`; static Map-layout borders from `world_tilemaps.txt`; runtime dirt **tile 15** pads/crosses + corridors + heli water **tile 4** via `WorldTilemaps::stamp_from_mapgen`. **River:** C2 `generate_array_locations` declares locals `river_X`/`river_Y`/`river_R` (default **R=2**) and `Bridges`/`Bridges_2`, but **data.js never reads/writes them** — port stamps a seed-based horizontal/vertical **tile 4** band (`tilemap_water`) using `river_R` width only. Full C2 river path / bridge tiles (map_element IV==4 + `199,185` ranges) not decoded. POI clusters in `poi_layout.cpp`; roads Array still incomplete. |
| Bunker | RLE load + walkable dens; typed bunker/mil/hosp loot + multi-type zeds — still not full C2 bunker event tree. |
| Crafting | Partial recipe set. **`Craft_1`/`Craft_4`/`Craft_5`** ported (bag/pack/bow via `Spawn_drop`). Event params `[[0,308]]` etc. are **object-type table indices** (e.g. 308=`test_bullet_hit`, 389=`composite_arrows` sprite), not item IDs — see `cpp/docs_gen/craft_uid_map.json`. **`Craft_3`/`Craft_21`–`22`/`Craft_31`–`34`** still blocked (pickpile/`Crafted`/world fence; no verified `Spawn_drop` result). |
| Reload / ammo | **Sverka:** `Mag_reload` sets `Reloading_mag`, pistol path uses `reload_pistol` when `player_weapons` IV93==0, rifle uses `Reload_rifle`; `Reloading_time=1` on many pistol paths; `+19` ticks added in `Mag_reload_body.json`. Port: R/V → `try_mag_reload`, `consume_ammo_from_bag`, pistol 1s reload. **Not ported:** `Check_ammo_pants/vest/outerwear/backpack` (clothing container slots), exact per-weapon IV0 reload ticks. |
| Bunker zeds | `Bunker_spawn_zeds` runs on each bunker enter (budget `Zed_per_base`); loot still once per save via `bunker_looted`. Spawner XY from `bunker_zed_spawner` layout not wired — random walkable tiles. |
| Quests | Chain zeds→…→church→military→helicrash→day2 — not full C2 tutorial tree. |
| Farming | Seeds plant → grow (~35–50s) → E harvest; rain/night modifiers; plots in MDAZ **v8**. Not full C2 garden plots. |
| Placed traps | Bear/barbed/fence/mine/claymore persist in MDAZ **v9**. |
| Campfires / tent | Active fires + tent warmth timer persist in MDAZ **v11**; adrenaline in **v12**. |
| Tree chop | Fell trees leave stumps that regrow ~90–150s. |
| Airdrop timer | Uses `Airdrop_Timer_ALL`=360 (was recharge 400); radio ETA HUD shows estimate. |
| Vehicle stash | 4 slots via G near vehicle; persisted in MDAZ **v7**. |
| Zed / animal HP | Melee dmg sverka by difficulty; HP/speed values still stubs pending object IV extract. |
| Difficulty NPC dmg | Novice/Regular/Veteran tables for Zed/Army/Wolf from C2 States assigns; Zed_fast Novice/Veteran interpolated (4/8). |
| POI loot tables | Typed tables (mil/hosp/gas/heli/police/fire/church/secret) + weighted Spawn_drop — not full C2 Check_loot trees. |
| AI | Zed/animal/bandit behaviors functional but incomplete vs C2 event trees. |
| Audio | Original OGG tags mapped for common cues; not every C2 audio tag. |
| Starter inventory | Lean survival kit by difficulty (food/water/knife/rags/matches/clothes; Novice +hatchet+bag). Exact C2 seed table unextracted; debug mega-kit removed 2026-10-17. |
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
| Menu | Background is the original Menu layout (tilemaps + 149 sprites from `tools/extract_layout_sprites.js`), slow camera pan, `menu_btn_wood` buttons. Pan speed/path is a guess, not taken from C2 events. |
| Font | 5×7 bitmap font drawn with rects, not the original `bitmap_font` sheet. |
| Minimap | Dark square with markers; original minimap rendering not ported. |
| Buildings | POI sprites drawn at native frame size × world scale; **2-building offsets** (~±150 world units from `create_map_location` ±300 SetTile windows) per `poi_layout.cpp`. Not C2 `Buildings_base` instance var placement. |
| Clothing layers | Skin swap by worn tier + gasmask badge; not full per-piece clothing spritesheets on body. |
| Exact HUD layout | Scaled GUI crops from original sheets; some panels approximate. |

## Build notes

- Desktop: MinGW + vendored SDL2 — see `BUILDING.md`.
- Tests: `mdaz_tests.exe` (survival, combat constants, craft, bunker, save, systems).
- Successful runs must be recorded in `PORTING_STATUS.md` when claimed.

## Legal

Assets and game design belong to Bohemia Interactive / original authors. This tree is a technical porting effort from publicly mirrored browser files; redistribution of assets may be restricted.
