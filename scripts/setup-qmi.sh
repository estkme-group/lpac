#!/bin/bash
# This script is only for GitHub Actions use

set -xeuo pipefail

apt -qq -o=Dpkg::Use-Pty=0 install meson ninja-build pkg-config help2man \
    libgirepository1.0-dev libglib2.0-dev libgudev-1.0-dev libqrtr-glib-dev

TMPDIR="$(mktemp -d)"
git clone --depth 1 https://gitlab.freedesktop.org/mobile-broadband/libqmi.git "$TMPDIR"
cd "$TMPDIR" || exit 1
meson setup build --prefix=/usr --buildtype=debug -Dmbim_qmux=false
ninja -C build
ninja -C build install
