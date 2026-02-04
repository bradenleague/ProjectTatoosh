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

echo "Done! Run with: ./engine/build/vkquake -basedir external/librequake -game tatoosh"
