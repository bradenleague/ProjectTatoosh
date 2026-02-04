# Project Tatoosh

<p align="center">
  <img width="600"
       src="https://github.com/user-attachments/assets/489b35a4-bbca-4cb3-b55c-6483a5deafaa" />
</p>


Built on [vkQuake](https://github.com/Novum/vkQuake) (Vulkan renderer), [RmlUI](https://github.com/mikke89/RmlUi) (HTML/CSS UI), and [LibreQuake](https://github.com/lavenderdotpet/LibreQuake) (BSD-licensed base assets).

## Getting Started

### Prerequisites (macOS)

```bash
brew install cmake meson ninja sdl2 molten-vk vulkan-headers glslang freetype
```

### Clone and Setup

```bash
git clone --recursive https://github.com/bradenleague/Tatoosh.git
cd Tatoosh

# Download LibreQuake PAK files
curl -L "https://github.com/lavenderdotpet/LibreQuake/releases/download/v0.09-beta/full.zip" -o /tmp/librequake.zip
unzip -q /tmp/librequake.zip -d /tmp/librequake
cp /tmp/librequake/full/id1/pak*.pak external/librequake/lq1/
```

### Build and Run

```bash
make run
```

Other targets:

| Command | Description |
|---------|-------------|
| `make` | Build everything (RmlUI libs + engine) |
| `make run` | Build, assemble game/, and launch |
| `make libs` | Build only RmlUI + UI integration library |
| `make engine` | Build only the engine |
| `make assemble` | Set up game/ runtime directory (symlinks + assets) |
| `make setup` | Re-run meson setup for the engine |
| `make clean` | Remove all build artifacts (including game/) |

<p align="center">
  <img width="600"
       src="https://github.com/user-attachments/assets/fccf44b6-feac-4438-a05e-ca6b4172ca7e" />
</p>

## Asset Compilation (optional)

Maps and QuakeC can be compiled from source if you have the tooling set up:

```bash
./scripts/compile-qc.sh           # Compile QuakeC -> progs.dat
./scripts/compile-maps.sh -m      # Compile all maps -> .bsp files
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
├── libembree.2.dylib   # ericw-tools runtime dependency
├── libtbb.dylib        # ericw-tools runtime dependency
└── libtbbmalloc.dylib  # ericw-tools runtime dependency
```

> **Note:** PAK files are currently downloaded from a LibreQuake release rather than
> built from source. LibreQuake's `build.py` can generate them with `qpakman`, but
> that toolchain is not yet integrated into the build.

## License

- **Tatoosh code:** GPL v2 (see [LICENSE](LICENSE))
- **vkQuake:** GPL v2
- **RmlUI:** MIT License
- **LibreQuake:** BSD License

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for details.
