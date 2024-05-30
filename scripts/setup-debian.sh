#!/bin/bash
set -xeuo pipefail
sudo apt-get update
sudo apt-get install -y build-essential libpcsclite-dev libcurl4-openssl-dev gcc-mingw-w64 g++-mingw-w64 zip
