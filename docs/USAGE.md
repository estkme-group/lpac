## Usage

In Linux, you need install `pcscd`, `pcsclite` and `libcurl`.

APDU and HTTP interfaces of lpac has serval backends, you need specify `$LPAC_APDU` and `$LPAC_HTTP` environment variable to interface library path. If not specified, it will use `pcsc` and `curl`. See also [environment variables](ENVVARS.md).

Using `at` APDU backend need access permission to serial port (normally `/dev/ttyUSBx`). On Arch Linux, you can add yourself to `uucp` group by `sudo usermod -aG uucp $USER`. On other distro, you may need add yourself into `dialout` group. If your serial port is not `/dev/ttyUSB0`, please use `$AT_DEVICE` to specify which one you want to use.

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

- `-s`: SM-DP+ server, optional, if not provided, it will try to read the default sm-dp+ attribute.
- `-m`: Matching ID, activation code. optional.
- `-c`: Confirmation Code, optional.
- `-i`: The IMEI of the device to which Profile is to be downloaded, optional.
- `-a`: LPA qrcode activation code string, e.g: `LPA:1$<sm-dp+ domain>$<matching id>`, if provided this option takes precedence over the `-s` and `-m` options, optional.

<details>

<summary>Example</summary>

```bash
./lpac profile download -s rsp.truphone.com -m "QR-G-5C-1LS-1W1Z9P7"

# LPA qrcode activation code string
./lpac profile download -a 'LPA:1$rsp.truphone.com$QR-G-5C-1LS-1W1Z9P7'
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
