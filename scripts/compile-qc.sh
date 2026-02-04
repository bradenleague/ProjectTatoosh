#!/bin/bash
# Compile Tatoosh QuakeC
# Usage: ./scripts/compile-qc.sh
#
# Compiles tatoosh/qcsrc/ → tatoosh/progs.dat

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_ROOT/tatoosh/qcsrc"

echo "Compiling QuakeC..."
"$PROJECT_ROOT/tools/fteqcc"

# Copy to runtime game directory if assembled
if [ -d "$PROJECT_ROOT/game/tatoosh" ]; then
    cp "$PROJECT_ROOT/tatoosh/progs.dat" "$PROJECT_ROOT/game/tatoosh/progs.dat"
    echo "Copied progs.dat to game/tatoosh/"
fi

echo "Done! Run with: make run"
