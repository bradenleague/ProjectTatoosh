# Tatoosh - Top-level build wrapper
#
# Usage:
#   make          Build everything (RmlUI libs + engine)
#   make run      Build, assemble game directory, and run
#   make engine   Build just the engine (after libs are built)
#   make libs     Build just RmlUI static library
#   make assemble Set up game/ runtime directory (symlinks + assets)
#   make clean    Clean all build artifacts
#   make setup    Run first-time setup (deps, submodules, PAK files)
#   make meson-setup  Re-run meson setup for the engine

GAMEDIR := game

.PHONY: all libs engine run clean setup meson-setup assemble check-submodules

# --- Submodule guard ---
check-submodules:
	@if [ ! -f external/rmlui/CMakeLists.txt ]; then \
		echo "Error: RmlUI submodule not initialized. Run 'make setup' first."; \
		exit 1; \
	fi
	@if [ ! -d engine/Quake ]; then \
		echo "Error: Engine submodule not initialized. Run 'make setup' first."; \
		exit 1; \
	fi

# --- Top-level targets ---
all: libs engine

libs: check-submodules
	cmake -B build
	cmake --build build

engine: check-submodules libs engine/build/.configured
	meson compile -C engine/build

# Stamp-based meson setup — re-runs when meson.build or options change
engine/build/.configured: engine/meson.build engine/meson_options.txt
	@if [ -f engine/build/build.ninja ]; then \
		meson setup engine/build engine --reconfigure || \
		(rm -rf engine/build && meson setup engine/build engine); \
	else \
		meson setup engine/build engine; \
	fi
	@touch $@

assemble:
	@mkdir -p $(GAMEDIR)/tatoosh
	@test -L $(GAMEDIR)/id1 || ln -s $(CURDIR)/external/librequake/lq1 $(GAMEDIR)/id1
	@cp tatoosh/quake.rc $(GAMEDIR)/tatoosh/quake.rc
	@cp tatoosh/config.cfg $(GAMEDIR)/tatoosh/config.cfg
	@test -f tatoosh/progs.dat && cp tatoosh/progs.dat $(GAMEDIR)/tatoosh/progs.dat || true

run: all assemble
	./engine/build/vkquake -basedir $(GAMEDIR) -game tatoosh

setup:
	./setup.sh

meson-setup:
	rm -rf engine/build
	meson setup engine/build engine
	@touch engine/build/.configured

clean:
	rm -rf build
	rm -rf engine/build
	rm -rf game
