#!/bin/bash
# This script is only for GitHub Actions use
set -euo pipefail

SCRIPT_DIR="$(dirname -- "${BASH_SOURCE[0]}")"

source "$SCRIPT_DIR/functions.sh"

BUILD="$(mktemp -d)"
PKGDIR="$(mktemp -d)"
ARTIFACT="$WORKSPACE/build"

mkdir -p "$ARTIFACT"
mkdir -p "$BUILD"

trap 'rm -rf '"$BUILD" EXIT

cd "$BUILD"

case "${1:-}" in
make)
    cmake "$WORKSPACE" -DSTANDALONE_MODE=ON -DLPAC_WITH_APDU_AT=ON
    make -j VERBOSE=1
    make DESTDIR="$PKGDIR" install

    test-driver-available # test driver loader works

    copy-license "$PKGDIR/executables"
    copy-usage "$PKGDIR/executables"
    create-bundle "$ARTIFACT/lpac-$KERNEL-$MACHINE.zip" "$PKGDIR/executables"
    ;;
make-qmi)
    cmake "$WORKSPACE" \
        -DSTANDALONE_MODE=ON \
        -DLPAC_WITH_APDU_QMI=ON \
        -DLPAC_WITH_APDU_QMI_QRTR=ON \
        -DLPAC_WITH_APDU_UQMI=ON \
        -DLPAC_WITH_APDU_MBIM=ON
    make -j VERBOSE=1
    make DESTDIR="$PKGDIR" install
    copy-license "$PKGDIR/executables"
    copy-usage "$PKGDIR/executables"
    create-bundle "$ARTIFACT/lpac-$KERNEL-$MACHINE-with-qmi.zip" "$PKGDIR/executables"
    ;;
make-gbinder)
    cmake "$WORKSPACE" -DSTANDALONE_MODE=ON -DLPAC_WITH_APDU_GBINDER=ON
    make -j VERBOSE=1
    make DESTDIR="$PKGDIR" install
    copy-license "$PKGDIR/executables"
    copy-usage "$PKGDIR/executables"
    create-bundle "$ARTIFACT/lpac-$KERNEL-$MACHINE-with-gbinder.zip" "$PKGDIR/executables"
    ;;
make-without-lto)
    cmake "$WORKSPACE" -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF -DSTANDALONE_MODE=ON
    make -j VERBOSE=1
    make DESTDIR="$PKGDIR" install

    test-driver-available # test driver loader works

    copy-license "$PKGDIR/executables"
    copy-usage "$PKGDIR/executables"
    create-bundle "$ARTIFACT/lpac-$KERNEL-$MACHINE-without-lto.zip" "$PKGDIR/executables"
    ;;
mingw)
    cmake "$WORKSPACE" -DSTANDALONE_MODE=ON -DCMAKE_TOOLCHAIN_FILE=./cmake/linux-mingw64.cmake
    make -j VERBOSE=1
    make DESTDIR="$PKGDIR" install
    copy-curl-win "$PKGDIR/executables/lib"
    copy-license "$PKGDIR/executables"
    copy-usage "$PKGDIR/executables"
    create-bundle "$ARTIFACT/lpac-windows-x86_64-mingw.zip" "$PKGDIR/executables"
    ;;
woa-mingw)
    cmake "$WORKSPACE" -DSTANDALONE_MODE=ON -DCMAKE_TOOLCHAIN_FILE=./cmake/linux-mingw64-woa.cmake
    make -j VERBOSE=1
    make DESTDIR="$PKGDIR" install
    copy-curl-woa "$PKGDIR/executables/lib"
    copy-license "$PKGDIR/executables"
    copy-usage "$PKGDIR/executables"
    create-bundle "$ARTIFACT/lpac-windows-arm64-mingw.zip" "$PKGDIR/executables"
    ;;
woa-zig)
    cmake "$WORKSPACE" -DSTANDALONE_MODE=ON -DCMAKE_TOOLCHAIN_FILE=./cmake/aarch64-windows-zig.cmake
    make -j VERBOSE=1
    make DESTDIR="$PKGDIR" install
    copy-curl-woa "$PKGDIR/executables/lib"
    copy-license "$PKGDIR/executables"
    copy-usage "$PKGDIR/executables"
    create-bundle "$ARTIFACT/lpac-windows-arm64-zig.zip" "$PKGDIR/executables"
    ;;
*)
    echo "Usage: $0 {make,debian,mingw,woa-mingw,woa-zig}"
    exit 1
    ;;
esac
