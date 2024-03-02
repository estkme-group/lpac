# lpac


lpac is a cross platform local profile agent program.

Features:
- Support Activate Code and Confirm Code
- Support custom IMEI sent to server
- Support Profile Discovery (SM-DS)
- Profile management: list, enable, disable, delete and nickname
- Notification management: list, send and delete
- Lookup eUICC chip info
- etc

Download: 
[Github Release](https://github.com/estkme-group/lpac/releases/latest)

lpac is dynamic-linked, Release is built by Github action, if you can't run it you need to compile by yourself

## GUI Frontend

- [EasyLPAC](https://github.com/creamlike1024/EasyLPAC)

# Compile

<details>
<summary>Linux</summary>

- Debian/Ubuntu

```bash
sudo apt install build-essential cmake git g++ libpcsclite-dev libcurl4-openssl-dev
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac && mkdir build && cd build 
cmake ..
make
```

then execute `./output/lpac` to use.

- Droidian

Same as normal Debian/Ubuntu, however, in order to build the GBinder backends, you will need `libgbinder-dev`, `glib2.0-dev`, and you will have to set `-DLPAC_APDU_INTERFACE_GBINDER=ON` when invoking `cmake`.

</details>

<details>
<summary>macOS</summary>

- Install [Homebrew](https://brew.sh/).

```bash
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac && mkdir build && cd build 
cmake .. 
make
```

</details>

<details>
<summary>Windows(x86_64)</summary>

Windows need prebuilt libcurl.dll, you can replace the download link to newest curl version.

- Build on Linux(MINGW)

```bash
sudo apt install build-essential cmake git g++ libpcsclite-dev libcurl4-openssl-dev gcc-mingw-w64 g++-mingw-w64
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac && mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=./cmake/linux-mingw64.cmake .. && make
# Download libcurl
wget https://curl.se/windows/dl-8.4.0_6/curl-8.4.0_6-win64-mingw.zip -O curl.zip && unzip curl.zip && mv curl-8.4.0_6-win64-mingw/bin/libcurl-x64.dll output/libcurl.dll
```

- Build on Windows(MSYS2)

```bash
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-gcc
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac && mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=./cmake/linux-mingw64.cmake .. && ninja
# Download libcurl
wget https://curl.se/windows/dl-8.4.0_6/curl-8.4.0_6-win64-mingw.zip -O curl.zip && unzip curl.zip && mv curl-8.4.0_6-win64-mingw/bin/libcurl-x64.dll output/libcurl.dll
```

- Build on Windows(Cygwin)

With `gcc-core` `gcc-g++` `make` `cmake` `unzip` `wget` installed

```bash
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac && mkdir build && cd build
cmake .. && make
# Download libcurl
wget https://curl.se/windows/dl-8.4.0_6/curl-8.4.0_6-win64-mingw.zip -O curl.zip && unzip curl.zip && mv curl-8.4.0_6-win64-mingw/bin/libcurl-x64.dll output/libcurl.dll
```
To run it outside Cygwin shell, you need copy `cygwin1.dll` to the program folder to distribute.
`cygwin1.dll` is located in `C:\cygwin64\bin\cygwin1.dll` (Default Cygwin installation location)

</details>

<details>
<summary>Windows on ARM</summary>

- Cross compile on Windows/Linux host(arm64,x86_64 and more architecture) with zig

Install [zig](https://ziglang.org/download/)

```bash
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac && mkdir build-woa-zig && cd build-woa-zig && cmake .. -GNinja -DCMAKE_TOOLCHAIN_FILE=./cmake/aarch64-windows-zig.cmake && ninja
# Download libcurl
wget https://curl.se/windows/dl-8.6.0_1/curl-8.6.0_1-win64a-mingw.zip -O curl.zip && unzip curl.zip && mv curl-8.6.0_1-win64a-mingw/bin/libcurl-arm64.dll output/libcurl.dll
```

- Cross compile on Linux x86_64 host(GNU toolchain)

```bash
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac && mkdir woa-gnu-cross-toolchain && cd woa-gnu-cross-toolchain
wget https://github.com/Windows-on-ARM-Experiments/mingw-woarm64-build/releases/download/2024-02-08/aarch64-w64-mingw32-msvcrt-toolchain.tar.gz && tar xf aarch64-w64-mingw32-msvcrt-toolchain.tar.gz
cd ../ && mkdir build-woa-mingw && cd build-woa-mingw && cmake .. -GNinja -DCMAKE_TOOLCHAIN_FILE=./cmake/linux-mingw64-woa.cmake -DTOOLCHAIN_BIN_PATH=<path-to-cross-toolchain-bin> && ninja
wget https://curl.se/windows/dl-8.6.0_1/curl-8.6.0_1-win64a-mingw.zip -O curl.zip && unzip curl.zip && mv curl-8.6.0_1-win64a-mingw/bin/libcurl-arm64.dll output/libcurl.dll
```

- Build on Windows(MSYS2)

It is possible to build on **WoA devices** with [MSYS2 ARM64 Support](https://www.msys2.org/wiki/arm64/)

You may need to install `mingw-w64-clang-aarch64-cmake`, `mingw-w64-clang-aarch64-ninja`,`mingw-w64-clang-aarch64-clang` and modify `cmake/linux-mingw64.cmake`(replace toolchain).

Download prebuilt curl dll is also needed. Refer to the previous compilation steps.

</details>

# Usage

In Linux, you need install `pcscd`, `pcsclite` and `libcurl`.

APDU and HTTP interfaces of lpac has serval backends, you need specify `$APDU_INTERFACE` and `$HTTP_INTERFACE` environment variable to interface library path. If not specified, it will use `libapduinterface_pcsc` and `libhttpinterface_curl`.

APDU Backends:
- `libapduinterface_at`: use AT commands interface used by LTE module
- `libapduinterface_pcsc`: use PC/SC Smart Card API
- `libapduinterface_stdio`: use standard input/output
- GBinder-based backends for `libhybris` (Halium) distributions:
  - `libapduinterface_gbinder_hidl`: use HIDL IRadio (SoC launched before Android 13)

Using `libapduinterface_at` need access permission to serial port (normally `/dev/ttyUSBx`). On Arch Linux, you can add yourself to `uucp` group by `sudo usermod -aG uucp $USER`. On other distro, you may need add yourself into `dialout` group.

HTTP Backends:
- `libhttpinterface_curl`: use libcurl
- `libhttpinterface_stdio`: use standard input/ouput

## CLI

### Command format
```
lpac <subcommand> [subcommand] [parameters]
  subcommand:
    chip          View and manage information about your eUICC card itself
    profile       Manage the profile of your eUICC card
    notification  Manage notifications within your eUICC card
    driver        View libXXXXinterface info
  subcommand 2:
    Please refer to the detailed instructions below
```

### Return value

The return contents of lpac instructions are all in json format, and the returns of all instructions comply with the following format.
```json
{
   "type": "lpa",
   "payload": {
      "code": 0,
      "message": "success",
      "data": {[]}
   }
}
```

- `"type": "lpa"`: fixed content
- `code`: if is 0, indicating successful execution, and other values are error codes.
- `message`: is success if the operation is successful, or the error type is returned if an error occurs.
- `data`: returns the returned content when the operation is successful, and is empty (but not NULL) when there is an error.

### Subcommand

#### chip

View the EID, default SM-DP+ server and SM-DS server of eUICC. euicc_info2 is also supported.

```
lpac chip <subcommand> [parameters]
  subcommand:
    info         View information about your eUICC card itself
    defaultsmdp  Modify the default SM-DP+ server address of your eUICC card
                 Example: lpac chip defaultsmdp <the address of the SM-DP+ server you want to modify>
    purge        Reset the eUICC and will clear all profiles. Use with caution!
```

<details>

<summary>Return value example</summary>

```json
{
  "type": "lpa",
  "payload": {
    "code": 0,
    "message": "success",
    "data": {
      "eidValue": "[EID]",
      "EuiccConfiguredAddresses": {
        "defaultDpAddress": null,
        "rootDsAddress": "testrootsmds.gsma.com"
      },
      "EUICCInfo2": {
        "profileVersion": "2.1.0",
        "svn": "2.2.0",
        "euiccFirmwareVer": "4.6.0",
        "extCardResource": {
          "installedApplication": 0,
          "freeNonVolatileMemory": 291666,
          "freeVolatileMemory": 5970
        },
        "uiccCapability": [
          "usimSupport",
          "isimSupport",
          "csimSupport",
          "akaMilenage",
          "akaCave",
          "akaTuak128",
          "akaTuak256",
          "gbaAuthenUsim",
          "gbaAuthenISim",
          "eapClient",
          "javacard",
          "multipleUsimSupport",
          "multipleIsimSupport"
        ],
        "javacardVersion": "9.2.0",
        "globalplatformVersion": "2.3.0",
        "rspCapability": [
          "additionalProfile",
          "testProfileSupport"
        ],
        "euiccCiPKIdListForVerification": [
          "81370f5125d0b1d408d4c3b232e6d25e795bebfb"
        ],
        "euiccCiPKIdListForSigning": [
          "81370f5125d0b1d408d4c3b232e6d25e795bebfb"
        ],
        "euiccCategory": null,
        "forbiddenProfilePolicyRules": [
          "pprUpdateControl",
          "ppr1"
        ],
        "ppVersion": "0.0.1",
        "sasAcreditationNumber": "GI-BA-UP-0419",
        "certificationDataObject": {
          "platformLabel": "1.2.840.1234567/myPlatformLabel",
          "discoveryBaseURL": "https://mycompany.com/myDLOARegistrar"
        }
      }
    }
  }
}
```
</details>

#### profile

Profile management, you can list, set alias (nickname), enable, disable, delete, download and discovery Profiles.
```
lpac profile <subcommand> [parameters]
  subcommand:
    list      enumerates your eUICC Profile
    nickname  sets an alias for the specified Profile
              Example: lpac profile nickname <ICCID of Profile> <alias>
    enable    enables the specified Profile. The RefreshFlag status is enabled by default and can be omitted.
              Example: lpac profile enable <ICCID/AID of Profile> [1/0]
    disable   disables the specified Profile. The RefreshFlag state is enabled by default and can be omitted.
              Example: lpac profile disable <ICCID/AID of Profile> [1/0]
    delete    deletes the specified Profile
              Example: lpac profile delete <ICCID/AID of Profile>
    download  Download profile from SM-DP server
    discovery Detect available profile registered on SM-DS server
```

> [!NOTE]
> Some eUICC chip have trouble when enable profile (e.g. These removeable eUICC cards from ECP), try AID, ICCID, refreshFlag with 1 or 0 to find out the working way for these chips.

There is no secondary confirmation for deleting a Profile, so please perform it with caution.
> [!NOTE]
> This function will only delete the Profile and issue a Notification, but it will not be sent automatically. You need to send it manually.

##### Download requires connection to SM-DP+ server and the following additional parameters:
- `-s`: SM-DP+ server, optional, if not provided, it will try to read the defaultsmdp attribute.
- `-m`: Matching ID, activation code. optional.
- `-c`: Confirmation Code, optional.
- `-i`: The IMEI of the device to which Profile is to be downloaded, optional.


<details>

<summary>Example</summary>

```bash
./lpac profile download -s rsp.truphone.com -m "QR-G-5C-1LS-1W1Z9P7"
```

</details>

##### Discovery requires connecting to the SM-DS server to query registered profile

The following parameters can be used to customize the IMEI and SM-DS server:
- `-s`: SM-DS server. If not provided, it will be gsma official server "lpa.ds.gsma.com"
- `-i`: IMEI of the device to which Profile is to be downloaded, optional

<details>

<summary>Return value example of lpac profile list</summary>

```json
{
  "type": "lpa",
  "payload": {
    "code": 0,
    "message": "success",
    "data": [
      {
        "iccid": "89353...",
        "isdpAid": "A0000005591010FFFFFFFF8900001000",
        "profileState": "disabled",
        "profileNickname": null,
        "serviceProviderName": "Vodafone IE",
        "profileName": "Vodafone IE eSIM",
        "iconType": "png",
        "icon": "iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAMAAACdt4HsAAAAMFBMVEUAAADgp6booKDoeXfrZ2Xn0tLsUE3sOjbtLCjtHhnvoaDpwsLvdHH1y8r57u7///9RHT9XAAAADHRSTlMASX2gyyX3/////lKi+t6IAAACOUlEQVR42pWX27KsIAxE5SLpIOD//+0ua86ZNtSI0A++9SJprm6Pcj7Ej0LwblvT7mMSAKrXB4Ck6Pdpu49yOeWri6SQfMzZExRCQSEl19ZaLfEdcSRV49aUazsv5Ysc3bj3YOyiKPX8qH7qUoTx8DD23M5/yor/NcXHND1s9aWd9N+48pBE6Iav51e1a8xP+FOjvwnkLqh/9Rfa2cCI4K0/n3exABKOLn88jm8TYJK7mX9R0z/N7KAnxDsgqoGfVkXlh+4xHDaA1gG4vO4C2ERSG2CnWkQxasLf/WADVKsF2gOA42cBlT5bhuKhhGMwAwbRFwE4TsGoAK7ojqDhswZgE5gnIDFCTsGQAFiCW+iAq9r2sCfTQTvHKtrPg4NQ4C6aKgFp37wB5PNNyabgtoC3CEZbU73NEO+ABtNDWAcIOgAsYC0E7QAyASgjAFYBEi1A8wxgFGJZBwQDSNMhEmDntS2uA8+lzBBW9qPbnMhSD0W7zbQlW0Jd6UAjDxSWsLSXeKQxhYUzDY6HKi/uNn8gJR7rFOS9AXbAi4VSaXN+yM6rzRDQHvz4cbUxRgqoP/IrDzcbU7BvxM6eRcVKg30hWamW2owdg1cSt6RBSMk515pzgmL0xGGOVrh8EKW7T5BygCxJ0z54aE74cdC7RuBDdUBYHJ8EXfZbHaJz+dHfE+KojcEvDxWgL3bx21BHhA7sCBx+hMDD6PHYZuRCggLWrEgc/VW7C1FEAf57Bw4+DfHh+/f/PPYfW1Z8RhpATXMAAAAASUVORK5CYII=",
        "profileClass": "operational"
      },
      {
        "iccid": "89012...",
        "isdpAid": "A0000005591010FFFFFFFF8900001100",
        "profileState": "disabled",
        "profileNickname": null,
        "serviceProviderName": "T-Mobile",
        "profileName": "CONVSIM5G_Adaptive",
        "iconType": "png",
        "icon": "iVBORw0KGgoAAAANSUhEUgAAADkAAABABAMAAABSG8K/AAAAMFBMVEXcAVrgir3VTpvXKYvXA33NGH7uAY7qAXztAYrvzebkcbL7/f3jq8/EGXHaMpThAW+lJAaXAAAACXBIWXMAAA7EAAAOxAGVKw4bAAACIklEQVR4nL3UMWsUQRQA4O1UtEhtKrGSFCrXCEIsRM9OEX/AFeHt6w6i3GxnCPr2VZ69WFgJgmDOHEfYmAVbCysLsVSIlaRyQZFz5s3s7szcJoWKrzh25pvZe/NmdhI+JEDHIPkXSoDUpaQk5kpVfaXuq75tj1gUz7yw8Ux+T7vWc+YNrcMnRVeUKDro1sehvr1iHp7efmebr5xKY/dCNTZa5gcXF3TnVsZWEfKPsS4BoNN7vLEc6tZ3ZLCqh/EPL2f9+J6ASZY20zUitRxoH8AtfIamjj99nYyYAl33tSQK5w59PS/b49T8Ca9oJbMLelyfI4WT+n3g9BhGev1EsV3r1iieS5utTnBBrxbTWt8sqO6c2pwfmvQixUHxGkSvdWi6VuxloumH/XRhbv75jjlXgKhPK3h1Ng0E0zlIdLeGPMu8SmbmfDsFkzGGu6AJyc5VMjZQZqWcfuv1Vle/tJqf7en4aquxKUdw2ig9kI6JXdEjaWy3OpaOvUM095SOVL5rlbDOSv0ftVnNGkWn7OVcNjnXand/PaoVk1+r9CDRUbVz80Q67LkCG61KD+INURvotMT2fgtuwr/WWaAATVbjeq7tOFoh1GHx55qKQocikf4YC7l+ICM5742m+sDnp4zuHifz6XAwVw+tVmRvljLgWLP5r3OCxc7LeZVhoPip8ONSqHA50P1IbyZ+xG9G8AIxUva2BsyV6q8X2I+FWnXrb4Q67QSVBN1hAAAAAElFTkSuQmCC",
        "profileClass": "operational"
      },
      {
        "iccid": "89444...",
        "isdpAid": "A0000005591010FFFFFFFF8900001200",
        "profileState": "enabled",
        "profileNickname": null,
        "serviceProviderName": "BetterRoaming",
        "profileName": "BetterRoaming",
        "iconType": "none",
        "icon": null,
        "profileClass": "operational"
      },
      {
        "iccid": "89852...",
        "isdpAid": "A0000005591010FFFFFFFF8900001300",
        "profileState": "disabled",
        "profileNickname": null,
        "serviceProviderName": "Redtea Mobile",
        "profileName": "RedteaGO",
        "iconType": "none",
        "icon": null,
        "profileClass": "operational"
      }
    ]
  }
}
```

- `iccid`: ICCID of Profile
- `isdpAid`: Aid of Profile
- `profileState`: State of Profile, "Enabled" or "Disabled"
- `profileNickname`: Nickname of Profile
- `serviceProviderName`: Telecom operators of Profile
- `profileName`: Name of Profile
- `iconType`: Profile icon data struct, "none", "png", "jpg"
- `icon`: Profile icon data in base64
- `profileClass`: Type of Profile

</details>

#### notification

Used for the management of Notifications, which are sent by telecom operators during Profile operations. You can enumerate (list), send (process), and remove (remove) Notifications.
```
lpac notification <subcommand> [parameters]
  subcommand:
    list     Enumerates your eUICC pending Notification list
    process  Send Notification
             Example: lpac notification process <sequence ID>
    remove   Remove Notification
             Example: lpac notification remove <sequence ID>
```
> [!NOTE]
> Downstream developers or end users should process Notification as soon as possible when they exist to comply with GSMA specifications. lpac will not automatically delete the Notification after sending it, and you need to delete it manually.

<details>

<summary>Return value example of lpac notification list</summary>

```json
{
  "type": "lpa",
  "payload": {
    "code": 0,
    "message": "success",
    "data": [
      {
        "seqNumber": 178,
        "profileManagementOperation": "install",
        "notificationAddress": "rsp-eu.redteamobile.com",
        "iccid": "89852..."
      },
      {
        "seqNumber": 215,
        "profileManagementOperation": "disable",
        "notificationAddress": "cust-005-v4-prod-atl2.gdsb.net",
        "iccid": "89012..."
      },
      {
        "seqNumber": 216,
        "profileManagementOperation": "enable",
        "notificationAddress": "rsp.truphone.com",
        "iccid": "89444..."
      }
    ]
  }
}
```

- `seqNumber`: Sequence ID
- `profileManagementOperation`: Which operation generated this notification
- `notificationAddress`: Profile's notification reporting server address

</details>

#### driver

Now there is only one command: `lpac driver apdu list` to get card reader list.

## FAQ

<details>
<summary>Q: Any subcommand of lpac will get error message like "SCardListReaders() failed:" and 8-digits error code, such as 80100069, 80100066, 8010002E, 8010000F and so on.</summary>

A:
- 80100069 means your UICC is not plugged correctly
- 80100066 means your card has no response, please clean the pin and plug in again
- 8010002E means communication error
- 8010000F means the card is not a eUICC, or detect wrong card reader like Yubikey. For latter one, you can use `lpac driver apdu list` to list all reader and use `$DRIVER_IFID` to specify correct card reader
- for others, Google is your friend.

</details>

<details>
<summary>Q: I can't download eSIM profile of xxx.</summary>

A: The verification of SM-DP+ servers of telecom operators is diverse. Please check whether the parameters you enter are consistent with those provided to you by the telecom operators. Some telecom operators issue profiles in the form of push, which may require the use of lpac's discovery and custom IMEI function.

</details>

## Thanks

<a href="https://github.com/estkme-group/lpac/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=estkme-group/lpac"  alt=""/>
</a>

---

## License
- lpac (/src): AGPL-3.0
- libeuicc (/euicc): LGPL-v2
- interfaces (/interface): MIT

Copyright (c) 2023-2024 eSTKme Group
