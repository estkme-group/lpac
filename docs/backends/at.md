# AT APDU Backend

> [!CAUTION]
>
> **FOR DEMO PURPOSES ONLY.**
> 
> Only requests that strictly follow the [ETSI TS 127 007] specification are supported. \
> Requests outside the specification will be **REJECTED**.
>
> Some operations (e.g: download, delete, etc.), may fail due to insufficient response time. \
> The Maximum Response Time is typically 300ms, which is insufficient for many eUICC operations.

The AT Backend is used to communicate with the eUICC via AT commands over a serial port. \
It is commonly used with cellular modems that support AT commands for eUICC management.

Not all cellular modems support the required AT commands for eUICC management. \
Please refer to your modem's documentation to verify compatibility.

[ETSI TS 127 007]: https://www.etsi.org/deliver/etsi_ts/127000_127099/127007/15.02.00_60/ts_127007v150200p.pdf

## Unix-like platform

If you need to allow **non-root** users to access serial ports, read the following article

1. <https://help.ubuntu.com/community/DialupModemHowto/SetUpDialer#Configuring_the_dialup_connection_to_your_provider>
   (Ubuntu)
2. <https://manpages.debian.org/unstable/minicom/minicom.1.en.html#DEBIAN_SPECIFIC>
   (Debian)
3. <https://wiki.archlinux.org/title/Working_with_the_serial_console#Connect_using_a_terminal_emulator_program>
   (Arch Linux)

## Caveats

- If you encounter the `+CME ERROR: 3` (Operation not allowed) issue, this may be related to device internal settings. \
  Please contact this modem FAE for instructions on how to resolve it.

## `at` Backend

Use the [AT+{CCHO,CCHC,CGLA}][managed] command for APDU transmission.

[managed]: https://www.etsi.org/deliver/etsi_ts/127000_127099/127007/15.02.00_60/ts_127007v150200p.pdf#page=147

## `at_csim` Backend

Use the [AT+CSIM][unmanaged] command for APDU transmission.

[unmanaged]: https://www.etsi.org/deliver/etsi_ts/127000_127099/127007/15.02.00_60/ts_127007v150200p.pdf#page=129

## Environment Variables

- `LPAC_APDU_AT_DEVICE`: specify which serial port device will be used by AT APDU backend. \
  ([string](types.md#string-type), default: `/dev/ttyUSB0` on Unix-like platform and `COM3` on Windows)
- `LPAC_APDU_AT_DEBUG`: enable debug output for AT APDU backend. \
  ([boolean](types.md#boolean-type), default: `false`)
