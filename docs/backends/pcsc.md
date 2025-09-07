# PC/SC APDU Backend

PC/SC (Personal Computer/Smart Card) is a standard for smart card integration with computers.
It provides a framework for communication between smart cards and applications, allowing for secure authentication and
data exchange

## Environment Variables

- `LPAC_APDU_PCSC_DRV_IFID`: specify which PC/SC interface index will be used by PC/SC APDU backend. \
  ([integer](types.md#integer-type), default: none, interface index starts from `0`)
- `LPAC_APDU_PCSC_DRV_NAME`: specify which PC/SC interface name will be used by PC/SC APDU backend. \
  ([string](types.md#string-type), default: none, for example: `ESTKme-RED`)
- `LPAC_APDU_PCSC_DRV_IGNORE_NAME`: specify which PC/SC interface names will be ignored by PC/SC APDU backend. \
  ([string](types.md#string-type), use [semicolon] split, for example: `Yubico;Canokeys`).

[semicolon]: https://en.wikipedia.org/wiki/Semicolon
