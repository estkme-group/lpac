#!/bin/bash
set -xeuo pipefail
sudo apt-get update
sudo apt-get install -y libpcsclite-dev libcurl4-openssl-dev gcc make cmake gcc-mingw-w64 g++-mingw-w64 unzip ninja-build
sudo snap install zig --classic --beta
