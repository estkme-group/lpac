#!/bin/bash
# This script is only for GitHub Actions use

set -xeuo pipefail

apt-get -qq --no-install-recommends install -y libqrtr-glib-dev libmbim-glib-dev

TMPDIR="$(mktemp -d)"

function download() {
    local NAME="$1"
    curl -L "https://launchpad.net/ubuntu/+archive/primary/+files/$NAME" -o "$TMPDIR/$NAME"
}

# https://launchpad.net/libqmi
VERSION="1.36.0-1_$(dpkg --print-architecture)"

download "libqmi-glib5_$VERSION.deb"
download "libqmi-glib-dev_$VERSION.deb"
download "libqmi-utils_$VERSION.deb"
download "libqmi-proxy_$VERSION.deb"
download "gir1.2-qmi-1.0_$VERSION.deb"

dpkg -i "$TMPDIR"/*
