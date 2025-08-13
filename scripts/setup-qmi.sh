#!/bin/bash
# This script is only for GitHub Actions use
set -euo pipefail

function apt() {
    sudo DEBIAN_PRIORITY=critical DEBIAN_FRONTEND=noninteractive \
        apt-get -qq -o=Dpkg::Use-Pty=0 "$@"
}

apt install -y libqrtr-glib-dev libmbim-glib-dev

TMPDIR="$(mktemp -d)"

trap 'rm -vrf '"$TMPDIR" EXIT

# https://launchpad.net/libqmi
QMI_VERSION="1.36.0-1_$(dpkg --print-architecture)"

wget -nv -P "$TMPDIR" -i - <<EOF
https://launchpad.net/ubuntu/+archive/primary/+files/libqmi-glib5_$QMI_VERSION.deb
https://launchpad.net/ubuntu/+archive/primary/+files/libqmi-glib-dev_$QMI_VERSION.deb
https://launchpad.net/ubuntu/+archive/primary/+files/libqmi-utils_$QMI_VERSION.deb
https://launchpad.net/ubuntu/+archive/primary/+files/libqmi-proxy_$QMI_VERSION.deb
https://launchpad.net/ubuntu/+archive/primary/+files/gir1.2-qmi-1.0_$QMI_VERSION.deb
EOF

sudo dpkg -i "$TMPDIR"/*
