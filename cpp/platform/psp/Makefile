# PSP homebrew build for Mini DAYZ C++
# Requires PSPDEV / PSPSDK: https://github.com/pspdev/pspdev
#
# Build (WSL example):
#   export PSPDEV=/root/pspdev
#   export PATH="$PSPDEV/bin:$PATH"
#   cd cpp/platform/psp && make
#
# Do not claim device support until EBOOT runs on PSP/PPSSPP.

TARGET = minidayz

SRC_ROOT = ../..
SRC = $(SRC_ROOT)/src

OBJS = \
  $(SRC)/core/game.o $(SRC)/core/game_tutorial.o \
  $(SRC)/core/time.o \
  $(SRC)/core/rng.o \
  $(SRC)/core/c2_session_globals.o \
  $(SRC)/player/player.o \
  $(SRC)/player/survival.o \
  $(SRC)/player/player_drop.o \
  $(SRC)/player/player_worn_visual.o \
  $(SRC)/world/world.o \
  $(SRC)/world/bunker.o \
  $(SRC)/world/spawn_global.o \
  $(SRC)/world/spawn_params.o \
  $(SRC)/world/loot.o \
  $(SRC)/world/mapgen.o \
  $(SRC)/world/poi_layout.o \
  $(SRC)/world/building_solid.o \
  $(SRC)/world/building_interior.o \
  $(SRC)/world/c2_mapgen.o \
  $(SRC)/world/map_props.o $(SRC)/world/cell_tilemaps.o $(SRC)/world/tutorial_layout.o $(SRC)/world/die_anims.o \
  $(SRC)/world/vehicle.o \
  $(SRC)/world/campfire.o \
  $(SRC)/world/world_tilemaps.o \
  $(SRC)/world/layout_sprites.o \
  $(SRC)/world/char_anims.o \
  $(SRC)/inventory/item_db.o \
  $(SRC)/inventory/inventory.o \
  $(SRC)/inventory/inventory_helpers.o \
  $(SRC)/inventory/inventory_ui.o \
  $(SRC)/inventory/inventory_armor.o \
  $(SRC)/inventory/ammo.o \
  $(SRC)/inventory/clothing_containers.o \
  $(SRC)/inventory/craft.o \
  $(SRC)/combat/combat.o \
  $(SRC)/combat/weapons.o \
  $(SRC)/combat/weapon_unlocks.o \
  $(SRC)/combat/bullet.o \
  $(SRC)/ai/zombie.o \
  $(SRC)/ai/animal.o \
  $(SRC)/ai/bandit.o \
  $(SRC)/weather/weather.o \
  $(SRC)/save/save_game.o \
  $(SRC)/ui/hud.o \
  $(SRC)/ui/psp_ui.o \
  $(SRC)/ui/screens.o \
  $(SRC)/ui/menu_camera.o \
  $(SRC)/ui/tutorial.o \
  $(SRC)/ui/item_sprites.o \
  $(SRC)/ui/ui_strings.o \
  $(SRC)/ui/quest.o \
  main_psp.o \
  platform_psp.o

# build.mak prepends -I to each INCDIR entry — do NOT also put -I$(INCDIR) in CFLAGS.
INCDIR = $(SRC_ROOT)/include $(SRC_ROOT)/third_party/stb
CFLAGS = -O2 -G0 -Wall -DMDAZ_PSP=1 -MMD -MP
CXXFLAGS = -fno-exceptions -fno-rtti -std=c++11
ASFLAGS =

LIBDIR =
LIBS = -lpspgum -lpspgu -lm -lpspctrl -lpspdisplay -lpspge -lpsppower -lpsprtc -lstdc++
LDFLAGS =

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Mini DAYZ C++
# Expand user memory partition (helps 2000/3000 / PPSSPP).

PSPSDK := $(shell psp-config --pspsdk-path)

ifeq ($(PSPSDK),)
$(error PSPSDK not found — install pspdev and ensure psp-config is on PATH)
endif

include $(PSPSDK)/lib/build.mak
# Header dependencies (stale objects after a header change broke the Game layout, 2026-10-20).
-include $(OBJS:.o=.d)
