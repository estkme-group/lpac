# AT APDU Backend

The AT Backend is used to communicate with the eUICC via AT commands over a serial interface.
It is commonly used with cellular modems that support AT commands for SIM card management.

## Unix-like platform

If your needs rootless access to serial port

1. <https://askubuntu.com/a/133244> (Debian/Ubuntu)
2. <https://bbs.archlinux.org/viewtopic.php?id=178552> (Arch Linux)

## Environment Variables

- `LPAC_APDU_AT_DEVICE`: specify which serial port device will be used by AT APDU backend. \
  ([string](types.md#string-type), default: `/dev/ttyUSB0` on Unix-like platform and `COM3` on Windows)
- `LPAC_APDU_AT_DEBUG`: enable debug output for AT APDU backend. \
  ([boolean](types.md#boolean-type), default: `false`)
