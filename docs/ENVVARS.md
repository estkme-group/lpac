# Environment Variables

## General

* `LPAC_CUSTOM_ES10X_MSS`: specify maximum segment size for ES10x APDU backend. (default: 120, min: 6, max: 255)
* `LPAC_CUSTOM_ISD_R_AID`: specify which AID will be used to open the logic channel. (hex string, 32 chars)
* `LPAC_APDU`: specify which APDU backend will be used. Values:
  - `at`: Use the AT command interface with commands like `AT+CCHO`, `AT+CCHC`, `AT+CGLA`.
    - On Unix-like platforms (Linux, BSD), use serial devices such as `/dev/ttyUSB0`.
    - On Windows platforms, use serial COM ports such as `COM3`.
  - `at_csim`: Same as above, but using `AT+CSIM` for APDU communication.
  - `pcsc`: use PC/SC Smart Card API
  - `stdio`: use standard input/output
  - `qmi`: use QMI
  - `qmi_qrtr`: use QMI over QRTR
  - `mbim`: use MBIM
  - GBinder-based backends for `libhybris` (Halium) distributions:
    - `gbinder_hidl`: use HIDL IRadio (SoC launched before Android 13)
* `LPAC_HTTP`: specify which HTTP backend will be used.
  - `curl`: use libcurl
  - `stdio`: use standard input/output
* `LPAC_APDU_AT_DEVICE`: specify which serial port device will be used by AT APDU backend.
* `LPAC_APDU_PCSC_DRV_IFID`: specify which PC/SC interface index will be used by PC/SC APDU backend.
* `LPAC_APDU_PCSC_DRV_NAME`: specify which PC/SC interface name will be used by PC/SC APDU backend.
* `LPAC_APDU_PCSC_DRV_IGNORE_NAME`: specify which PC/SC interface names will be ignored by PC/SC APDU backend. (use semicolon (`;`) split, for example: `Yubico;Canokeys`).
* `LPAC_APDU_QMI_UIM_SLOT`: specify which UIM slot will be used by QMI APDU backend. (default: 1, slot number starts from 1)
* `LPAC_APDU_QMI_DEVICE`: specify which QMI device will be used by QMI APDU backend.
* `LPAC_APDU_MBIM_UIM_SLOT`: specify which UIM slot will be used by MBIM APDU backend. (default: 1, slot number starts from 1)
* `LPAC_APDU_MBIM_USE_PROXY`: tell the MBIM APDU backend to use the mbim-proxy. (boolean)
* `LPAC_APDU_MBIM_DEVICE`: specify which MBIM device will be used by MBIM APDU backend. (default: `/dev/cdc-wdm0`)

## Debug

* `LIBEUICC_DEBUG_APDU`: enable debug output for APDU.
* `LIBEUICC_DEBUG_HTTP`: enable debug output for HTTP.
* `LPAC_APDU_AT_DEBUG`: enable debug output for AT APDU backend. (boolean)
* `LPAC_APDU_GBINDER_DEBUG`: enable debug output for GBinder APDU backend. (boolean)
