# MiniDayZ (Plus)

###### The Browser (and PC) port of Bohemia Interactive's mobile game

[![Survivor](./survivor.png)](https://nextdev56.github.io/MiniDayZ/)

## Notes
* Current build is **Plus 1.2**
* You can change controls from stick to tap/draw (better for PC)
* Try to F11/Fullscreen before the game loads

___
## [Original 1.4.1](https://raw.githack.com/NextDev65/MiniDayZ/main/original_docs/index.html) by [Bohemia Interactive](https://minidayz.com/home)
  * https://minidayz.com/blog/mini-dayz-browser-version-will-no-longer-be-supported (bohemia stopped hosting)
## Mods
  * ### [Reloaded 1.1](https://raw.githack.com/NextDev65/MiniDayZ/main/reloaded_docs/index.html) by [Kev3232](https://discord.gg/CSktjeQWtC)
  * ### [Plus 1.2](https://nextdev65.github.io/MiniDayZ/) by [Altero](https://discord.gg/CSktjeQWtC)
    * Exported to apk by [meterpreter](https://github.com/MeterPreter57/)

---

## C++ rewrite (in progress)

Native C++11 port of **Original 1.4.1** under `cpp/` (does **not** run Construct 2 / `c2runtime.js`).

| Doc | Purpose |
|-----|---------|
| [`PORTING_PLAN.md`](PORTING_PLAN.md) | Architecture, file map, sequence |
| [`PORTING_STATUS.md`](PORTING_STATUS.md) | Per-system status |
| [`BUILDING.md`](BUILDING.md) | Desktop build / controls |
| [`KNOWN_ISSUES.md`](KNOWN_ISSUES.md) | Gaps; **PSP EBOOT not verified** |

```bat
cd cpp
cmake -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=%CD%\third_party\SDL2-2.30.9\x86_64-w64-mingw32
cmake --build build -j
build\mdaz_tests.exe
build\minidayz_desktop.exe
```
