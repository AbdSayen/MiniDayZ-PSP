# Mini DAYZ → C++ / PSP Porting Plan

**Baseline source:** `original_docs/` — Bohemia Interactive Mini DAYZ **1.4.1** (Construct 2 HTML5 export)  
**Also present (not baseline):** `docs/` Plus 1.2, `reloaded_docs/` Reloaded 1.1  
**Verified:** 2026-10-16 by scanning the GitHub tree at https://github.com/NextDev65/MiniDayZ

---

## 1. What the original actually is

| Fact | Evidence |
|------|----------|
| Engine | Construct 2 (`c2runtime.js` + `data.js`) |
| Language | Event sheets (visual scripting), not hand-written JS game logic |
| Version | `project[16] == "1.4.1"`, global `gameversion = 140` |
| Resolution | 1024×768 landscape |
| Package | Cordova/Intel XDK wrapper (`config.json` id `com.bistudio.minidayz`) |
| Author field | Mike Azovskikh / cannedbits |

**There is no separate readable C++/C#/Java game source.** All gameplay is encoded inside `original_docs/data.js` (~6.9 MB JSON project dump) interpreted by `c2runtime.js`.

### Variants in this repo

| Folder | Role | Notes |
|--------|------|-------|
| `original_docs/` | **Port baseline** | 200 items in `l_eng_items.xml`, version 1.4.1 |
| `docs/` | Plus 1.2 mod | Extra items/weapons; larger `data.js` |
| `reloaded_docs/` | Reloaded 1.1 mod | Separate balance/content |

---

## 2. File map (verified)

```
MiniDayZ/
├── original_docs/          # BASELINE
│   ├── data.js             # Project: objects, layouts, events, audio table
│   ├── c2runtime.js        # Construct 2 runtime + plugins
│   ├── index.html          # Canvas 1024×768 entry
│   ├── images/             # 1137 spritesheets/frames
│   ├── media/              # 207 audio files (ogg/m4a pairs mostly)
│   ├── bunker_json         # C2 tilemap RLE for bunker (225×74)
│   ├── l_*_items.xml       # Item display names by language
│   ├── l_*_ui.xml          # UI strings
│   ├── l_*_log.xml         # Log/death strings
│   └── config.json         # Cordova app metadata
├── docs/                   # Plus (not baseline)
├── reloaded_docs/          # Reloaded (not baseline)
├── tools/                  # Analysis/extraction scripts (this port)
└── cpp/                    # C++ rewrite (this port)
```

### Construct 2 `data.js` layout (confirmed)

| Index | Content |
|-------|---------|
| 1 | First layout name: `Loading` |
| 2 | 20 plugin slots |
| 3 | **947 object types** (`t0`…), mostly Sprites |
| 4 | 38 families (member index lists) |
| 5 | **7 layouts:** GUI_layout, Polygon, Map (17850×20000), Tutorial, Menu, Login, Loading |
| 6 | **6 event sheets:** Game_events, Menu_Events, Login_events, Loading_events, Everywere, Tutorial_events |
| 7 | 415 audio file entries |
| 10–11 | 1024×768 |
| 16 | `"1.4.1"` |
| 26 | `"Mini DAYZ"` |

### Plugins present in runtime (names)

AJAX, Arr, Audio, Browser, Dictionary, Function, Keyboard, List, LocalStorage, Mouse, NinePatch, Particles, Sprite, Spritefont2, Text, TextBox, TiledBg, Tilemap, Touch, XML.

---

## 3. Systems map (from event group tree)

Extracted from `Game_events` groups (`tools/extract_group_tree.js` → `cpp/docs_gen/group_tree.json`):

### Core loop / meta
- Start events, SEED RUNS, Timeline, Analytics, saving / sav, Android exit
- Difficulty: Novice / Regular / Veteran (text global + rate multipliers)

### World
- Map generation, Location loader, Bunker_generate (`bunker_json`)
- Buildings and obstacles, transparency, barbed wire/barricades
- Level change, Map size globals (16×16 sector grid over 17850×20000 layout)

### Player
- Player stats, colchecker, Animations, Hands Switch
- Survival globals: starving / thirsty / weathercold / regeneration (per difficulty)
- Temperature: `Player_temperature`, `Weather_temperature`, Rain/Night/Day cold
- Bleeding, blood bar UI, torch, NVG, adrenaline

### Inventory / items / crafting
- Using / picking / dropping inventory & ground items
- Pickpile UI, drag, craft possibility global `GUI_craft_possible`
- 200 localized item IDs (food, ammo, medical, tools, weapons, clothing, …)

### Combat
- Aiming, Aim_lines, Firearm shooting, Melee fight, Throwing
- Dispersion globals, reload timers, Fight_mode
- Bullets parameters, obstacle hits, range falloff
- Player damaging, God_mode (debug)

### AI / NPCs
- Zombies: Normal, Buried, Army, Fast, Army Shooters, Screamer, Tank
- Damage constants (globals): Zed_dmg=9, Zed_fast_dmg=6, Zed_army_dmg=13, Wolf_dmg=10
- Bots: punchers, shooters, teammates, bandits
- Animals: Crow, Wolf, Deer, Rabbit, Dog
- Spawn/respawn: ZED_RESPAWN_TIME=60, BOT_RESPAWN_TIME=10, LOOT_RESPAWN_TIME=1480

### Vehicles
- Drivable vehicles, collisions, UI (UAZ, Volga, truck refs in assets/audio)

### Environment
- Day&Night (`Day_Night_speed=720`), Rain (`Rain_speed=360`)
- Light: flare, molotov, grenade, fireplace
- Airdrop timer/recharge

### Progression / UI
- Perks (meleedef, meleeatk, farmer, metabolism, hunter, …)
- Quests (`Q_status`, `Q_task_id`, …)
- Achievements, notebook/pad, tutorial guides
- Menus: character unlocks, weapon unlocks, options

### Persistence
- LocalStorage-backed saves (`sav`, `saving`, `fromsav`, `gamesav_tutorial`)
- Death log fields

---

## 4. Data vs code

| Data | Location | Port strategy |
|------|----------|---------------|
| Item display names | `l_*_items.xml` | Load XML / ship JSON |
| UI / log strings | `l_*_ui.xml`, `l_*_log.xml` | Same |
| Balance globals (init values) | `Game_events` var decls | `cpp/include/mdaz/balance_globals.hpp` |
| Item combat/food numbers | Inside event actions / arrays (not plain XML) | Extract incrementally; document gaps |
| Map / bunker tiles | Layout instances + `bunker_json` | Tilemap loader |
| Sprites | `images/*.png` | Atlas/stream for PSP VRAM |
| Audio | `media/*` | Convert to ATRAC3/WAV for PSP later |
| Event logic | Nested C2 events (~hundreds of groups) | Hand-port by group; no JS shell |

---

## 5. Technical stack (PSP-aware)

| Concern | Choice | Rationale |
|---------|--------|-----------|
| Language | C++11 subset (no RTTI/exceptions in hot paths) | Works with modern MinGW; PSPSDK GCC can use `-std=c++11` with care |
| Desktop host | SDL2 + OpenGL/software fallback | Available on this machine; validates logic |
| PSP target | PSPSDK (`psp-gcc`), GU / sceDisplay, sceCtrl, scePower | Standard homebrew path |
| Memory | Fixed pools, streaming textures, no unbounded `new` in frame | PSP ~24–32 MB user RAM practical budget |
| Screen | Logical 480×272 (PSP); scale from 1024×768 UI coords | Native PSP framebuffer |
| Input | Abstraction: keyboard/gamepad desktop → PSP buttons | Analog = left stick |
| Audio | Stub → SDL_mixer desktop; sceAudio PSP later | Don’t block logic on audio |
| Dependencies | Minimal: SDL2 (desktop only), no Boost | Keep PSP link light |

**Claim status:** Desktop build is the verification target. PSP EBOOT is prepared structurally; **not claimed working until PSPSDK build succeeds.**

---

## 6. Port sequence

1. **Tooling** — parsers for `data.js`, globals, items, group tree ✅  
2. **Platform layer** — time, input, render stub, file I/O  
3. **Game loop + state machine** — Loading → Menu → Game → Dead → Save  
4. **Balance + player survival** — rates from globals, difficulty tables  
5. **Inventory + item DB** — 200 items from XML; stats as extracted  
6. **World / map streaming stub** — sector grid 16×16, collisions AABB  
7. **Combat** — melee + firearms + dispersion constants  
8. **AI** — zed types, wolf, damage tables  
9. **Vehicles, weather, airdrop, perks, quests, UI, save**  
10. **Asset pipeline + PSP makefile**  
11. **Behavioral tests vs extracted constants**

---

## 7. Risks

| Risk | Mitigation |
|------|------------|
| Event sheets are huge / opaque opcodes | Port by named group; extract numbers; never invent balance |
| Item stats not in XML | Dig Array/Dictionary/actions; mark UNKNOWN until found |
| No Construct source `.capx` | Work only from export; some editor-only data gone |
| Map 17850×20000 too big for PSP RAM | Sector streaming; don’t load full bitmap |
| Online login / DeltaDNA / ads | Stub offline; out of scope for PSP fidelity |
| Plus vs Original divergence | Baseline = 1.4.1 only unless user asks for Plus |
| Copyrighted BI assets | Keep assets local for personal port; don’t redistribute blindly |

---

## 8. Success criteria

- C++ binary runs game loop without executing `c2runtime.js` / interpreting JS events as the game.
- Survival/combat/inventory numbers match extracted 1.4.1 globals where implemented.
- Tests assert those numbers.
- PSP build instructions documented; desktop host compilable on this PC.
- Status docs stay current; no system marked “сверена” without a real comparison.
