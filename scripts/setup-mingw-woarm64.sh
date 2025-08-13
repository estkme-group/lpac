#!/usr/bin/env bash
# This script is only for GitHub Actions use
set -euo pipefail

BASE_URL="https://github.com/Windows-on-ARM-Experiments/mingw-woarm64-build"
FILENAME="aarch64-w64-mingw32-msvcrt-toolchain.tar.gz"
VERSION="2024-02-08"

SAVED_PATH="$(mktemp)"
SAVED_DIR="$(mktemp -d)"

wget -nv "$BASE_URL/releases/download/$VERSION/$FILENAME" -O "$SAVED_PATH"
tar -C "$SAVED_DIR" zxvf "$SAVED_PATH"

echo "$SAVED_DIR" >> "$GITHUB_PATH"
