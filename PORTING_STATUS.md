# Porting Status

Last updated: 2026-10-16 (continued session)  
Baseline: Mini DAYZ **1.4.1** (`original_docs/`)

| Subsystem | Status | Notes |
|-----------|--------|-------|
| Source / architecture | Протестирована | Construct 2 export |
| Balance globals | **Сверена с оригиналом** | `balance_globals.hpp` |
| Survival tick (States) | **Сверена с оригиналом** | Every 5s / bleed 1s / cold on difficulty |
| Item catalog (200) | Протестирована | |
| Item consumable numbers | Исследована | Placeholders |
| Weapon base damage | Исследована | Spawn IV0 still expression-driven |
| Bullet hit variance | **Сверена с оригиналом** | ±25% (`roll_bullet_damage`) |
| Fist melee range | **Сверена с оригиналом** | 45 |
| Combat zed/wolf dmg | **Сверена с оригиналом** | 9/6/13/10 |
| Dispersion | **Сверена с оригиналом** | |
| Timers loot/zed/airdrop | **Сверена с оригиналом** (constants) | |
| Loot piles | Реализуется | Random/airdrop tables partial |
| Zombie AI types | Реализуется | All 7 types spawnable |
| Animals | Реализуется | |
| Map generation | Реализуется | POI scaffold 16×16 |
| Bunker loader | Протестирована | 225×74 |
| Craft | Реализуется | Partial recipes |
| Save | Протестирована | |
| Platform / PSP | Заблокирована внешним ограничением | No SDL2 / no PSPSDK EBOOT yet |
| Full parity | Реализуется | |

## Tests

```
mdaz_tests.exe → ALL TESTS PASSED
```

Includes survival sverka: Every-5s food drain, starve −1 blood/5s, bleed −0.5/s, metabolism −0.25/s, hamster perk, cold globals.

## Iteration log

| Date | Work |
|------|------|
| 2026-10-16 | Initial C++ scaffold + docs |
| 2026-10-16 | ACE-decoded `States`/`Update_Stats`; rewrote survival; loot; mapgen; zed types |
| 2026-10-16 | Bullet variance + Fist range 45; continued without pause |
