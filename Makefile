# Tatoosh - Top-level build wrapper
#
# Usage:
#   make          Build everything (RmlUI libs + engine)
#   make run      Build, assemble game directory, and run
#   make engine   Build just the engine (after libs are built)
#   make libs     Build just RmlUI + UI integration library
#   make assemble Set up game/ runtime directory (symlinks + assets)
#   make clean    Clean all build artifacts
#   make setup    Re-run meson setup for the engine

GAMEDIR := game

.PHONY: all libs engine run clean setup assemble

all: libs engine

libs: build/librmlui.a

build/librmlui.a: CMakeLists.txt $(wildcard external/rmlui/Source/**/*.cpp) $(wildcard rmlui/**/*.cpp)
	cmake -B build
	cmake --build build

engine: engine/build/vkquake

engine/build/vkquake: libs $(wildcard engine/Quake/*.c) $(wildcard engine/Quake/*.h) $(wildcard engine/Shaders/*) engine/meson.build
	@if [ ! -f engine/build/build.ninja ]; then \
		cd engine && meson setup build; \
	fi
	cd engine && meson compile -C build

assemble:
	@mkdir -p $(GAMEDIR)/tatoosh
	@test -L $(GAMEDIR)/id1 || ln -s ../external/librequake/lq1 $(GAMEDIR)/id1
	@test -L $(GAMEDIR)/tatoosh/quake.rc || ln -s ../../tatoosh/quake.rc $(GAMEDIR)/tatoosh/quake.rc
	@test -L $(GAMEDIR)/tatoosh/config.cfg || ln -s ../../tatoosh/config.cfg $(GAMEDIR)/tatoosh/config.cfg
	@test -f tatoosh/progs.dat && cp tatoosh/progs.dat $(GAMEDIR)/tatoosh/progs.dat || true

run: all assemble
	./engine/build/vkquake -basedir $(GAMEDIR) -game tatoosh

setup:
	rm -rf engine/build
	cd engine && meson setup build

clean:
	rm -rf build
	rm -rf engine/build
	rm -rf game
