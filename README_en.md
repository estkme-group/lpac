# LPAC

[中文](README.md)

LPAC is a cross platform local profile agent program. Committed to becoming the most compatible, powerful, and concise LPA program.

Download: 
[Github Release](https://github.com/estkme-group/lpac/releases/latest)

Join our [Telegram Group](https://t.me/estkme)to discuss!

# Usage

## Linux pcscd
You need to install `pcscd` and keep its daemon running to access pcsc smart card reader.
```bash
sudo apt install pcscd
sudo systemctl start pcscd
```

## GUI Frontend

- [lpa-agent](https://github.com/estkme-group/lpa-agent): Web based lpac frontend

# Compile

## Linux
Debian/Ubuntu
```bash
sudo apt install build-essential cmake git g++ libpcsclite-dev libcurl4-openssl-dev
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac && mkdir build && cd build 
cmake .. 
make
```

## macOS

WIP

## Windows

- Build on Linux(MINGW)

```bash
sudo apt install build-essential cmake git g++ libpcsclite-dev libcurl4-openssl-dev gcc-mingw-w64 g++-mingw-w64
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac && mkdir build && cd build
cmake -DLINUX_MINGW32=ON .. && make
# Download libcurl
wget https://curl.se/windows/dl-8.4.0_6/curl-8.4.0_6-win64-mingw.zip -O curl.zip && unzip curl.zip && mv curl-8.4.0_6-win64-mingw/bin/libcurl-x64.dll output/libcurl.dll
```

- Build on Windows(MSYS2)

```bash
# MSYS2 MINGW64 system
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-gcc
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac && mkdir build && cd build
cmake -DLINUX_MINGW32=ON .. && ninja
# Download libcurl
wget https://curl.se/windows/dl-8.4.0_6/curl-8.4.0_6-win64-mingw.zip -O curl.zip && unzip curl.zip && mv curl-8.4.0_6-win64-mingw/bin/libcurl-x64.dll output/libcurl.dll
```