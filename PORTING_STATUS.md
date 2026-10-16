# Porting Status

Last updated: 2026-10-16 (autonomous session)  
Baseline: Mini DAYZ **1.4.1** (`original_docs/`)

| Subsystem | Status | Notes |
|-----------|--------|-------|
| Source / architecture | Протестирована | Construct 2 export |
| Balance globals | **Сверена с оригиналом** | `balance_globals.hpp` |
| Survival tick (States) | **Сверена с оригиналом** | Every 5s / bleed 1s / cold on difficulty |
| Item catalog (200) | Протестирована | |
| Item consumable numbers | Исследована | Blood/saline/morphine/vitamins wired; most food still placeholders |
| Map fog / visited | Реализуется | 16×16 sector visited + minimap fog |
| Weapon base damage | Исследована | Fist **10**/range **45**; FNX IV0 candidate **23**; sword **15** |
| Bullet hit variance | **Сверена с оригиналом** | ±25% (`roll_bullet_damage`) |
| Fist melee range | **Сверена с оригиналом** | 45 |
| Combat zed/wolf dmg | **Сверена с оригиналом** | 9/6/13/10 |
| Player_get_hit bleed | **Сверена с оригиналом** | `random(3) >= 1.5` ≈ 50%; Bleed timer 60s |
| Dispersion | **Сверена с оригиналом** | Persistent `DispersionState` on Game |
| Timers loot/zed/airdrop | **Сверена с оригиналом** (constants) | |
| Loot piles | Реализуется | Hospital/gas/civ/military POI tables; airdrop |
| Zombie AI types | Реализуется | Buried ambush, screamer alert, army shooter bullets |
| Animals | Реализуется | Meat drops; 90s respawn; wolf packs |
| Bandits / bots | Реализуется | Melee + gun stub; BOT_RESPAWN_TIME wired |
| Vehicles | Реализуется | Car + boat mount; fuel / paddle stub |
| Quests | Реализуется | Kill-10-zeds starter objective |
| Map generation | Реализуется | POI scaffold 16×16 + bunker enter + collision |
| Bunker loader | Протестирована | 225×74; enter/exit + loot/zeds dens |
| Craft | Реализуется | Rags/arrows/bow/firekit/fishing rod/cook; R key |
| Save | Протестирована | MDAZ v3 + visited fog bits; v2 load OK |
| Audio | Реализуется | Procedural SDL `SdlAudio` (gunshot/melee/hit/pickup) |
| Campfire | Реализуется | Place kit (T), warmth, cook requires fire |
| Desktop SDL host | Протестирована | Vendored SDL2 2.30.9; 480×272 logical @ 2× |
| Platform / PSP | Заблокирована внешним ограничением | PSPSDK EBOOT not verified |
| Full parity | Реализуется | |

## How to run (desktop)

```
cd cpp/build
cmake -S .. -B . -G "MinGW Makefiles" ^
  -DCMAKE_PREFIX_PATH=../third_party/SDL2-2.30.9/x86_64-w64-mingw32
mingw32-make -j4
.\minidayz_desktop.exe
```

Controls: WASD move, Shift run, Z/Space attack, E loot/bunker, G drop, V reload, C use, R craft, 1–8 / Q·X / wheel hotbar, P pause, Esc menu/quit.

## Tests

```
mdaz_tests.exe → ALL TESTS PASSED
```

Includes survival sverka, fist dmg 10 / range 45, bullet ±25%, craft, bunker load, timers.

## Iteration log

| Date | Work |
|------|------|
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
