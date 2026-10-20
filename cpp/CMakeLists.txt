cmake_minimum_required(VERSION 3.16)
project(minidayz_cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MDAZ_DESKTOP "Build desktop SDL2 host" ON)
option(MDAZ_BUILD_TESTS "Build unit tests" ON)
option(MDAZ_PSP "Configure PSP toolchain hints (does not build EBOOT alone)" OFF)

set(MDAZ_CORE_SOURCES
  src/core/game.cpp
  src/core/game_tutorial.cpp
  src/core/game_audio.cpp
  src/core/game_pump.cpp
  src/core/game_chop.cpp
  src/core/time.cpp
  src/core/rng.cpp
  src/core/c2_session_globals.cpp
  src/player/player.cpp
  src/player/survival.cpp
  src/player/player_drop.cpp
  src/player/player_worn_visual.cpp
  src/world/world.cpp
  src/world/bunker.cpp
  src/world/spawn_global.cpp
  src/world/spawn_params.cpp
  src/world/loot.cpp
  src/world/mapgen.cpp
  src/world/poi_layout.cpp
  src/world/building_solid.cpp
  src/world/building_interior.cpp
  src/world/c2_mapgen.cpp
  src/world/map_props.cpp
  src/world/tutorial_layout.cpp
  src/world/die_anims.cpp
  src/world/cell_tilemaps.cpp
  src/world/vehicle.cpp
  src/world/campfire.cpp
  src/world/world_tilemaps.cpp
  src/world/layout_sprites.cpp
  src/world/char_anims.cpp
  src/inventory/item_db.cpp
  src/inventory/inventory.cpp
  src/inventory/inventory_helpers.cpp
  src/inventory/inventory_ui.cpp
  src/inventory/inventory_armor.cpp
  src/inventory/ammo.cpp
  src/inventory/clothing_containers.cpp
  src/inventory/craft.cpp
  src/combat/combat.cpp
  src/combat/weapons.cpp
  src/combat/c2_sfx.cpp
  src/combat/weapon_unlocks.cpp
  src/combat/bullet.cpp
  src/ai/zombie.cpp
  src/ai/animal.cpp
  src/ai/bandit.cpp
  src/weather/weather.cpp
  src/save/save_game.cpp
  src/ui/hud.cpp
  src/ui/psp_ui.cpp
  src/ui/screens.cpp
  src/ui/menu_camera.cpp
  src/ui/tutorial.cpp
  src/ui/item_sprites.cpp
  src/ui/ui_strings.cpp
  src/ui/quest.cpp
  src/platform/platform_null.cpp
  src/platform/audio_mixer.cpp
)

add_library(mdaz_core STATIC ${MDAZ_CORE_SOURCES})
target_include_directories(mdaz_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_definitions(mdaz_core PUBLIC
  MDAZ_VERSION_MAJOR=0
  MDAZ_VERSION_MINOR=1
  MDAZ_BASELINE_GAMEVERSION=140
)

if(MSVC)
  target_compile_options(mdaz_core PRIVATE /W4)
else()
  target_compile_options(mdaz_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- Desktop host ---
if(MDAZ_DESKTOP)
  add_executable(minidayz_desktop
    src/platform/desktop_main.cpp
    src/platform/platform_sdl.cpp
    src/platform/audio_sdl.cpp
  )
  target_link_libraries(minidayz_desktop PRIVATE mdaz_core)
  target_include_directories(minidayz_desktop PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
  )

  find_package(SDL2 QUIET)
  if(SDL2_FOUND)
    message(STATUS "SDL2 found — building windowed desktop host")
    # Keep our own main(); do not pull in SDL2main's WinMain wrapper.
    target_compile_definitions(minidayz_desktop PRIVATE MDAZ_HAS_SDL2=1 SDL_MAIN_HANDLED=1)
    target_link_libraries(minidayz_desktop PRIVATE SDL2::SDL2)
    if(WIN32 AND MINGW)
      # Shared MinGW SDL2 also needs these when using the import lib.
      target_link_libraries(minidayz_desktop PRIVATE imm32 winmm version setupapi ole32 oleaut32)
    endif()

    # Copy SDL2.dll next to the exe when available (shared builds).
    if(DEFINED SDL2_BINDIR AND EXISTS "${SDL2_BINDIR}/SDL2.dll")
      add_custom_command(TARGET minidayz_desktop POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${SDL2_BINDIR}/SDL2.dll"
          $<TARGET_FILE_DIR:minidayz_desktop>/SDL2.dll
      )
    elseif(EXISTS "${CMAKE_PREFIX_PATH}/bin/SDL2.dll")
      add_custom_command(TARGET minidayz_desktop POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_PREFIX_PATH}/bin/SDL2.dll"
          $<TARGET_FILE_DIR:minidayz_desktop>/SDL2.dll
      )
    endif()
  else()
    message(WARNING "SDL2 not found — desktop host will use null/software console fallback")
    target_compile_definitions(minidayz_desktop PRIVATE MDAZ_HAS_SDL2=0)
  endif()

  # Copy data next to binary
  add_custom_command(TARGET minidayz_desktop POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${CMAKE_CURRENT_SOURCE_DIR}/data
      $<TARGET_FILE_DIR:minidayz_desktop>/data
  )
endif()

# --- Tests (no SDL required) ---
if(MDAZ_BUILD_TESTS)
  add_executable(mdaz_tests
    tests/test_main.cpp
    tests/test_survival.cpp
    tests/test_inventory.cpp
    tests/test_combat.cpp
    tests/test_balance.cpp
    tests/test_weather.cpp
    tests/test_save.cpp
    tests/test_bunker.cpp
    tests/test_craft.cpp
    tests/test_systems.cpp
    tests/test_weapon_unlocks.cpp
    tests/test_spawn_global.cpp
    tests/test_spawn_params.cpp
    tests/test_c2_session_globals.cpp
    tests/test_tutorial.cpp
    tests/test_map_fidelity.cpp
    tests/test_player_visual.cpp
    tests/test_selection.cpp
    tests/test_fidelity2.cpp
    tests/test_audio.cpp
    tests/test_pump.cpp
    tests/test_chop.cpp
    tests/test_trees.cpp
  )
  target_link_libraries(mdaz_tests PRIVATE mdaz_core)
  add_custom_command(TARGET mdaz_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${CMAKE_CURRENT_SOURCE_DIR}/data
      $<TARGET_FILE_DIR:mdaz_tests>/data
  )
  enable_testing()
  add_test(NAME mdaz_tests COMMAND mdaz_tests)
endif()

if(MDAZ_PSP)
  message(STATUS "PSP mode: see platform/psp/Makefile and BUILDING.md")
endif()
