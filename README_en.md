# LPAC

[中文](README.md)

LPAC is a cross platform local profile agent program.

Features:
- Support Activate Code and **Confirm Code**
- Support custom IMEI sent to server
- Support esim Discovery(Push)
- Notification manage
- Lookup eUICC chip info
- etc

Download: 
[Github Release](https://github.com/estkme-group/lpac/releases/latest)

lpac is dynamic-linked, Release is built by Github action, if you can't run it you need to compile by yourself

Join our [Telegram Group](https://t.me/estkme) to discuss!

# Usage

## Linux dependency
You need to install `pcscd` and keep its daemon running to access pcsc smart card reader.
```bash
sudo apt install pcscd
sudo systemctl start pcscd
```

`libpcsclite` is alse needed


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

Due to a [bug in asn1c](https://github.com/vlm/asn1c/issues/196), the MinGW version of lpac cannot send notification successfully. Building with Cygwin seems to avoid it.

Windows need prebuilt libcurl.dll, you can replace the download link to newest curl version.

- Build on Windows(Cygwin)

With `gcc-core` `gcc-g++` `make` `cmake` `unzip` `wget` installed

```bash
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac && mkdir build && cd build
cmake -DCYGWIN=ON .. && make
# Download libcurl
wget https://curl.se/windows/dl-8.4.0_6/curl-8.4.0_6-win64-mingw.zip -O curl.zip && unzip curl.zip && mv curl-8.4.0_6-win64-mingw/bin/libcurl-x64.dll output/libcurl.dll
```
To run it outside Cygwin shell, you need copy `cygwin1.dll` to the program folder to distribute.
`cygwin1.dll` is located in `C:\cygwin64\bin\cygwin1.dll` (Default Cygwin installation location)

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