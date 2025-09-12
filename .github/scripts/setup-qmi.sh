#!/bin/bash
# This script is only for GitHub Actions use
set -euo pipefail

function apt() {
    sudo DEBIAN_PRIORITY=critical DEBIAN_FRONTEND=noninteractive \
        apt-get -qq -o=Dpkg::Use-Pty=0 "$@"
}

apt install -y meson ninja-build \
    libgirepository1.0-dev libglib2.0-dev libgudev-1.0-dev \
    libqrtr-glib-dev libmbim-glib-dev

# QMI_REPO_URL="https://gitlab.freedesktop.org/mobile-broadband/libqmi.git" # official repo
QMI_REPO_URL="https://github.com/linux-mobile-broadband/libqmi" # official mirror repo
QMI_REPO_DIR="$(mktemp -d libqmi-src-XXXXXX)"
QMI_BUILD_DIR="$QMI_REPO_DIR/build"
QMI_VERSION=1.36.0

trap 'rm -rf '"$QMI_REPO_DIR" EXIT

pushd "$QMI_REPO_DIR"
wget -nv "$QMI_REPO_URL/archive/refs/tags/$QMI_VERSION.tar.gz" -O - |
    tar xz --strip-components=1
meson setup "$QMI_BUILD_DIR" --prefix=/usr --buildtype=release \
    -Dman=false -Dbash_completion=false
pushd "$QMI_BUILD_DIR"
ninja -j"$(nproc)"
sudo ninja install
popd
popd
