# Host targets run on macOS. ROM rules only exist when N64_INST is set,
# which is the case inside the ipod-brick-n64:dev container.
ROMNAME   ?= games
BUILD_DIR ?= build/n64
IMAGE     := ipod-brick-n64:dev
DOCKER_RUN := docker run --rm -v "$(CURDIR):/app" -w /app $(IMAGE)
HOST_CC     ?= clang
HOST_CFLAGS := -std=c11 -Wall -Wextra -Werror -Werror=double-promotion -O1 -Isrc
GAME      ?= brick
CORE_SRCS := $(wildcard src/*.c) $(wildcard src/games/*.c)
TESTS     := $(patsubst tests/%.c,build/host/%,$(wildcard tests/test_*.c))

.PHONY: test frames music image rom rom-autoplay run shots clean

test: $(TESTS)
	@set -e; for t in $(TESTS); do echo "== $$t"; $$t; done

build/host/%: tests/%.c $(CORE_SRCS) $(wildcard src/*.h src/games/*.h) tests/harness.h
	mkdir -p build/host
	$(HOST_CC) $(HOST_CFLAGS) -Itests -o $@ tests/$*.c $(CORE_SRCS)

frames: build/host/framedump
	rm -rf build/frames/$(GAME) && mkdir -p build/frames/$(GAME)
	./build/host/framedump $(GAME) build/frames/$(GAME)
	for f in build/frames/$(GAME)/*.ppm; do sips -s format png "$$f" --out "$${f%.ppm}.png" >/dev/null; done
	@echo "frames written to build/frames/$(GAME)/*.png"

build/host/framedump: tools/framedump.c $(CORE_SRCS) $(wildcard src/*.h src/games/*.h)
	mkdir -p build/host
	$(HOST_CC) $(HOST_CFLAGS) -o $@ tools/framedump.c $(CORE_SRCS)

music: build/host/musicdump
	mkdir -p build/music
	./build/host/musicdump
	@echo "afplay build/music/menu.wav"

build/host/musicdump: tools/musicdump.c src/synth.c src/music.c src/music_data.c src/synth.h src/music.h
	mkdir -p build/host
	$(HOST_CC) $(HOST_CFLAGS) -o $@ tools/musicdump.c src/synth.c src/music.c src/music_data.c

image:
	docker build -t $(IMAGE) .

rom:
	$(DOCKER_RUN) make rom-in-container DEBUG=$(DEBUG)

rom-autoplay:
	$(DOCKER_RUN) make rom-in-container ROMNAME=games-autoplay BUILD_DIR=build/autoplay AUTOPLAY=1 GAME=$(GAME)

run:
	@test -f games.z64 || { echo "games.z64 missing: run make rom first"; exit 1; }
	open -a ares --args --system "Nintendo 64" "$(CURDIR)/games.z64"

shots:
	@test -f games-autoplay.z64 || { echo "games-autoplay.z64 missing: run make rom-autoplay first"; exit 1; }
	scripts/ares-shot.sh games-autoplay.z64 build/shots 4 8 15

clean:
	$(RM) -r build filesystem *.z64

ifdef N64_INST
include $(N64_INST)/include/n64.mk
N64_CFLAGS += -Isrc
ifeq ($(AUTOPLAY),1)
N64_CFLAGS += -DAUTOPLAY_GAME=\"$(GAME)\"
endif
ifeq ($(DEBUG),1)
N64_CFLAGS += -DBRICK_DEBUG
endif
C_FILES := $(wildcard src/*.c) $(wildcard src/games/*.c) src/n64/app.c
OBJS := $(addprefix $(BUILD_DIR)/,$(C_FILES:.c=.o))
FONT_TTF := assets/Inter-Bold.ttf

filesystem/hud.font64: $(FONT_TTF)
	@mkdir -p $(BUILD_DIR)/font-hud filesystem
	$(N64_MKFONT) --size 12 --display 320x240 -o $(BUILD_DIR)/font-hud "$<"
	cp $(BUILD_DIR)/font-hud/Inter-Bold.font64 $@

filesystem/big.font64: $(FONT_TTF)
	@mkdir -p $(BUILD_DIR)/font-big filesystem
	$(N64_MKFONT) --size 22 --display 320x240 -o $(BUILD_DIR)/font-big "$<"
	cp $(BUILD_DIR)/font-big/Inter-Bold.font64 $@

$(BUILD_DIR)/$(ROMNAME).dfs: filesystem/hud.font64 filesystem/big.font64
$(ROMNAME).z64: $(BUILD_DIR)/$(ROMNAME).dfs
$(BUILD_DIR)/$(ROMNAME).elf: $(OBJS)
N64_ROM_SAVETYPE = eeprom4k
$(ROMNAME).z64: N64_ROM_TITLE = "Games"
.PHONY: rom-in-container
rom-in-container: $(ROMNAME).z64
ifneq ($(wildcard $(BUILD_DIR)),)
  -include $(shell find $(BUILD_DIR) -name '*.d')
endif
endif
