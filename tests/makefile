# Native unit tests. Not cross-compiled - these exercise logic that is the same
# everywhere, so they run on whatever machine you are sitting at.
#
#   make test        (from the repo root)
#
# SDL is needed for headers only: defines.h pulls in platform.h which pulls in
# sdl.h. Nothing here links against SDL, and nothing opens a window.

COMMON = ../workspace/all/common
PLATFORM_DIR = ../workspace/desktop/platform

CC ?= cc
CFLAGS += -std=gnu99 -Wall -g
CFLAGS += -DUSE_SDL2 -DPLATFORM=\"desktop\"
CFLAGS += -I$(COMMON) -I$(PLATFORM_DIR)
CFLAGS += $(shell pkg-config --cflags sdl2 SDL2_image SDL2_ttf)

BUILD = build
BINARIES = $(BUILD)/test_utils

.PHONY: all clean

all: $(BINARIES)
	@for t in $(BINARIES); do echo "== $$t"; ./$$t || exit 1; done

$(BUILD)/test_utils: test_utils.c $(COMMON)/utils.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(BUILD)
