#!/bin/bash
# Compile Tatoosh QuakeC
# Usage: ./scripts/compile-qc.sh
#
# Compiles tatoosh/qcsrc/ → tatoosh/progs.dat
#
# Requires fteqcc in tools/:
#   Linux:  tools/fteqcc64  (download from https://fte.triptohell.info/downloads)
#   macOS:  tools/fteqcc    (build from https://github.com/BryanHaley/fteqw-applesilicon)

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Pick the right compiler binary for this platform
case "$(uname -s)" in
    Linux)  FTEQCC="$PROJECT_ROOT/tools/fteqcc64" ;;
    Darwin) FTEQCC="$PROJECT_ROOT/tools/fteqcc" ;;
    *)      echo "Error: unsupported platform $(uname -s)"; exit 1 ;;
esac

if [ ! -x "$FTEQCC" ]; then
    echo "Error: fteqcc not found at $FTEQCC"
    echo ""
    case "$(uname -s)" in
        Linux)
            echo "Download the Linux 64-bit binary:"
            echo "  curl -L -o tools/fteqcc64 https://sourceforge.net/projects/fteqw/files/FTEQCC/fteqcc64/download"
            echo "  chmod +x tools/fteqcc64"
            ;;
        Darwin)
            echo "Build the Apple Silicon fork:"
            echo "  git clone https://github.com/BryanHaley/fteqw-applesilicon /tmp/fteqw"
            echo "  make -C /tmp/fteqw/engine fteqcc"
            echo "  cp /tmp/fteqw/engine/fteqcc tools/fteqcc"
            ;;
    esac
    exit 1
fi

cd "$PROJECT_ROOT/tatoosh/qcsrc"

echo "Compiling QuakeC..."
"$FTEQCC"

# Copy to runtime game directory if assembled
if [ -d "$PROJECT_ROOT/game/tatoosh" ]; then
    cp "$PROJECT_ROOT/tatoosh/progs.dat" "$PROJECT_ROOT/game/tatoosh/progs.dat"
    echo "Copied progs.dat to game/tatoosh/"
fi

echo "Done! Run with: make run"
