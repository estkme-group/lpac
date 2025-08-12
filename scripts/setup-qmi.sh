#!/bin/bash
# This script is only for GitHub Actions use

set -xeuo pipefail

sudo apt-get -qq --no-install-recommends install -y libqrtr-glib-dev

function install() {
    local URL="https://launchpad.net/ubuntu/+archive/primary/+files/$1"
    local NAME="$(basename "$URL")"
    local TMPFILE="$(mktemp)"
    curl -L "$1" -o "$TMPFILE"
    dpkg -i "$TMPFILE"
    rm -rf "$TMPFILE"
}

VERSION="1.36.0-1_$(dpkg --print-architecture)"

install "libqmi-glib5_$VERSION.deb"
install "gir1.2-qmi-1.0_$VERSION.deb"
install "libqmi-glib-dev_$VERSION.deb"
install "libqmi-utils_$VERSION.deb"
install "libqmi-proxy_$VERSION.deb"
