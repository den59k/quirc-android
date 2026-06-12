#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
IMAGES_DIR="${1:-$SCRIPT_DIR/images}"

echo "=== Building ==="
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -G Ninja 2>/dev/null \
  || cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR"

echo ""
echo "=== Running tests ==="
cd "$BUILD_DIR"
./quirc_test "$IMAGES_DIR"
