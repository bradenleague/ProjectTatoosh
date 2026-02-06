# Tatoosh - Top-level build wrapper
#
# Usage:
#   make          Build everything
#   make run      Build, assemble game directory, and run
#   make engine   Build the engine (+ embedded RmlUI deps)
#   make libs     Alias for engine build (for compatibility)
#   make assemble Set up game/ runtime directory (symlinks + assets)
#   make clean    Clean build artifacts only (preserves game runtime data)
#   make distclean Clean everything including game runtime data
#   make setup    Run first-time setup (deps, engine/rmlui submodules, PAK files)
#   make meson-setup  Re-run meson setup for the engine

GAMEDIR := game
ID1_LINK_TARGET := ../external/assets/id1
UI_LINK_TARGET := ../ui

.PHONY: all libs engine run clean distclean setup meson-setup assemble check-submodules

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
all: engine

libs: engine

engine: check-submodules engine/build/.configured
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
	@if [ -e $(GAMEDIR)/id1 ] && [ ! -L $(GAMEDIR)/id1 ]; then \
		echo "Error: $(GAMEDIR)/id1 exists and is not a symlink."; \
		exit 1; \
	fi
	@rm -f $(GAMEDIR)/id1
	@ln -s $(ID1_LINK_TARGET) $(GAMEDIR)/id1
	@if [ -e $(GAMEDIR)/ui ] && [ ! -L $(GAMEDIR)/ui ]; then \
		echo "Error: $(GAMEDIR)/ui exists and is not a symlink."; \
		exit 1; \
	fi
	@rm -f $(GAMEDIR)/ui
	@ln -s $(UI_LINK_TARGET) $(GAMEDIR)/ui
	@for cfg in quake.rc config.cfg; do \
		target="$(GAMEDIR)/tatoosh/$$cfg"; \
		backup="$$target.migrated-copy"; \
		if [ -e "$$target" ] && [ ! -L "$$target" ]; then \
			if [ ! -e "$$backup" ]; then \
				mv "$$target" "$$backup"; \
				echo "Info: preserved existing $$target at $$backup"; \
			else \
				rm -f "$$target"; \
			fi; \
		fi; \
		rm -f "$$target"; \
		ln -s ../../tatoosh/$$cfg "$$target"; \
	done
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

distclean: clean
	rm -rf game
