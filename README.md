# lpac


lpac is a cross platform local profile agent program.

Features:
- Support Activate Code and Confirm Code
- Support custom IMEI sent to server
- Support eSIM Discovery (Push Model)
- Profile management: list, enable, disable, delete and nickname
- Notification management: list, send and delete
- Lookup eUICC chip info
- etc

Download: 
[Github Release](https://github.com/estkme-group/lpac/releases/latest)

lpac is dynamic-linked, Release is built by Github action, if you can't run it you need to compile by yourself

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
<summary>Windows</summary>

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
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-gcc
git clone --depth=1 https://github.com/estkme-group/lpac
cd lpac && mkdir build && cd build
cmake -DLINUX_MINGW32=ON .. && ninja
# Download libcurl
wget https://curl.se/windows/dl-8.4.0_6/curl-8.4.0_6-win64-mingw.zip -O curl.zip && unzip curl.zip && mv curl-8.4.0_6-win64-mingw/bin/libcurl-x64.dll output/libcurl.dll
```
</details>

# Usage

In Linux, you need install `pcscd`, `pcsclite` and `libcurl`.

APDU and HTTP interfaces of lpac has serval backends, you need specify `$APDU_INTERFACE` and `$HTTP_INTERFACE` environment variable to interface library path. If not specified, it will use `libapduinterface_pcsc` and `libhttpinterface_curl`.

APDU Backends:
- `libapduinterface_at`: use AT commands interface used by LTE module
- `libapduinterface_pcsc`: use PC/SC Smart Card API
- `libapduinterface_stdio`: use standard input/output

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
    driver        View backend info
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
      "eid": " EID  ",
      "default_smds": "testrootsmds.gsma.com",
      "default_smdp": "",
      "euicc_info2": {
        "profile_version": "2.1.0",
        "sgp22_version": "2.2.0",
        "euicc_firmware_version": "4.6.0",
        "uicc_firmware_version": "9.2.0",
        "global_platform_version": "2.3.0",
        "protection_profile_version": "0.0.1",
        "sas_accreditation_number": "GI-BA-UP-0419",
        "free_nvram": 295424,
        "free_ram": 295424
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
    download  Pull method to download new Profile
    discovery Push method to download Profiles
```

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

##### Discovery requires connecting to the SM-DS server to do the profile push query

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
            "iccid": "8999990000... ",
            "isdpAid": "A0000005591010...",
            "profileState": 1,
            "profileNickname": "tel-u",
            "serviceProviderName": "eSIM",
            "profileName": "NEWARE_CUG_V001",
            "profileClass": 2
         }, 
         {
            "iccid": "894447860000...", 
            "isdpAid": "A0000005591011...", 
            "profileState": 0,
            "serviceProviderName": "BetterRoaming", 
            "profileName": "BetterRoaming", 
            "profileClass": 2
         }
      ]
   }
}
```

- `iccid`: ICCID of Profile
- `isdpAid`: Aid of Profile
- `profileState`: State of Profile, 1 is enabled. You can enable multiple Profile only when device support MEP (Multiple Enabled Profiles)
- `profileNickname`: Nickname of Profile
- `serviceProviderName`: Telecom operators of Profile
- `profileName`: Name of Profile
- `profileClass`: Version of Profile

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
            "seqNumber": 7,
            "profileManagementOperation": 32,
            "notificationAddress": "rsp-eu.simlessly.com",
            "iccid": "8999990000"
         },
         {
            "seqNumber": 8,
            "profileManagementOperation": 64,
            "notificationAddress": "rsp.truphone.com",
            "iccid": "894447860000"
         }
      ]
   }
}
```

- `seqNumber`: Sequence ID
- `profileManagementOperation`: Profile status identifier
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
AGPL-3.0
Copyright (c) 2023-2024 eSTKme Group
