# Source-only FireRed adapter; this check target contains no game or ROM data.
SHELL := bash
DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
SDK_ROOT := $(abspath ..)
BUILD ?= $(CURDIR)/build
CC := $(DEVKITARM)/bin/arm-none-eabi-gcc
HOSTCC ?= gcc
ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -marm
INCLUDES := -I$(CURDIR)/include -I$(SDK_ROOT)/3ds_port/include
CORE := $(CURDIR)/src/bridge.c $(SDK_ROOT)/3ds_port/src/3ds_input_decode.c $(SDK_ROOT)/3ds_port/src/3ds_video_decode.c
.PHONY: check port-check clean
check:
	@mkdir -p $(BUILD)
	$(HOSTCC) -std=c11 -O2 -Wall -Wextra -Werror $(INCLUDES) tests/bridge_test.c $(CORE) -o $(BUILD)/bridge_test
	$(BUILD)/bridge_test
	$(HOSTCC) -std=c99 -O2 -Wall -Wextra -Werror $(INCLUDES) $(SDK_ROOT)/3ds_port/tests/input_test.c $(SDK_ROOT)/3ds_port/src/3ds_input_decode.c -o $(BUILD)/input_test
	$(BUILD)/input_test
	$(HOSTCC) -std=c99 -O2 -Wall -Wextra -Werror $(INCLUDES) $(SDK_ROOT)/3ds_port/tests/video_decode_test.c $(SDK_ROOT)/3ds_port/src/3ds_video_decode.c -o $(BUILD)/video_test
	$(BUILD)/video_test
	$(HOSTCC) -std=c11 -O2 -Wall -Wextra -Werror $(INCLUDES) tests/render_test.c src/render_text256.c $(CORE) -o $(BUILD)/render_test
	$(BUILD)/render_test
	$(HOSTCC) -std=c11 -O2 -Wall -Wextra -Werror $(INCLUDES) tests/payload_test.c src/payload.c -o $(BUILD)/payload_test
	cd $(BUILD) && ./payload_test
	$(HOSTCC) -std=c11 -O2 -Wall -Wextra -Werror -pthread $(INCLUDES) -I$(SDK_ROOT)/3ds_port/tests/settings_host tests/voxel_options_test.c src/voxel_options.c -o $(BUILD)/voxel_options_test
	$(BUILD)/voxel_options_test
port-check:
	@mkdir -p $(BUILD)
	$(CC) $(ARCH) -O2 -std=gnu11 -Wall -Wextra -Werror $(INCLUDES) -I$(DEVKITPRO)/libctru/include -D__3DS__ $(EXTRA_FLAGS) $(CURDIR)/src/port_check_3ds.c $(CORE) $(EXTRA_SOURCES) $(EXTRA_OBJECTS) -L$(DEVKITPRO)/libctru/lib -lctru -lm -specs=3dsx.specs -Wl,-Map,$(BUILD)/port-check.map -o $(BUILD)/port-check.elf
	$(DEVKITPRO)/tools/bin/3dsxtool $(BUILD)/port-check.elf $(BUILD)/FireRed-Evolv3D-port-check.3dsx
clean:
	rm -f $(BUILD)/bridge_test $(BUILD)/port-check.elf $(BUILD)/port-check.map $(BUILD)/FireRed-Evolv3D-port-check.3dsx

.PHONY: runtime-check
runtime-check:
	@mkdir -p $(BUILD)
	$(CC) $(ARCH) -O2 -std=gnu11 -Wall -Wextra -Werror $(INCLUDES) -I$(DEVKITPRO)/libctru/include -D__3DS__ -c src/runtime_3ds.c -o $(BUILD)/runtime_3ds.o
	python3 tests/test_bundle_section.py
