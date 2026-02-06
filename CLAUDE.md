# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Project Tatoosh is built on:
- **vkQuake** (Vulkan-based Quake engine) - `engine/` submodule (branch: `tatoosh`)
- **RmlUI** (HTML/CSS UI framework) - `external/rmlui/` submodule (branch: `tatoosh`)
- **LibreQuake** (BSD-licensed game assets) - `external/librequake/` submodule

All three submodules are custom forks. Engine and RmlUI are on `tatoosh` branches.

## Build Commands

### Prerequisites

**Arch Linux (primary):**
```bash
sudo pacman -S cmake meson ninja sdl2 vulkan-devel glslang freetype2
```

**macOS:**
```bash
brew install cmake meson ninja sdl2 molten-vk vulkan-headers glslang freetype
```

### Build & Run
```bash
make setup       # First-time: check deps, init submodules, download PAK files
make             # Build RmlUI libs (CMake) + engine (Meson)
make run         # Build, assemble game/, and launch
make engine      # Rebuild just the engine
make libs        # Rebuild just RmlUI static library
make assemble    # Set up game/ runtime directory (symlinks + assets)
make meson-setup # Wipe and re-run meson setup for engine
make clean       # Clean all build artifacts (including game/)
```

### Manual Run
```bash
./engine/build/vkquake -basedir game -game tatoosh
```

### Asset Compilation (requires tools in `tools/`, git-ignored)

```bash
./scripts/compile-qc.sh              # QuakeC → progs.dat (needs tools/fteqcc)
./scripts/compile-maps.sh -m         # All maps (needs tools/qbsp, vis, light)
./scripts/compile-maps.sh -d src/e1  # Episode 1 only
./scripts/compile-maps.sh -c         # Clean map build artifacts
```

On Arch Linux: `yay -S ericw-tools` for map tools, or download binaries into `tools/`.

## Architecture

### Two Build Systems

- **CMake** (`CMakeLists.txt`): Builds RmlUI from submodule as static libraries (`librmlui.a`, `librmlui_debugger.a`)
- **Meson** (`engine/meson.build`): Builds vkQuake engine, compiles `rmlui/` integration sources, links `librmlui.a`

The Makefile orchestrates both: `make` runs CMake first, then Meson.

### RmlUI Integration Layer (`rmlui/`)

Layered architecture with dependency inversion:

```
interface/          C API facade (extern "C") — what vkQuake calls
  └── ui_manager    Init, frame loop, input dispatch, document management

application/        Coordination logic
  ├── document_manager  Load/unload RML documents
  └── menu_stack        Stack-based menu navigation (push/pop)

domain/             Pure types, no dependencies
  ├── input_mode.h      UI_INPUT_INACTIVE / MENU_ACTIVE / OVERLAY
  ├── game_state.h      Synced game state struct
  ├── cvar_schema.h     Console variable metadata
  └── ports/            Interfaces for dependency inversion
      ├── cvar_provider.h
      ├── command_executor.h
      └── logger.h

infrastructure/     RmlUI + Vulkan adapters
  ├── render_interface_vk   Custom Vulkan renderer (pipelines, buffers, textures)
  ├── system_interface      Time/logging/clipboard bridge to engine
  ├── game_data_model       Sync Quake game state → RmlUI data bindings
  ├── cvar_binding          Two-way sync between cvars and UI elements
  ├── menu_event_handler    Handle menu clicks, parse action strings
  └── quake_*_provider/executor/logger  Port implementations
```

### C/C++ Boundary

- RmlUI layer is C++ (`namespace Tatoosh`, C++17)
- vkQuake engine is pure C (gnu11)
- `interface/ui_manager.h` provides `extern "C"` API
- All engine-side UI calls are gated with `#ifdef USE_RMLUI`

### Input Modes (`ui_input_mode_t`)

1. `UI_INPUT_INACTIVE` — Game controls active, RmlUI doesn't capture input
2. `UI_INPUT_MENU_ACTIVE` — Menu captures all input
3. `UI_INPUT_OVERLAY` — HUD visible, input passes through to game

### Runtime Directory (`game/`)

Assembled by `make assemble`, gitignored:
- `game/id1/` → symlink to `external/librequake/lq1/` (base assets, read-only)
- `game/tatoosh/` → copies of `quake.rc`, `config.cfg`, `progs.dat` from `tatoosh/`
- Engine writes `vkQuake.cfg` into `game/tatoosh/`, never into the source tree

### Mod Sources (`tatoosh/`)

QuakeC source (`tatoosh/qcsrc/`, 38 files) and game configs. Source of truth — copied into `game/` at assemble time.

## Engine Integration Points

When modifying engine code, all UI hooks are behind `#ifdef USE_RMLUI`:

| Point | Engine File | UI Call |
|-------|-------------|---------|
| Init | `host.c` | `UI_Init()` after video init |
| Frame | `gl_screen.c` | `UI_Update()` and `UI_Render()` |
| Input | `in_sdl2.c` | `UI_*Event()` functions |
| Escape | `keys.c` | `UI_WantsMenuInput()`, `UI_HandleEscape()` |
| Shutdown | `host.c` | `UI_Shutdown()` |

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

## Skills

Use `/rmlui` when working with RCSS/RML files — RmlUI has many syntax differences from standard CSS.

## Key Documentation

- `docs/RMLUI_INTEGRATION.md` — Input handling system, menu stack, data binding, and full API reference
- `.claude/skills/rmlui.md` — RmlUI/RCSS syntax rules and gotchas
