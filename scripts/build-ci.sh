#!/bin/bash
set -euo pipefail
WORKSPACE="$(pwd)"
KERNEL="$(uname -s)"
MATCHINE="$(uname -m)"
CURL_VERSION="8.6.0_1"
WOA_TOOLCHAIN_VERSION="2024-02-08"
MINGW_CURL_WIN64_BLOB="https://curl.se/windows/dl-$CURL_VERSION/curl-$CURL_VERSION-win64-mingw.zip"
MINGW_CURL_WIN64A_BLOB="https://curl.se/windows/dl-$CURL_VERSION/curl-$CURL_VERSION-win64a-mingw.zip"
MINGW32_TOOLCHAIN_BLOB="https://github.com/Windows-on-ARM-Experiments/mingw-woarm64-build/releases/download/$WOA_TOOLCHAIN_VERSION/aarch64-w64-mingw32-msvcrt-toolchain.tar.gz"

case "$KERNEL" in
Linux)
    KERNEL="linux"
    ;;
Darwin)
    KERNEL="darwin"
    MATCHINE="universal"
    ;;
esac

function download {
    URL="$1"
    SAVED_PATH="$(mktemp)"
    SAVED_DIR="$(mktemp -d)"
    wget --no-verbose "$URL" -O "$SAVED_PATH"
    case "$URL" in
    *.zip)
        unzip -q -d "$SAVED_DIR" "$SAVED_PATH"
        rm "$SAVED_PATH"
        echo "$SAVED_DIR"
        ;;
    *.tar.gz)
        tar -C "$SAVED_DIR" --gzip --extract --file="$SAVED_PATH"
        rm "$SAVED_PATH"
        echo "$SAVED_DIR"
        ;;
    *)
        echo "$SAVED_PATH"
        ;;
    esac
}

set -x

BUILD="$(mktemp -d)"
cd "$BUILD" || exit 1
mkdir "$WORKSPACE"/action

case "${1:-}" in
make)
    cmake "$WORKSPACE"
    make -j
    cp "$WORKSPACE/src/LICENSE" output/lpac-LICENSE
    cp "$WORKSPACE/euicc/LICENSE" output/libeuicc-LICENSE
    cp "$WORKSPACE/cjson/LICENSE" output/cjson-LICENSE
    cp "$WORKSPACE/dlfcn-win32/LICENSE" output/dlfcn-win32-LICENSE
    zip -r -j "$WORKSPACE/action/lpac-$KERNEL-$MATCHINE.zip" output/*
    ;;
debian)
    cmake "$WORKSPACE" -DCPACK_GENERATOR=DEB
    make -j package
    cp lpac_*.deb "$WORKSPACE"/action
    ;;
mingw)
    cmake "$WORKSPACE" -DCMAKE_TOOLCHAIN_FILE=./cmake/linux-mingw64.cmake
    make -j
    CURL="$(download "$MINGW_CURL_WIN64_BLOB")"
    cp "$CURL"/curl-*-mingw/bin/libcurl-x64.dll output/libcurl.dll
    cp "$CURL"/curl-*-mingw/COPYING.txt output/libcurl-LICENSE
    cp "$WORKSPACE/src/LICENSE" output/lpac-LICENSE
    cp "$WORKSPACE/euicc/LICENSE" output/libeuicc-LICENSE
    cp "$WORKSPACE/cjson/LICENSE" output/cjson-LICENSE
    cp "$WORKSPACE/dlfcn-win32/LICENSE" output/dlfcn-win32-LICENSE
    zip -r -j "$WORKSPACE/action/lpac-windows-x86_64-mingw.zip" output/*
    ;;
woa-mingw)
    TOOLCHAIN="$(download "$MINGW32_TOOLCHAIN_BLOB")"
    cmake "$WORKSPACE" -DCMAKE_TOOLCHAIN_FILE=./cmake/linux-mingw64-woa.cmake "-DTOOLCHAIN_BIN_PATH=$TOOLCHAIN/bin"
    make -j
    CURL="$(download "$MINGW_CURL_WIN64A_BLOB")"
    cp "$CURL"/curl-*-mingw/bin/libcurl-arm64.dll output/libcurl.dll
    cp "$CURL"/curl-*-mingw/COPYING.txt output/libcurl-LICENSE
    cp "$WORKSPACE/src/LICENSE" output/lpac-LICENSE
    cp "$WORKSPACE/euicc/LICENSE" output/libeuicc-LICENSE
    cp "$WORKSPACE/cjson/LICENSE" output/cjson-LICENSE
    cp "$WORKSPACE/dlfcn-win32/LICENSE" output/dlfcn-win32-LICENSE
    zip -r -j "$WORKSPACE/action/lpac-windows-arm64-mingw.zip" output/*
    ;;
woa-zig)
    cmake "$WORKSPACE" -DCMAKE_TOOLCHAIN_FILE=./cmake/aarch64-windows-zig.cmake
    make -j
    CURL="$(download "$MINGW_CURL_WIN64A_BLOB")"
    cp "$CURL"/curl-*-mingw/bin/libcurl-arm64.dll output/libcurl.dll
    cp "$CURL"/curl-*-mingw/COPYING.txt output/libcurl-LICENSE
    cp "$WORKSPACE/src/LICENSE" output/lpac-LICENSE
    cp "$WORKSPACE/euicc/LICENSE" output/libeuicc-LICENSE
    cp "$WORKSPACE/cjson/LICENSE" output/cjson-LICENSE
    cp "$WORKSPACE/dlfcn-win32/LICENSE" output/dlfcn-win32-LICENSE
    zip -r -j "$WORKSPACE/action/lpac-windows-arm64-zig.zip" output/*
    ;;
*)
    echo "Usage: $0 {make,debian,mingw,woa-mingw,woa-zig}"
    exit 1
    ;;
esac

rm -rf "$BUILD"
