#!/bin/bash
# Compile LibreQuake QuakeC
# Usage: ./scripts/compile-qc.sh
#
# Note: id1/ is a symlink to lq1/, so progs.dat is automatically available
# to vkQuake after compilation (no copy needed).

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_ROOT/external/librequake/qcsrc"

echo "Compiling QuakeC..."
"$PROJECT_ROOT/tools/fteqcc"

echo "Done! Run with: ./engine/build/vkquake -basedir external/librequake"
