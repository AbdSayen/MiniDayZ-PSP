# Known Issues & Limitations

Last updated: 2026-10-16

## External / environment

| Issue | Impact | Status |
|-------|--------|--------|
| No Construct `.capx` source — only HTML5 export | Some editor-only data unavailable | Permanent for this repo |
| SDL2 not installed on build PC | Desktop uses null/console platform (quits after 3 frames) | Install SDL2 for interactive window |
| PSPSDK not verified on this PC | No successful EBOOT yet | **Do not claim PSP support** |
| Online login / DeltaDNA / ads in original | Not ported | Intentionally stubbed offline |

## Incomplete extractions logic

| Issue | Detail |
|-------|--------|
| Item numeric stats | Names/IDs from XML only. Food/water restore values in C++ are **placeholders** until extracted from C2 event actions / arrays. Marked `known_stats=false`. |
| Survival tick | **Sverka:** States `Every 5s` drains food/water by `*_current`; bleed `Every 1s` (−0.5 / −0.25 w/ metabolism). Cold Day/Night/Rain set on difficulty select. Clothing insulation / fire / bunker bonuses partially pending. |
| HUD `0.72` | Bar width scale in Update_Stats — **not** a cold formula. |
| Day/Night / Rain mapping | Globals `Day_Night_speed=720`, `Rain_speed=360` are used, but Construct timeline units ≠ seconds; phase advancement is approximate. |
| Weathercold globals init=0 | Extracted initial values are 0; runtime assignment in events not yet reverse-engineered. |
| Map generation | Full C2 `Map generation` / `Location loader` not ported — sector grid + collision stub only. |
| Bunker | RLE loader implemented; gameplay integration incomplete. |
| Firearms per-weapon base dmg | Still not fully extracted (bullet IV0 set at spawn is expression-driven). **Hit variance is sverka:** `roll_bullet_damage` = round(base×U(0.75,1.25)) from `Bullets_dmg_deal`. Melee Fist range **45** verified. |
| Vehicles, perks, quests | Groups identified; logic not ported. |
| Crafting recipes | Partial scaffold (`craft.cpp`); only a few inferred recipes — full `Craft_*` C2 groups not ported. |
| Airdrop / loot respawn | Timer constants wired (`60`/`1480`/`400`); spawn content logic incomplete. |
| Audio / full sprite rendering | Not started. |
| Exact starter inventory | Placeholder (beans, bandage, water bottle). |
| Plus / Reloaded content | Out of scope (baseline is 1.4.1). |

## Build notes

- Tests cover extracted balance constants, survival multipliers, inventory, combat damage globals, save roundtrip.
- Successful compilations and tests are recorded in `PORTING_STATUS.md` iteration log when run.

## Legal

Assets and game design belong to Bohemia Interactive / original authors. This tree is a technical porting effort from publicly mirrored browser files; redistribution of assets may be restricted.
