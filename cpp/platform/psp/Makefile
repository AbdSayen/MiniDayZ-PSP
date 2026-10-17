# PSP homebrew skeleton for Mini DAYZ C++
# Requires PSPDEV / PSPSDK: https://github.com/pspdev/pspdev
#
# This Makefile does NOT claim a successful EBOOT until built on a machine
# with psp-gcc. It documents the intended link.

TARGET = minidayz
OBJS = \
  ../../src/core/game.o \
  ../../src/core/time.o \
  ../../src/core/rng.o \
  ../../src/player/player.o \
  ../../src/player/survival.o \
  ../../src/world/world.o \
  ../../src/world/bunker.o \
  ../../src/world/loot.o \
  ../../src/world/mapgen.o \
  ../../src/world/vehicle.o \
  ../../src/world/campfire.o \
  ../../src/inventory/item_db.o \
  ../../src/inventory/inventory.o \
  ../../src/inventory/craft.o \
  ../../src/combat/combat.o \
  ../../src/combat/weapons.o \
  ../../src/combat/bullet.o \
  ../../src/ai/zombie.o \
  ../../src/ai/animal.o \
  ../../src/ai/bandit.o \
  ../../src/weather/weather.o \
  ../../src/save/save_game.o \
  ../../src/ui/hud.o \
  ../../src/ui/quest.o \
  ../../src/ui/item_sprites.o \
  main_psp.o \
  platform_psp.o

INCDIR = ../../include
CFLAGS = -O2 -G0 -Wall -I$(INCDIR) -DMDAZ_PSP=1
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti -std=c++11
ASFLAGS = $(CFLAGS)

LIBDIR =
LIBS = -lpspgum -lpspgu -lm -lpspctrl -lpspdisplay -lpspge -lpsppower
LDFLAGS =

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Mini DAYZ C++

PSPSDK = $(shell psp-config --pspsdk-path 2>/dev/null)
ifeq ($(PSPSDK),)
$(warning PSPSDK not found — install pspdev before building EBOOT)
endif

-include $(PSPSDK)/lib/build.mak
