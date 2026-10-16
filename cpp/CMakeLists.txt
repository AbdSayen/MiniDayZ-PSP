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
  src/core/time.cpp
  src/core/rng.cpp
  src/player/player.cpp
  src/player/survival.cpp
  src/world/world.cpp
  src/world/bunker.cpp
  src/inventory/item_db.cpp
  src/inventory/inventory.cpp
  src/inventory/craft.cpp
  src/combat/combat.cpp
  src/combat/weapons.cpp
  src/ai/zombie.cpp
  src/ai/animal.cpp
  src/weather/weather.cpp
  src/save/save_game.cpp
  src/ui/hud.cpp
  src/platform/platform_null.cpp
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
  )
  target_link_libraries(minidayz_desktop PRIVATE mdaz_core)

  find_package(SDL2 QUIET)
  if(SDL2_FOUND)
    target_compile_definitions(minidayz_desktop PRIVATE MDAZ_HAS_SDL2=1)
    target_link_libraries(minidayz_desktop PRIVATE SDL2::SDL2)
    if(TARGET SDL2::SDL2main)
      target_link_libraries(minidayz_desktop PRIVATE SDL2::SDL2main)
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
    tests/test_save.cpp
    tests/test_bunker.cpp
    tests/test_craft.cpp
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
