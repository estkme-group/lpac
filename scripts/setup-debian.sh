#!/bin/bash
# This script is only for GitHub Actions use
set -euo pipefail

SCRIPT_DIR="$(dirname "${BASH_SOURCE[0]}")"

function apt() {
    sudo DEBIAN_PRIORITY=critical DEBIAN_FRONTEND=noninteractive \
        apt-get -qq -o=Dpkg::Use-Pty=0 "$@"
}

apt update
apt install -y build-essential libpcsclite-dev libcurl4-openssl-dev zip

function setup-mingw-woarm64() {
    BASE_URL="https://github.com/Windows-on-ARM-Experiments/mingw-woarm64-build"
    FILENAME="aarch64-w64-mingw32-msvcrt-toolchain.tar.gz"
    VERSION="2024-02-08"

    SAVED_PATH="$(mktemp --suffix .tar.gz)"
    SAVED_DIR="$(mktemp -d)"

    wget -nv "$BASE_URL/releases/download/$VERSION/$FILENAME" -O "$SAVED_PATH"
    tar -C "$SAVED_DIR" --gzip --extract --file="$SAVED_PATH"

    echo "$SAVED_DIR/bin" >> "$GITHUB_PATH"
}

case "${1:-}" in
woa-mingw)
    setup-mingw-woarm64
    ;;
make-qmi)
    exec "$SCRIPT_DIR/setup-qmi.sh"
    ;;
mingw)
    apt install -y gcc-mingw-w64 g++-mingw-w64
    ;;
esac
