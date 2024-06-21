#!/bin/bash
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive
export DEBIAN_PRIORITY=critical

apt-get -qq -o=Dpkg::Use-Pty=0 update
apt-get -qq -o=Dpkg::Use-Pty=0 install -y build-essential libpcsclite-dev libcurl4-openssl-dev zip

case "${1:-}" in
mingw)
    apt-get -qq -o=Dpkg::Use-Pty=0 install gcc-mingw-w64 g++-mingw-w64
    ;;
esac
