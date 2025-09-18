# AT APDU Backend

The AT Backend is used to communicate with the eUICC via AT commands over a serial interface.
It is commonly used with cellular modems that support AT commands for SIM card management.

## Unix-like platform

If you need to allow **non-root** users to access serial ports, read the following article

1. <https://help.ubuntu.com/community/DialupModemHowto/SetUpDialer#Configuring_the_dialup_connection_to_your_provider>
   (Ubuntu)
2. <https://manpages.debian.org/unstable/minicom/minicom.1.en.html#DEBIAN_SPECIFIC>
   (Debian)
3. <https://wiki.archlinux.org/title/Working_with_the_serial_console#Connect_using_a_terminal_emulator_program>
   (Arch Linux)

## Environment Variables

- `LPAC_APDU_AT_DEVICE`: specify which serial port device will be used by AT APDU backend. \
  ([string](types.md#string-type), default: `/dev/ttyUSB0` on Unix-like platform and `COM3` on Windows)
- `LPAC_APDU_AT_DEBUG`: enable debug output for AT APDU backend. \
  ([boolean](types.md#boolean-type), default: `false`)
