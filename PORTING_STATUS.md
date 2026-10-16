# Porting Status

Last updated: 2026-10-16  
Baseline: Mini DAYZ **1.4.1** (`original_docs/`)

Status legend: Не исследована | Исследована | Реализуется | Реализована, но не проверена | Протестирована | Сверена с оригиналом | Заблокирована внешним ограничением

| Subsystem | Status | Notes |
|-----------|--------|-------|
| Source inventory / architecture | Протестирована | PORTING_PLAN.md; `tools/*` |
| Balance globals extraction | **Сверена с оригиналом** | `balance_globals.hpp`; tests assert key values |
| Item name catalog (200) | Протестирована | `cpp/data/items.json` from `l_eng_items.xml` |
| Item numeric stats (food/dmg) | Исследована | Placeholders; not from C2 yet |
| Weapon ID table | Реализована, но не проверена | IDs from XML; **damage numbers are guesses** |
| Platform abstraction | Протестирована | Null + SDL stub; SDL2 missing on this PC |
| Game loop / app states | Протестирована | Menu/Playing/Dead/Paused |
| Player movement | Реализована, но не проверена | Speed constants not yet from C2 |
| Survival rates (difficulty) | **Сверена с оригиналом** | Multipliers only |
| Survival absolute drain | Реализуется | Approximate dt rates |
| Inventory | Протестирована | Add/use/save roundtrip |
| Combat zed/wolf damage | **Сверена с оригиналом** | Globals 9/6/13/10 |
| Dispersion globals | **Сверена с оригиналом** | max=25, pershot=5, run=10 |
| Zombie AI | Реализуется | Types + chase/melee stub |
| Animals | Реализуется | Wolf/deer/rabbit/crow/dog stub |
| Bots / bandits | Исследована | Groups mapped only |
| Vehicles | Исследована | Groups mapped only |
| World / map gen | Реализуется | 16×16 sectors + grid stub |
| Bunker tilemap loader | Протестирована | RLE decode; 225×74 when file found |
| Loot / respawn timers | Исследована | Constants known; logic TBD |
| Weather / day-night | Реализуется | Speeds from globals; phase approx |
| Crafting | Реализуется | Partial recipes; timers OK |
| Perks / quests | Исследована | Function names extracted |
| Loot/airdrop/zed timers | **Сверена с оригиналом** (constants) | Logic partial |
| UI / HUD | Реализуется | Debug bars |
| Audio | Не исследована | Catalogued only |
| Save / load | Протестирована | Binary blob roundtrip |
| Tutorial | Исследована | |
| Online / ads / analytics | Заблокирована внешним ограничением | |
| PSP EBOOT | Заблокирована внешним ограничением | Skeleton only; no PSPSDK build yet |
| Full parity | Реализуется | Ongoing |

## Automated tests (last run)

```
mdaz_tests.exe → ALL TESTS PASSED
```

Suites: balance (sverka), survival, inventory, combat, save, bunker, craft/timers.

## Iteration log

| Date | Work |
|------|------|
| 2026-10-16 | Cloned repo; C2 analysis; PORTING_*; C++ core; tests green; weapons/craft/timers; bunker 225×74; PSP Makefile stub |
