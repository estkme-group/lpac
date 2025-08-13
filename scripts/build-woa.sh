#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(dirname -- "${BASH_SOURCE[0]}")"

source "$SCRIPT_DIR/functions.sh"

BUILD="$WORKSPACE/build"
rm -rf "$BUILD"
mkdir -p "$BUILD"

cd "$BUILD" || exit 1

case "${1:-}" in
mingw)
    cmake "$WORKSPACE" -DCMAKE_TOOLCHAIN_FILE=./cmake/linux-mingw64-woa.cmake
    ;;
zig)
    cmake "$WORKSPACE" -DCMAKE_TOOLCHAIN_FILE=./cmake/aarch64-windows-zig.cmake

    ;;
*)
    echo "Usage: $0 {mingw,zig}"
    exit 1
    ;;
esac

make -j
copy-license "$BUILD/output"
copy-curl-woa "$BUILD/output"
copy-usage "$BUILD/output"
