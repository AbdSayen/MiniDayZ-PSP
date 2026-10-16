# Building Mini DAYZ C++

## Prerequisites (desktop host — verified path on this project)

- CMake ≥ 3.16
- C++11 compiler (MinGW g++ / MSVC / Clang)
- Optional: SDL2 (for windowed host). Without SDL2 the desktop binary uses a null/console platform.

### Windows (MinGW + bundled SDL2)

SDL2 MinGW devel is vendored under `cpp/third_party/SDL2-2.30.9/` (downloaded once).

```bat
cd cpp
set SDL=C:\Users\abdur\Desktop\MiniDayZ\cpp\third_party\SDL2-2.30.9\x86_64-w64-mingw32
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%SDL%
cmake --build build -j
copy /Y third_party\SDL2-2.30.9\x86_64-w64-mingw32\bin\SDL2.dll build\SDL2.dll
cd build
mdaz_tests.exe
minidayz_desktop.exe
```

Data files are copied next to the binaries from `cpp/data/` (items.json, bunker_json).  
`SDL2.dll` must sit next to `minidayz_desktop.exe`.

### Controls (desktop)

| Key | Action |
|-----|--------|
| WASD / arrows | Move |
| Shift | Run |
| Z / Space | Attack / confirm |
| C | Use first consumable |
| E | Interact (reserved) |
| R | Reload (reserved) |
| P | Pause |
| Enter | New game from menu |
| Esc | Quit |

## Analysis tools (Node.js)

```bat
node tools/analyze_c2.js original_docs/data.js
node tools/extract_globals_and_groups.js original_docs/data.js
node tools/extract_group_tree.js original_docs/data.js
node tools/extract_items.js
node tools/gen_balance_hpp.js
```

Outputs land in `cpp/docs_gen/` and `cpp/include/mdaz/balance_globals.hpp`.

## PSP (not verified on this machine)

1. Install [pspdev](https://github.com/pspdev/pspdev) so `psp-config` and `psp-gcc` are on PATH.
2. From `cpp/platform/psp/`:

```sh
make
```

3. Copy `EBOOT.PBP` + `data/` to `ms0:/PSP/GAME/MINIDAYZ/`.

**Do not claim PSP support until `make` produces a runnable EBOOT and it is tested on hardware or PPSSPP.**

See `cpp/platform/psp/Makefile` and `KNOWN_ISSUES.md`.

## Baseline

Gameplay numbers are taken from **original_docs** Mini DAYZ **1.4.1** (`gameversion=140`). Plus/Reloaded are not the port baseline.
