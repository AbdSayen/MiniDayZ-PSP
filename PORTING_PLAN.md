# Mini DAYZ → C++ / PSP Porting Plan

**Baseline source:** `original_docs/` — Bohemia Interactive Mini DAYZ **1.4.1** (Construct 2 HTML5 export)  
**Also present (not baseline):** `docs/` Plus 1.2, `reloaded_docs/` Reloaded 1.1  
**Verified on disk:** 2026-10-17 — `original_docs/data.js` ~6.72 MB; `images/` 1137 PNGs; `media/` 207 audio; desktop `minidayz_desktop.exe` + `mdaz_tests.exe` build and pass.  
**Parity snapshot (audit 2026-10-17):** C2 function **names** **175/175**, gameplay globals **242/242**, Craft_* **11/11**, perk_* **15/15** referenced in C++ (name coverage ≠ full behavior). Executable path is C++ only (does not run `c2runtime.js`). PSP EBOOT **not** verified.

---

## 1. What the original actually is

| Fact | Evidence |
|------|----------|
| Engine | Construct 2 (`c2runtime.js` + `data.js`) |
| Language | Event sheets (visual scripting), not hand-written JS game logic |
| Version | `project[16] == "1.4.1"`, global `gameversion = 140` |
| Resolution | 1024×768 landscape (logical PSP host: 480×272) |
| Package | Cordova/Intel XDK wrapper (`config.json` id `com.bistudio.minidayz`) |

**There is no separate readable C++/C#/Java game source.** All gameplay is encoded inside `original_docs/data.js` interpreted by `c2runtime.js`.

### Variants in this repo

| Folder | Role | Notes |
|--------|------|-------|
| `original_docs/` | **Port baseline** | 200 items in `l_eng_items.xml`, version 1.4.1 |
| `docs/` | Plus 1.2 mod | Out of scope unless requested |
| `reloaded_docs/` | Reloaded 1.1 mod | Out of scope |

---

## 2. File map (verified)

```
MiniDayZ/
├── original_docs/          # BASELINE 1.4.1 export
│   ├── data.js             # Objects, layouts, events, audio table
│   ├── c2runtime.js        # Construct 2 runtime
│   ├── images/             # 1137 spritesheets
│   ├── media/              # 207 ogg/m4a
│   ├── bunker_json         # Bunker tilemap RLE (225×74)
│   └── l_*_{items,ui,log}.xml
├── tools/                  # Node extractors → cpp/docs_gen/
├── cpp/                    # C++ rewrite (does NOT run c2runtime.js)
│   ├── include/mdaz/       # Public headers
│   ├── src/{core,player,inventory,combat,ai,world,ui,save,weather,platform}/
│   ├── data/               # items.json, bunker_json (shipped with binary)
│   ├── docs_gen/           # Extracted analysis (not runtime)
│   ├── platform/psp/       # PSPSDK Makefile + stubs (EBOOT unverified)
│   ├── tests/              # mdaz_tests
│   └── third_party/SDL2-2.30.9/
└── PORTING_*.md / BUILDING.md / KNOWN_ISSUES.md
```

### Construct 2 `data.js` (confirmed)

| Index | Content |
|-------|---------|
| 3 | ~947 object types |
| 5 | 7 layouts (Map 17850×20000, GUI, Menu, …) |
| 6 | 6 event sheets (`Game_events` is the gameplay core) |
| 16 | `"1.4.1"` |

---

## 3. Systems map (from C2 group tree + port modules)

| Original area | C++ module(s) | Notes |
|---------------|---------------|-------|
| Game loop / difficulty / save | `game.cpp`, `save_game.cpp`, `timers.cpp` | Menu→Play→Pause→Dead |
| Survival (States / Update_Stats) | `survival.cpp`, `balance_globals.hpp` | 5s drains, bleed 1s — sverka where marked |
| Inventory / craft / items | `inventory.cpp`, `craft.cpp`, `item_db.cpp`, `item_sprites.cpp` | 200 IDs; many restores still placeholders |
| Combat / bullets / weapons | `combat.cpp`, `bullet.cpp`, `weapons.cpp` | FNX 23 / Fist 10·45 / ±25% / bleed chance sverka |
| Zeds / animals / bandits | `zombie.cpp`, `animal.cpp`, `bandit.cpp` | Types + packs; not full C2 AI trees |
| Map / bunker / loot / vehicles | `mapgen.cpp`, `world.cpp`, `bunker.cpp`, `loot.cpp`, `vehicle.cpp`, `campfire.cpp` | Sector POIs; bunker RLE; drive stubs |
| Weather / quests / HUD / audio | `weather.cpp`, `quest.cpp`, `hud.cpp`, `audio_sdl.cpp` | OGG tags + procedural fallbacks |
| Platform | `platform_sdl.cpp`, `platform_null.cpp`, `platform/psp/*` | Desktop verified; PSP structural only |

Full event-group list: `cpp/docs_gen/group_tree.json` (from `tools/extract_group_tree.js`).

---

## 4. Data vs code

| Data | Location | Port strategy |
|------|----------|---------------|
| Item names | `l_*_items.xml` → `cpp/data/items.json` | Loaded at runtime |
| Balance globals | C2 var decls → `balance_globals.hpp` | Generated / hand-checked |
| Item food/water/heal | Event actions (opaque) | Incremental extract; `known_stats` flag |
| Firearm base dmg | Bullet IV0 / New_bullet | FNX/Fist/sword confirmed; others placeholder |
| Bunker tiles | `bunker_json` RLE | `BunkerMap::load_from_*` |
| Sprites / audio | `original_docs/images|media` | Desktop loads via stb; PSP pipeline TBD |

---

## 5. Technical stack (PSP-aware)

| Concern | Choice |
|---------|--------|
| Language | C++11 |
| Desktop host | SDL2 2.30.9 (vendored), logical 480×272 |
| PSP | `cpp/platform/psp/Makefile` + `psp-gcc` — **EBOOT not verified on this PC** |
| Memory | Prefer fixed pools; avoid unbounded per-frame `new` |
| Claim | Desktop playable + tests. **Do not claim PSP support until EBOOT runs.** |

---

## 6. Port sequence (status)

1. Tooling / analysis ✅  
2. Platform + game loop ✅ (desktop)  
3. Survival / inventory / combat constants ✅ (partial sverka)  
4. World / AI / vehicles / quests / save ✅ (functional, incomplete vs C2)  
5. Original atlases / HUD / audio tags ✅ (presentation parity improving)  
6. Remaining (behavior, not name coverage): exact gun IV0s beyond Fist/FNX/sword; mag capacities; NPC max HP/speed; Buildings_base prefab XY; full AI trees; Craft_* Spawn_drop completeness; PSP EBOOT  
7. Continuous behavioral tests vs extracted numbers ✅ (`mdaz_tests`)  
8. Save format: MDAZ **v18** loot piles; older v2–v17 load OK

---

## 7. Risks

| Risk | Mitigation |
|------|------------|
| Opaque C2 opcodes | Port by named group; never invent balance as “sverka” |
| No `.capx` | Work only from export |
| Map too large for PSP RAM | Sector streaming; no full-map bitmap |
| Online / ads / login | Offline stubs only |
| Copyrighted assets | Local personal port; redistribution restricted |

---

## 8. Success criteria

- C++ binary does **not** execute `c2runtime.js` as the game.
- Extracted 1.4.1 constants match where status says **Сверена с оригиналом**.
- `mdaz_tests` run and pass when claiming tests.
- Docs (`PORTING_STATUS`, `KNOWN_ISSUES`, `BUILDING`) stay current.
- PSP: instructions only until a real EBOOT build succeeds.
