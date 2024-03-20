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
- [{Open,Easy}EUICC](https://gitea.angry.im/PeterCxy/OpenEUICC) ([Mirror](https://github.com/estkme-group/openeuicc))

## Compile

<details>
<summary>Linux</summary>

- Debian/Ubuntu

```bash
sudo apt install build-essential cmake git g++ libpcsclite-dev libcurl4-openssl-dev
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac
./scripts/build.sh make
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
cd lpac
./scripts/build.sh make
```

</details>

<details>
<summary>Windows(x86_64)</summary>

Windows need prebuilt libcurl.dll, you can replace the download link to newest curl version.

- Build on Linux(MINGW)

```bash
sudo apt install build-essential cmake git g++ libpcsclite-dev libcurl4-openssl-dev gcc-mingw-w64 g++-mingw-w64
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac
./scripts/build.sh mingw
```

- Build on Windows(MSYS2)

```bash
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac
./scripts/build.sh mingw
```

- Build on Windows(Cygwin)

With `gcc-core` `gcc-g++` `make` `cmake` `unzip` `wget` installed

```bash
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac
./scripts/build.sh mingw
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
cd lpac
./scripts/build.sh woa-zig
```

- Cross compile on Linux x86_64 host(GNU toolchain)

```bash
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac
./scripts/build.sh woa-mingw
```

- Build on Windows(MSYS2)

It is possible to build on **WoA devices** with [MSYS2 ARM64 Support](https://www.msys2.org/wiki/arm64/)

You may need to install `mingw-w64-clang-aarch64-cmake`, `mingw-w64-clang-aarch64-clang` and modify `cmake/linux-mingw64.cmake`(replace toolchain).

Download prebuilt curl dll is also needed. Refer to the previous compilation steps.

</details>

## Usage

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

```plain
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

```jsonc
{
   "type": "lpa",
   "payload": {
      "code": 0,
      "message": "success",
      "data": {/* .... */}
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

```plain
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
        "ts102241Version": "9.2.0",
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

\* Starting from SGP.22 v2.1, `javacardVersion` is renamed to `ts102241Version` \
\*\* SGP.22 has been a typo, `sasAcreditationNumber` should be `sasAccreditationNumber`

</details>

#### profile

Profile management, you can list, set alias (nickname), enable, disable, delete, download and discovery Profiles.

```plain
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
        "icon": "iVBO...",
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
        "icon": "iVBO...",
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

```plain
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
  <img src="https://contrib.rocks/image?repo=estkme-group/lpac" alt="" />
</a>

---

## License

- lpac (/src): AGPL-3.0
- libeuicc (/euicc): LGPL-v2

Copyright (c) 2023-2024 eSTKme Group
