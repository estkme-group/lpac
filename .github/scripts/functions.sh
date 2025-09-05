#!/bin/bash
# This script is only for GitHub Actions use
set -euo pipefail

KERNEL="$(uname -s)"
MACHINE="$(uname -m)"

export KERNEL MACHINE

export WORKSPACE="${GITHUB_WORKSPACE:-$(pwd)}"
export CURL_VERSION="8.6.0_1"
export MINGW_CURL_WIN64_BLOB="https://curl.se/windows/dl-$CURL_VERSION/curl-$CURL_VERSION-win64-mingw.zip"
export MINGW_CURL_WIN64A_BLOB="https://curl.se/windows/dl-$CURL_VERSION/curl-$CURL_VERSION-win64a-mingw.zip"

case "$KERNEL" in
Linux)
    KERNEL="linux"
    ;;
Darwin)
    KERNEL="darwin"
    MACHINE="universal"
    ;;
esac

function download {
    local URL SAVED_PATH SAVED_DIR
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
    *)
        echo "$SAVED_PATH"
        ;;
    esac
}

function copy-license {
    local OUTPUT="$1"

    mkdir "$OUTPUT/LICENSES"
    cp -r "$WORKSPACE/LICENSES" "$OUTPUT/LICENSES"
}

function copy-curl-woa {
    local OUTPUT="$1"

    CURL="$(download "$MINGW_CURL_WIN64A_BLOB")"
    cp "$CURL"/curl-*-mingw/bin/libcurl-arm64.dll "$OUTPUT/libcurl.dll"
    cp "$CURL"/curl-*-mingw/COPYING.txt "$OUTPUT/LICENSE-libcurl"
    rm -rf "$CURL"
}

function copy-curl-win {
    local OUTPUT="$1"

    CURL="$(download "$MINGW_CURL_WIN64_BLOB")"
    cp "$CURL"/curl-*-mingw/bin/libcurl-x64.dll "$OUTPUT/libcurl.dll"
    cp "$CURL"/curl-*-mingw/COPYING.txt "$OUTPUT/LICENSE-libcurl"
    rm -rf "$CURL"
}

function copy-usage {
    local OUTPUT="$1"

    cp "$WORKSPACE/docs/USAGE.md" "$OUTPUT/README.md"
}

function create-bundle {
    local BUNDLE_FILE="$1"
    local INPUT_DIR="$2"

    pushd "$INPUT_DIR"
    zip -r "$BUNDLE_FILE" ./*
    popd
}
