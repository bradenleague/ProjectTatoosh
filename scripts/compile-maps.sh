#!/bin/bash
# Compile LibreQuake maps using ericw-tools
# Usage: ./scripts/compile-maps.sh [options]
#
# Options (passed to compile_maps.py):
#   -m          Compile all maps
#   -s <map>    Compile a single map (e.g., -s lq_e1m1)
#   -d <dir>    Compile all maps in a directory (e.g., -d src/e1)
#   -c          Clean build artifacts
#   -h          Show help
#
# Examples:
#   ./scripts/compile-maps.sh -m              # Compile all maps
#   ./scripts/compile-maps.sh -d src/e1       # Compile episode 1
#   ./scripts/compile-maps.sh -s lq_e1m1      # Compile single map

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
TOOLS_DIR="$PROJECT_ROOT/tools"
MAPS_DIR="$PROJECT_ROOT/external/librequake/lq1/maps"

# Check for required tools
if [[ ! -x "$TOOLS_DIR/qbsp" ]]; then
    echo "Error: ericw-tools not found in $TOOLS_DIR"
    echo "Download from: https://github.com/ericwa/ericw-tools/releases"
    echo "Extract qbsp, vis, light, and *.dylib to tools/"
    exit 1
fi

# Add tools to PATH and set library path for ericw-tools
export PATH="$TOOLS_DIR:$PATH"
export DYLD_LIBRARY_PATH="$TOOLS_DIR:$DYLD_LIBRARY_PATH"

cd "$MAPS_DIR"

if [[ $# -eq 0 ]]; then
    echo "Usage: $0 [options]"
    echo "Run '$0 -h' for help"
    exit 1
fi

python3 compile_maps.py "$@"
