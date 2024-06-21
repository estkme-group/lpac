#!/bin/bash
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)

source "$SCRIPT_DIR/functions.sh"

set -euo pipefail

BUILD="$(mktemp -d)"
cd "$BUILD" || exit 1

case "${1:-}" in
mingw)
    TOOLCHAIN="$(download "$MINGW32_TOOLCHAIN_BLOB")"
    cmake "$WORKSPACE" -DCMAKE_TOOLCHAIN_FILE=./cmake/linux-mingw64-woa.cmake "-DTOOLCHAIN_BIN_PATH=$TOOLCHAIN/bin"
    make -j
    copy-license "$BUILD/output"
    copy-curl-woa "$BUILD/output"
    copy-usage "$BUILD/output"
    ;;
zig)
    cmake "$WORKSPACE" -DCMAKE_TOOLCHAIN_FILE=./cmake/aarch64-windows-zig.cmake
    make -j
    copy-license "$BUILD/output"
    copy-curl-woa "$BUILD/output"
    copy-usage "$BUILD/output"
    ;;
*)
    echo "Usage: $0 {mingw,zig}"
    exit 1
    ;;
esac

rm -rf "$BUILD"
