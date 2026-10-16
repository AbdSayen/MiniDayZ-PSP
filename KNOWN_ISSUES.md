# Known Issues & Limitations

Last updated: 2026-10-16 (desktop playable; PSP EBOOT still unverified)

## External / environment

| Issue | Impact | Status |
|-------|--------|--------|
| No Construct `.capx` source — only HTML5 export | Some editor-only data unavailable | Permanent for this repo |
| SDL2 previously missing | Desktop host now uses vendored `cpp/third_party/SDL2-2.30.9` | Working on this PC |
| PSPSDK not verified on this PC | No successful EBOOT yet | **Do not claim PSP support** |
| Online login / DeltaDNA / ads in original | Not ported | Intentionally stubbed offline |

## Incomplete extractions logic

| Issue | Detail |
|-------|--------|
| Item numeric stats | Names/IDs from XML only. Food/water restore values in C++ are **placeholders** until extracted from C2 event actions / arrays. Marked `known_stats=false`. |
| Survival tick | **Sverka:** States `Every 5s` drains food/water by `*_current`; bleed `Every 1s` (−0.5 / −0.25 w/ metabolism). Cold Day/Night/Rain set on difficulty select. Worn clothing insulation (warm pieces), fire/tent/bunker/indoor bonuses wired; full C2 gear curves TBD. |
| HUD `0.72` | Bar width scale in Update_Stats — **not** a cold formula. |
| Day/Night / Rain mapping | Globals `Day_Night_speed=720`, `Rain_speed=360` are used, but Construct timeline units ≠ seconds; phase advancement is approximate. |
| Weathercold globals init=0 | Extracted initial values are 0; runtime assignment in events not yet reverse-engineered. |
| Map generation | Full C2 `Map generation` / `Location loader` not ported — sector grid + collision stub only. |
| Bunker | RLE loader fixed (was empty due to JSON string parse bug); wall collision heuristic on tile IDs; dens/loot stub. |
| Firearms per-weapon base dmg | Partial: Fist **10**/range **45**, FNX IV0 candidate **23**, sword **15**. Other guns still placeholders. **Hit variance is sverka:** `roll_bullet_damage` = round(base×U(0.75,1.25)). Bleed start **sverka:** `random(3)>=1.5`. |
| Vehicles, perks, quests | Car/boat/bike drive + fuel/paddle/headlights/runover; quest chain kill→hospital→bunker→bandits→fish→police→day2. Perks unlock via play (snowborn/camel/hamster/regen/runner/metabolism/hunter/farmer). Full C2 groups incomplete. |
| Crafting recipes | Partial: bow/arrows/bag/pack/molotov/cook(+ruffe)/firekit/splint/torch/rope/fence — full `Craft_*` C2 groups not ported. |
| Airdrop / loot respawn | Timers + richer airdrop tables wired; exact Spawn_drop parity incomplete. |
| Audio / full sprite rendering | Procedural SDL cues (gun/hit/howl/airdrop); sprites not ported. |
| Traps | Bear trap + barbed wire + wood fence + claymore placeable stubs (not full C2 trap events). |
| Exact starter inventory | Expanded playable kit (medical, NVG, flare, craft mats, traps) — not original spawn table. |
| Crouch | Ctrl crouch quieter vs zeds (aggro mul 0.55) — not full C2 stealth group. |
| Plus / Reloaded content | Out of scope (baseline is 1.4.1). |

## Build notes

- Tests cover extracted balance constants, survival multipliers, inventory, combat damage globals, save roundtrip.
- Successful compilations and tests are recorded in `PORTING_STATUS.md` iteration log when run.

## Legal

Assets and game design belong to Bohemia Interactive / original authors. This tree is a technical porting effort from publicly mirrored browser files; redistribution of assets may be restricted.
