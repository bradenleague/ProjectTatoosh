# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Project Tatoosh is built on:
- **vkQuake** (Vulkan-based Quake engine) - `engine/` submodule (branch: `tatoosh`)
- **RmlUI** (HTML/CSS UI framework) - `external/rmlui/` submodule (branch: `tatoosh`)
- **LibreQuake** (BSD-licensed game assets) - PAK files downloaded to `external/assets/id1/`

Engine and RmlUI are custom forks on `tatoosh` branches.

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
make setup       # First-time: check deps, init engine+rmlui submodules, download PAK files to external/assets/id1
make             # Build everything
make run         # Build, assemble game/, and launch
make engine      # Rebuild engine (+ embedded RmlUI deps)
make libs        # Compatibility alias for engine build
make assemble    # Set up game/ runtime directory (symlinks + assets)
make meson-setup # Wipe and re-run meson setup for engine
make clean       # Clean build artifacts only (preserves game/)
make distclean   # Clean build artifacts + game runtime data
```

### Manual Run
```bash
./engine/build/vkquake -basedir game -game tatoosh
```

### Asset Compilation (requires tools in `tools/`, git-ignored)

```bash
./scripts/compile-qc.sh              # QuakeC → progs.dat (needs tools/fteqcc)
LIBREQUAKE_SRC=~/src/LibreQuake ./scripts/compile-maps.sh -m
LIBREQUAKE_SRC=~/src/LibreQuake ./scripts/compile-maps.sh -d src/e1
LIBREQUAKE_SRC=~/src/LibreQuake ./scripts/compile-maps.sh -c
```

On Arch Linux: `yay -S ericw-tools` for map tools, or download binaries into `tools/`.

## Architecture

### Build Topology

- **Meson** (`engine/meson.build`): Primary build orchestration for vkQuake and `rmlui/` integration sources.
- **CMake via Meson subproject** (`engine/subprojects/rmlui`): Meson bridges into RmlUI's CMake project and links `rmlui_core` + `rmlui_debugger`.

The Makefile is a thin wrapper around Meson build/assemble/run commands.

### RmlUI Integration Layer (`rmlui/`)

```
rmlui/
  ui_manager.h/.cpp     Public C API (extern "C") — what vkQuake calls
  types/                Header-only shared types
    ├── input_mode.h        UI_INPUT_INACTIVE / MENU_ACTIVE / OVERLAY
    ├── game_state.h        Synced game state struct
    ├── cvar_schema.h       Console variable metadata
    ├── cvar_provider.h     ICvarProvider interface
    └── command_executor.h  ICommandExecutor interface
  internal/             C++ implementation (RmlUI + Vulkan adapters)
    ├── render_interface_vk   Custom Vulkan renderer (pipelines, buffers, textures)
    ├── system_interface      Time/logging/clipboard bridge to engine
    ├── game_data_model       Sync Quake game state → RmlUI data bindings
    ├── cvar_binding          Two-way sync between cvars and UI elements
    ├── menu_event_handler    Handle menu clicks, parse action strings
    ├── quake_cvar_provider   ICvarProvider implementation
    └── quake_command_executor ICommandExecutor implementation
```

### C/C++ Boundary

- RmlUI layer is C++ (`namespace Tatoosh`, C++17)
- vkQuake engine is pure C (gnu11)
- `rmlui/ui_manager.h` provides `extern "C"` API
- All engine-side UI calls are gated with `#ifdef USE_RMLUI`

### Input Modes (`ui_input_mode_t`)

1. `UI_INPUT_INACTIVE` — Game controls active, RmlUI doesn't capture input
2. `UI_INPUT_MENU_ACTIVE` — Menu captures all input
3. `UI_INPUT_OVERLAY` — HUD visible, input passes through to game

### Runtime Directory (`game/`)

Assembled by `make assemble`, gitignored:
- `game/id1/` → symlink to `external/assets/id1/` (base assets, read-only)
- `game/ui/` → symlink to `ui/` (RmlUI documents and fonts)
- `game/tatoosh/quake.rc` and `game/tatoosh/config.cfg` → symlinks to `tatoosh/` source files
- `game/tatoosh/progs.dat` → copied from `tatoosh/progs.dat` when present
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
