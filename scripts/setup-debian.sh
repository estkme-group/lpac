#!/bin/bash
# This script is only for GitHub Actions use
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)

export DEBIAN_FRONTEND=noninteractive
export DEBIAN_PRIORITY=critical

apt-get -qq -o=Dpkg::Use-Pty=0 update
apt-get -qq -o=Dpkg::Use-Pty=0 install -y build-essential libpcsclite-dev libcurl4-openssl-dev zip

case "${1:-}" in
make-qmi)
    "$SCRIPT_DIR/setup-qmi.sh"
    ;;
mingw)
    apt-get -qq -o=Dpkg::Use-Pty=0 install gcc-mingw-w64 g++-mingw-w64
    ;;
esac
