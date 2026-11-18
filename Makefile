GBDK ?= $(HOME)/.local/share/modretro-chromatic/toolchain/.local/gbdk
LCC ?= $(GBDK)/bin/lcc
PYTHON ?= python3
CFLAGS ?= -Wf--opt-code-speed

ROM := columns.gb
OBJECTS := build/main.o build/game.o

.DELETE_ON_ERROR:
.PHONY: all clean

all: $(ROM)

$(ROM): build/$(ROM)
	cp $< $@

build/$(ROM): $(OBJECTS)
	"$(LCC)" -Wm-yn"COLUMNS" -Wl-j -o $@ $(OBJECTS)

build/main.o: src/main.c src/game.h build/tiles.h
	"$(LCC)" $(CFLAGS) -Ibuild -c -o $@ $<

build/game.o: src/game.c src/game.h | build
	"$(LCC)" $(CFLAGS) -c -o $@ $<

build/tiles.h: tools/make_tiles.py | build
	$(PYTHON) tools/make_tiles.py $@

build:
	mkdir -p $@

clean:
	rm -rf build $(ROM)
