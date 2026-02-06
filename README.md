# Project Tatoosh

<p align="center">
  <img width="600"
       src="https://github.com/user-attachments/assets/489b35a4-bbca-4cb3-b55c-6483a5deafaa" />
</p>


Built on [vkQuake](https://github.com/Novum/vkQuake) (Vulkan renderer), [RmlUI](https://github.com/mikke89/RmlUi) (HTML/CSS UI), and [LibreQuake](https://github.com/lavenderdotpet/LibreQuake) (BSD-licensed base assets).

## Getting Started

### Prerequisites

**Arch Linux:**
```bash
sudo pacman -S cmake meson ninja sdl2 vulkan-devel glslang freetype2
```

**macOS:**
```bash
brew install cmake meson ninja sdl2 molten-vk vulkan-headers glslang freetype
```

### Clone, Setup, and Run

```bash
git clone https://github.com/bradenleague/ProjectTatoosh.git
cd ProjectTatoosh
make setup   # checks deps, inits engine+rmlui submodules, downloads PAK files to external/assets/id1
make run
```

Other targets:

| Command | Description |
|---------|-------------|
| `make` | Build everything |
| `make run` | Build, assemble game/, and launch |
| `make libs` | Compatibility alias for engine build |
| `make engine` | Build the engine (+ embedded RmlUI deps) |
| `make assemble` | Set up game/ runtime directory (symlinks + assets) |
| `make setup` | First-time setup (deps, engine+rmlui submodules, PAK files in `external/assets/id1`) |
| `make meson-setup` | Re-run meson setup for the engine |
| `make clean` | Remove build artifacts only (preserves `game/`) |
| `make distclean` | Remove build artifacts and `game/` runtime data |

<p align="center">
  <img width="600"
       src="https://github.com/user-attachments/assets/fccf44b6-feac-4438-a05e-ca6b4172ca7e" />
</p>

## Asset Compilation (optional)

Maps and QuakeC can be compiled from source if you have the tooling set up:

```bash
./scripts/compile-qc.sh           # Compile QuakeC -> progs.dat
LIBREQUAKE_SRC=~/src/LibreQuake ./scripts/compile-maps.sh -m   # Optional map compile
```

These require tools in `tools/` (git-ignored). Download and extract:

- **[ericw-tools](https://github.com/ericwa/ericw-tools/releases)** — `qbsp`, `vis`, `light`, and bundled `*.dylib` files
- **[FTEQCC](https://www.fteqcc.com/)** — `fteqcc` binary

```
tools/
├── fteqcc              # QuakeC compiler
├── qbsp                # BSP compiler (ericw-tools)
├── vis                 # Visibility compiler (ericw-tools)
├── light               # Light compiler (ericw-tools)
├── libembree.2.dylib   # ericw-tools runtime dep (macOS)
├── libtbb.dylib        # ericw-tools runtime dep (macOS)
└── libtbbmalloc.dylib  # ericw-tools runtime dep (macOS)
```

On Linux, ericw-tools links against system libraries instead. Install with `yay -S ericw-tools` (AUR) or build from source.

Map compilation is intentionally decoupled from repo setup and requires an external LibreQuake source checkout (`LIBREQUAKE_SRC`).

> **Note:** PAK files are currently downloaded from a LibreQuake release rather than
> built from source. LibreQuake's `build.py` can generate them with `qpakman`, but
> that toolchain is not yet integrated into the build.

## License

- **Tatoosh code:** GPL v2 (see [LICENSE](LICENSE))
- **vkQuake:** GPL v2
- **RmlUI:** MIT License
- **LibreQuake:** BSD License

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for details.
