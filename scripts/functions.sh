#!/bin/bash
export KERNEL="$(uname -s)"
export MATCHINE="$(uname -m)"

export WORKSPACE="${GITHUB_WORKSPACE:-$(pwd)}"
export CURL_VERSION="8.6.0_1"
export WOA_TOOLCHAIN_VERSION="2024-02-08"
export MINGW_CURL_WIN64_BLOB="https://curl.se/windows/dl-$CURL_VERSION/curl-$CURL_VERSION-win64-mingw.zip"
export MINGW_CURL_WIN64A_BLOB="https://curl.se/windows/dl-$CURL_VERSION/curl-$CURL_VERSION-win64a-mingw.zip"
export MINGW32_TOOLCHAIN_BLOB="https://github.com/Windows-on-ARM-Experiments/mingw-woarm64-build/releases/download/$WOA_TOOLCHAIN_VERSION/aarch64-w64-mingw32-msvcrt-toolchain.tar.gz"

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
    local URL="$1"
    local SAVED_PATH="$(mktemp)"
    local SAVED_DIR="$(mktemp -d)"
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

function copy-license {
    local OUTPUT="$1"

    cp "$WORKSPACE/src/LICENSE" "$OUTPUT/lpac-LICENSE"
    cp "$WORKSPACE/euicc/LICENSE" "$OUTPUT/libeuicc-LICENSE"
    cp "$WORKSPACE/cjson/LICENSE" "$OUTPUT/cjson-LICENSE"
    cp "$WORKSPACE/dlfcn-win32/LICENSE" "$OUTPUT/dlfcn-win32-LICENSE"
}

function copy-curl-woa {
    local OUTPUT="$1"

    CURL="$(download "$MINGW_CURL_WIN64A_BLOB")"
    cp "$CURL"/curl-*-mingw/bin/libcurl-arm64.dll "$OUTPUT/libcurl.dll"
    cp "$CURL"/curl-*-mingw/COPYING.txt "$OUTPUT/libcurl-LICENSE"
    rm -rf "$CURL"
}

function copy-curl-win {
    local OUTPUT="$1"

    CURL="$(download "$MINGW_CURL_WIN64_BLOB")"
    cp "$CURL"/curl-*-mingw/bin/libcurl-x64.dll "$OUTPUT/libcurl.dll"
    cp "$CURL"/curl-*-mingw/COPYING.txt "$OUTPUT/libcurl-LICENSE"
    rm -rf "$CURL"
}

function copy-docs {
    local OUTPUT="$1"

    cp "$WORKSPACE/README.md" "$OUTPUT/README.md"
    cp -r "$WORKSPACE/docs" "$OUTPUT/docs"
}
