# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Project Tatoosh is built on:
- **vkQuake** (Vulkan-based Quake engine) - `engine/` submodule
- **RmlUI** (HTML/CSS UI framework) - `external/rmlui/` submodule
- **LibreQuake** (BSD-licensed game assets) - `external/librequake/` submodule

## Build Commands

### Prerequisites (macOS)
```bash
brew install cmake meson ninja sdl2 molten-vk vulkan-headers glslang freetype
```

### Build Everything
```bash
make          # Build RmlUI libs + engine
make run      # Build, assemble game/, and run
make engine   # Rebuild just the engine
make libs     # Rebuild just RmlUI + UI library
make assemble # Set up game/ runtime directory (symlinks + assets)
make clean    # Clean all build artifacts (including game/)
make setup    # Re-run meson setup for engine
```

### Run the Game
```bash
make run
# or manually:
make && make assemble
./engine/build/vkquake -basedir game -game tatoosh
```

### Compile QuakeC (gameplay code)
```bash
./scripts/compile-qc.sh
```

### Compile Maps
```bash
./scripts/compile-maps.sh -m        # All maps
./scripts/compile-maps.sh -d src/e1 # Episode 1 only
./scripts/compile-maps.sh -c        # Clean build artifacts
```

## Architecture

### RmlUI Integration Layer (`rmlui/`)

Custom Vulkan-based UI integration with 15 source files:

| File | Purpose |
|------|---------|
| `ui_manager.cpp` | C API for vkQuake, initialization, frame loop |
| `render_interface_vk.cpp` | Custom Vulkan renderer (pipelines, buffers, textures) |
| `game_data_model.cpp` | Sync Quake game state → RmlUI data bindings |
| `cvar_binding.cpp` | Two-way sync between console variables and UI |
| `menu_event_handler.cpp` | Handle menu clicks, parse action strings |
| `system_interface.cpp` | Time/logging/clipboard bridge |

### C/C++ Boundary

- RmlUI layer is C++ with `namespace Tatoosh`
- vkQuake engine is C
- `ui_manager.h` provides C-linkage API (`extern "C"`) for vkQuake integration

### Input Modes (`ui_input_mode_t`)

1. `UI_INPUT_INACTIVE` - Game controls, RmlUI doesn't capture
2. `UI_INPUT_MENU_ACTIVE` - Menu captures all input
3. `UI_INPUT_OVERLAY` - HUD visible, input passes to game

### UI Assets (`ui/`)

- `rml/` - 21+ RML documents (HTML-like markup)
- `rcss/` - 5 RCSS stylesheets (CSS-like)
- `fonts/` - TTF font files

### Runtime Directory (`game/`)

Assembled at build time by `make assemble`, gitignored. Contains:
- `game/id1/` — symlink to `external/librequake/lq1/` (base assets, read-only)
- `game/tatoosh/` — real directory with symlinks to source configs + copied `progs.dat`
- Engine writes `vkQuake.cfg` into `game/tatoosh/`, never into the source tree

### Mod Sources (`tatoosh/`)

QuakeC source and game configs (source of truth, symlinked into `game/`).

## Console Commands

| Command | Purpose |
|---------|---------|
| `ui_menu [path]` | Open RML document |
| `ui_toggle` | Toggle UI visibility |
| `ui_closemenu` | Close all menus |
| `ui_debugger` | Toggle RmlUI visual debugger |

## Code Conventions

- C++ classes: PascalCase with underscore suffix for implementation (`RenderInterface_VK`)
- C++ private members: `m_` prefix
- C functions: snake_case
- C structs: `_t` suffix

## Integration Points

When modifying engine code, these are the key integration files:

| Point | Engine File | UI Call |
|-------|-------------|---------|
| Init | `host.c` | `UI_Init()` after video init |
| Frame | `gl_screen.c` | `UI_Update()` and `UI_Render()` |
| Input | `in_sdl2.c` | `UI_*Event()` functions |
| Escape | `keys.c` | `UI_WantsMenuInput()`, `UI_HandleEscape()` |
| Shutdown | `host.c` | `UI_Shutdown()` |

## Skills

Use `/rmlui` when working with RCSS/RML files - RmlUI has many syntax differences from standard CSS.

## Key Documentation

- `docs/RMLUI_INTEGRATION.md` - Input handling system and API reference
- `.claude/skills/rmlui.md` - RmlUI/RCSS syntax rules and gotchas
