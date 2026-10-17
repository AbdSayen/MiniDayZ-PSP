# Porting Status

Last updated: 2026-10-17 (autonomous session)  
Baseline: Mini DAYZ **1.4.1** (`original_docs/`)  
Executable game: **C++ only** (`minidayz_desktop.exe`) — does not run `c2runtime.js`.

| Subsystem | Status | Notes |
|-----------|--------|-------|
| Source / architecture | Протестирована | Construct 2 export mapped; see `PORTING_PLAN.md` |
| Balance globals | **Сверена с оригиналом** | `balance_globals.hpp` |
| Survival tick (States) | **Сверена с оригиналом** | Every 5s / bleed 1s / cold on difficulty |
| Item catalog (200) | Протестирована | JSON + icons for all 200 IDs |
| Item consumable numbers | Исследована | Partial known_stats; drink IDs fixed (54/55 water/canteen; 10=Kvas) |
| Item ground/hotbar icons | Реализуется | `item_sprites.cpp` full ID map |
| Map fog / visited | Реализуется | 16×16 sector visited + minimap fog |
| Weapon defs (catalog) | Реализуется | IDs 151–187 + melee; most dmg **placeholder** |
| Weapon base damage | Исследована | Fist **10**/45; FNX **23**; sword **15** sverka |
| Bullet hit variance | **Сверена с оригиналом** | ±25% |
| Fist melee range | **Сверена с оригиналом** | 45 |
| Combat zed/wolf dmg | **Сверена с оригиналом** | Regular 9/6/13/10; Novice 6/4/9/6; Veteran 10/8/15/10 |
| Player_get_hit bleed | **Сверена с оригиналом** | ≈50%; Bleed 60s |
| Dispersion | **Сверена с оригиналом** | Persistent state |
| Timers loot/zed/airdrop | **Сверена с оригиналом** (constants) | |
| Loot piles | Реализуется | Typed POI (incl. fire/church) + weighted Spawn_drop + airdrop |
| Zombie AI types | Реализуется | Buried/screamer/army/tank/… |
| Animals | Реализуется | Howl/stampede/crow/deer SFX |
| Bandits / bots | Реализуется | Melee+gun; markers |
| Vehicles | Реализуется | Car/boat/bike + 4-slot trunk stash (G) |
| Quests | Реализуется | Chain …church→military→helicrash→day2; HUD panel |
| Map generation | Реализуется | POI sectors; atlas ground (not C2 RLE map) |
| Bunker loader | Протестирована | RLE 225×74; tile atlas draw |
| Craft | Реализуется | Core + Craft_21/22/3 firekit paths; incomplete vs full C2 |
| Save | Протестирована | MDAZ v13 (+bunker_looted/melee mul) |
| Audio | Реализуется | Original OGG tags + procedural fallback |
| Sprites / VFX | Реализуется | C2 sheet crops; muzzle/shell/blood/fish/grenade |
| HUD / menu | Реализуется | GUI crops; bag icons; night_overlay; amenu_bg |
| Campfire | Реализуется | Place/warmth/cook |
| Desktop SDL host | Протестирована | SDL2 2.30.9; 480×272 |
| Platform / PSP | Заблокирована внешним ограничением | Makefile present; EBOOT **not** verified |
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
