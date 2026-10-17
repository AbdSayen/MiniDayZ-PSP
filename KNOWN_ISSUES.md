# Known Issues & Limitations

Last updated: 2026-10-17 (desktop playable; PSP EBOOT still unverified)

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
| Survival / weather timebase | `Day_Night_speed` / `Rain_speed` globals used; Construct timeline units ≠ wall-clock seconds — phase advance approximate. |
| Map generation | Sector POI grid + roads/trees/water band — **not** full C2 `Map generation` / Location loader tilemaps. No real RLE `ground_lvl*` world data yet (visual tiling is procedural from atlases). |
| Bunker | RLE load + walkable dens; typed bunker/mil/hosp loot + multi-type zeds — still not full C2 bunker event tree. |
| Crafting | Partial recipe set (bow/arrows/bag/molotov/cook/fire/splint/rod rope+duct/fence/burlap/spinning…). Full `Craft_*` groups incomplete. |
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

## Presentation gaps (priority 5)

| Issue | Detail |
|-------|--------|
| Overworld tiles | Uses `ground_lvl1–4` / forest / sand / water atlases with biome hash — not original RLE tilemap state. |
| Clothing layers | Skin swap by worn tier + gasmask badge; not full per-piece clothing spritesheets on body. |
| Exact HUD layout | Scaled GUI crops from original sheets; some panels approximate. |

## Build notes

- Desktop: MinGW + vendored SDL2 — see `BUILDING.md`.
- Tests: `mdaz_tests.exe` (survival, combat constants, craft, bunker, save, systems).
- Successful runs must be recorded in `PORTING_STATUS.md` when claimed.

## Legal

Assets and game design belong to Bohemia Interactive / original authors. This tree is a technical porting effort from publicly mirrored browser files; redistribution of assets may be restricted.
