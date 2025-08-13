#!/bin/bash
SCRIPT_DIR=$(dirname -- "${BASH_SOURCE[0]}")

source "$SCRIPT_DIR/functions.sh"

set -euo pipefail

BUILD="$(mktemp -d)"
ARTIFACT="$WORKSPACE/build"

cd "$BUILD" || exit 1

mkdir -p "$BUILD/output"
mkdir -p "$ARTIFACT"

case "${1:-}" in
make)
    cmake "$WORKSPACE"
    make -j
    copy-license "$BUILD/output"
    copy-usage "$BUILD/output"
    create-bundle "$ARTIFACT/lpac-$KERNEL-$MACHINE.zip" "$BUILD/output"
    ;;
make-qmi)
    cmake "$WORKSPACE" -DLPAC_WITH_APDU_QMI=ON -DLPAC_WITH_APDU_QMI_QRTR=ON -DLPAC_WITH_APDU_MBIM=ON
    make -j
    copy-license "$BUILD/output"
    copy-usage "$BUILD/output"
    create-bundle "$ARTIFACT/lpac-$KERNEL-$MACHINE-with-qmi.zip" "$BUILD/output"
    ;;
make-gbinder)
    cmake "$WORKSPACE" -DLPAC_WITH_APDU_GBINDER=ON
    make -j
    copy-license "$BUILD/output"
    copy-usage "$BUILD/output"
    create-bundle "$ARTIFACT/lpac-$KERNEL-$MACHINE-with-gbinder.zip" "$BUILD/output"
    ;;
make-without-lto)
    cmake -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF "$WORKSPACE"
    make -j
    copy-license "$BUILD/output"
    copy-usage "$BUILD/output"
    create-bundle "$ARTIFACT/lpac-$KERNEL-$MACHINE-without-lto.zip" "$BUILD/output"
    ;;
debian)
    cmake "$WORKSPACE" -DCPACK_GENERATOR=DEB
    make -j package
    cp lpac_*.deb "$ARTIFACT"
    ;;
mingw)
    cmake "$WORKSPACE" -DCMAKE_TOOLCHAIN_FILE=./cmake/linux-mingw64.cmake
    make -j
    copy-license "$BUILD/output"
    copy-curl-win "$BUILD/output"
    copy-usage "$BUILD/output"
    create-bundle "$ARTIFACT/lpac-windows-x86_64-mingw.zip" "$BUILD/output"
    ;;
woa-mingw)
    cmake "$WORKSPACE" -DCMAKE_TOOLCHAIN_FILE=./cmake/linux-mingw64-woa.cmake
    make -j
    copy-license "$BUILD/output"
    copy-curl-woa "$BUILD/output"
    copy-usage "$BUILD/output"
    create-bundle "$ARTIFACT/lpac-windows-arm64-mingw.zip" "$BUILD/output"
    ;;
woa-zig)
    cmake "$WORKSPACE" -DCMAKE_TOOLCHAIN_FILE=./cmake/aarch64-windows-zig.cmake
    make -j
    copy-license "$BUILD/output"
    copy-curl-woa "$BUILD/output"
    copy-usage "$BUILD/output"
    create-bundle "$ARTIFACT/lpac-windows-arm64-zig.zip" "$BUILD/output"
    ;;
*)
    echo "Usage: $0 {make,debian,mingw,woa-mingw,woa-zig}"
    exit 1
    ;;
esac

rm -rf "$BUILD"
