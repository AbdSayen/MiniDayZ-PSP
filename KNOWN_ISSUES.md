# Known Issues & Limitations

Last updated: 2026-10-17 (IV6 armor wear, melee def_percent, Death_guide sprites; PSP EBOOT still unverified)

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
| Death screen | Native `dead_screen` + stats; `last_death.txt` uses `Death_log_*` field names. **Partial:** `Death_guide` sprites (`gui_tutor_tip_bg`, `bot_tutor_*`) after delay; `tutor_death` audio on death. Full C2 Death_guide event tree / text panels not decoded. |
| Map generation | POI counts from `balance_globals`; static Map-layout borders; runtime dirt **tile 15** POI footprints + **generate_array_roads_1/_2** sector markers (**12/13/14** between paired POI **15** cells on rows/cols) stamped as dirt corridors; heli water **tile 4**. **Reload_locations** partial (see prior notes). **River** band seed-only (export never uses `river_X/Y`). Full `generate_array_locations` Array **150** cell types (6/9/11/…) not fully ported — roads use POI sector **15** only. `generate_array_tomap` / `generate_mapgen_toelements` not ported. |
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
| Drivable cars | **Partial C2:** UAZ/Volga/Truck spawn; IV284 fuel **−8/0.1s**, max **100**, gas **+40**; crash hp **15/30**; UAZ/Volga **600s+1s** respawn. **GUI_draw_vehicle** partial: `gui_car_bg`, vertical **bar_fuel** / **bar_hp** (IV284/294), nofuel/disable icons, car button sprites; hidden when not driving (**GUI_erase_vehicle** / States). AltMove speed, full **Vehicles UI** / dpad car layer, truck respawn, boat fuel tick **not** verified. |
| Zed / animal HP | **NPC_get_damage** wired (`npc_apply_damage` + kind map 0–11); HP bar uses C2 `min(30, hp×scale)`. Max HP / speed still stubs pending full IV extract. |
| Difficulty NPC dmg | Novice/Regular/Veteran tables for Zed/Army/Wolf from C2 States assigns; Zed_fast Novice/Veteran interpolated (4/8). **Wolf→player** hits use **Zed_dmg** in `Wolf_dmg_deal` (not `Wolf_dmg` global); player→wolf still uses `Wolf_dmg` via `NPC_get_damage` kind 0. |
| Bullets_dmg_deal / armor | **Sverka (partial):** rolled dmg ±25%; `NPC_get_damage` kinds 0–11; player bullets skip vest/helmet **mul** (`Player_get_hit` bullet path). **IV6** wear on bullet + melee hits (`dmg_randomizer` 3–7 → helmet/vest/outer/pants/backpack slots); broken armor (IV6≤0) drops mul. **IV8** second hit on army/tank zeds for mosin/SKS/SVD/repeater/SV-98/sawed mosin when `dmg < random(0,100)`. Melee **def_percent** block roll (IV4 tiers 5/15/25/35 + `perk_meleedef`). **Not ported:** full `dmg_hint` IV15 frame routing, exact IV6 max per item from C2 spawn, bullet IV2 hitbox / kill cam, full bot kind 6–10 routing. |
| Wolf attack | **Sverka (partial):** range **50**, cooldown **3s** (ScrollTo), SFX `wolf_attack`/`_2`/`_3`. Animation/interactor overlap not simulated — distance check only. |
| Loot save/load | MDAZ **v14** stores up to **32** ground piles + `loot_respawn_t` (`LOOT_RESPAWN_TIME`=1480 default). C2 `Save_loot`/`Load_loot` GUI CSV + per-pile `checkpile`/`Spawn_drop_from_player` not ported; `LOOT_respawn` stagger timers (**3/4/5s** on obj209) not replicated — only global recharge timer. |
| Pickup space | `Check_space_inventory` → `can_add_item` before pickup; no clothing-container slot checks (`Check_space_pants` full tree). |
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
| Character anims | Idle/run/axe/pistol/twohand/using_item + zed walk/run/attack from `char_anims.txt`. **Death:** zed/wolf play `die1`/`die2` ~0.55s on kill when frames exist, then static dead sheets; tank/screamer/deer/rabbit often skip (no living-skin die in export). Player still `player_skin_dead` sprite. |
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
