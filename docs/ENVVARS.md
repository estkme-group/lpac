# Environment Variables

## General

* `LPAC_APDU`: specify which APDU backend will be used. Values:
  - `at`: use AT commands interface used by LTE module (Linux only)
  - `pcsc`: use PC/SC Smart Card API
  - `stdio`: use standard input/output
  - `qmi_qrtr`: use QMI over QRTR
  - `gbinder_hidl`: use HIDL IRadio (GBinder-based backends for `libhybris` (Halium) distributions, SoC launched before Android 13.)
* `LPAC_HTTP`: specify which HTTP backend will be used.
  - `curl`: use libcurl
  - `stdio`: use standard input/ouput

* `LPAC_DRIVER_AT_DEVICE`: specify which serial port device will be used by AT APDU backend.
* `LPAC_DRIVER_UIM_SLOT`: specify which UIM slot will be used by QMI QRTR APDU backend. (default: 1)
* `LPAC_DRIVER_PCSC_READER`: specify which PC/SC interface will be used by PC/SC APDU backend.

## Debug

* `LIBEUICC_DEBUG_APDU`: enable debug output for APDU.
* `LIBEUICC_DEBUG_HTTP`: enable debug output for HTTP.
* `LPAC_DRIVER_AT_DEBUG`: enable debug output for AT APDU backend.
* `LPAC_DRIVER_GBINDER_DEBUG`: enable debug output for GBinder APDU backend. MUST be `true` to take effect.
