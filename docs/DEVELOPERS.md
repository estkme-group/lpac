# Developer Manual

## Coding Standard

lpac is written with C99 and compatible with [SGP.22 version 2.2.2](https://www.gsma.com/solutions-and-impact/technologies/esim/wp-content/uploads/2020/06/SGP.22-v2.2.2.pdf).

## How to Compile

### Linux

#### Debian/Ubuntu

``` bash
# clone this repo in the top-level folder

# install dependencies
./scripts/setup-debian.sh 
```

Run `cmake` and `make`, compiled output will be in `output` directory.  
If you want to get a Deb package, run `cmake -DCPACK_GENERATOR=DEB` then `make`.

#### Droidian

Same as normal Debian/Ubuntu, however, in order to build the GBinder backends, you will need `libgbinder-dev`, `glib2.0-dev`, and you will have to pass `-DLPAC_WITH_APDU_GBINDER=ON` when invoking `cmake`.

---

### macOS

Install [Homebrew](https://brew.sh/).  
Run `cmake` and `make`, compiled output will be in `output` directory.  

---

### Windows(x86_64)

Windows need libcurl.dll to run.  
Download libcurl from <https://curl.se/download.html> and place it as `libcurl.dll` in `output` directory.  

Install prerequisites, run `cmake` and `make`, compiled output will be in `output` directory.  

#### Build on Linux(MINGW)

Require `build-essential` `cmake` `git` `g++` `libpcsclite-dev` `libcurl4-openssl-dev` `gcc-mingw-w64` `g++-mingw-w64` installed.  

#### Build on Windows(MSYS2)

Require `mingw-w64-x86_64-cmake` `mingw-w64-x86_64-gcc` installed.  

#### Build on Windows(Cygwin)

Require `gcc-core` `gcc-g++` `make` `cmake` `unzip` `wget` installed.  

To run it outside Cygwin shell, you need copy `cygwin1.dll` to the program folder to distribute.  
`cygwin1.dll` is located in `C:\cygwin64\bin\cygwin1.dll` (Default Cygwin installation location)

---

### Windows on ARM

#### Cross compile on Windows/Linux host(arm64,x86_64 and more architecture) with zig

Install [zig](https://ziglang.org/download/)

```bash
# clone this repo in the top-level folder

./scripts/build-woa.sh zig
```

#### Cross compile on Linux x86_64 host(GNU toolchain)

```bash
# clone this repo in the top-level folder

./scripts/build-woa.sh mingw
```

#### Build on Native Windows on ARM(MSYS2)

It is possible to build on **WoA devices** with [MSYS2 ARM64 Support](https://www.msys2.org/wiki/arm64/)

You may need to install `mingw-w64-clang-aarch64-cmake`, `mingw-w64-clang-aarch64-clang` and modify `cmake/linux-mingw64.cmake`(replace toolchain).

Download prebuilt curl dll is also needed. Refer to the previous compilation steps.

## Debug

Please see [debug environment variables](ENVVARS.md#debug)
