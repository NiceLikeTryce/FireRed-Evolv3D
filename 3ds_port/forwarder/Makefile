# HOME Menu launcher: starts sdmc:/3ds/FireRed-Evolv3D/FireRed-Evolv3D.3dsx
# through Luma3DS's hb:ldr, so updates only replace the 3DSX.
#
#   make -C 3ds_port/forwarder            -> dist/FireRed-Evolv3D-Launcher.cia
#
# Needs devkitARM + libctru, makerom and bannertool (on PATH, in ../tools-bin,
# or given as MAKEROM= / BANNERTOOL=).
.SUFFIXES:
DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
CTRULIB := $(DEVKITPRO)/libctru
CC := $(DEVKITARM)/bin/arm-none-eabi-gcc
ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS := $(ARCH) -O2 -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -D__3DS__ -DCURL_STATICLIB -I$(CTRULIB)/include -I$(DEVKITPRO)/portlibs/3ds/include
LDFLAGS := $(ARCH) -specs=3dsx.specs -Wl,--gc-sections -L$(CTRULIB)/lib
LIBS := -L$(DEVKITPRO)/portlibs/3ds/lib -lcurl -lmbedtls -lmbedx509 -lmbedcrypto -lz -lctru -lm
SOURCES := source/main.c source/service.c source/service_manifest.c source/update_http.c source/update_hash.c source/update_bps.c source/update_transaction.c

TOOLS_BIN := $(abspath ../tools-bin)
EXE := $(if $(filter Windows_NT,$(OS)),.exe,)
MAKEROM ?= $(firstword $(wildcard $(TOOLS_BIN)/makerom$(EXE)) makerom)
BANNERTOOL ?= $(firstword $(wildcard $(TOOLS_BIN)/bannertool$(EXE)) bannertool)

ART := artwork
APP_TITLE := FireRed Evolv3D
APP_SHORT := FireRed Evolv3D
APP_AUTHOR := NiceLikeTryce
# Shown by FBI and the HOME Menu; bump with the forwarder, not with the game.
APP_VERSION ?= 1.1.0
CIA := dist/FireRed-Evolv3D-Launcher.cia

all: $(CIA)

build/forwarder.elf: $(SOURCES) $(wildcard source/*.h) Makefile
	@mkdir -p build
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) $(LIBS) -o $@

build/banner.bnr: $(ART)/banner.png $(ART)/banner-sound.wav
	@mkdir -p build
	$(BANNERTOOL) makebanner -i $(ART)/banner.png -a $(ART)/banner-sound.wav -o $@

build/icon.smdh: $(ART)/icon.png Makefile
	@mkdir -p build
	$(BANNERTOOL) makesmdh -s "$(APP_SHORT)" -l "$(APP_TITLE)" -p "$(APP_AUTHOR)" -i $(ART)/icon.png -o $@

$(CIA): build/forwarder.elf build/banner.bnr build/icon.smdh $(wildcard romfs/*.pem romfs/*.txt romfs/licenses/*.txt) forwarder.rsf
	@mkdir -p dist
	$(MAKEROM) -f cia -o $@ -target t -exefslogo -rsf forwarder.rsf -elf build/forwarder.elf \
		-banner build/banner.bnr -icon build/icon.smdh \
		-major $(word 1,$(subst ., ,$(APP_VERSION))) -minor $(word 2,$(subst ., ,$(APP_VERSION))) \
		-micro $(word 3,$(subst ., ,$(APP_VERSION)))

clean:
	rm -rf build dist

.PHONY: all clean
