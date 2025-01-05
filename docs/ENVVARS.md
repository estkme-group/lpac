# Environment Variables

## General

* `LPAC_CUSTOM_ISD_R_AID`: specify which AID will be used to open the logic channel. (hex string, 32 chars)
* `LPAC_APDU`: specify which APDU backend will be used. Values:
  - `at`: use AT commands interface used by LTE module
  - `pcsc`: use PC/SC Smart Card API
  - `stdio`: use standard input/output
  - `qmi`: use QMI
  - `qmi_qrtr`: use QMI over QRTR
  - `mbim`: use MBIM
  - GBinder-based backends for `libhybris` (Halium) distributions:
	- `gbinder_hidl`: use HIDL IRadio (SoC launched before Android 13)
* `LPAC_HTTP`: specify which HTTP backend will be used.
  - `curl`: use libcurl
  - `stdio`: use standard input/ouput
* `AT_DEVICE`: specify which serial port device will be used by AT APDU backend.
* `QMI_DEVICE`: specify which QMI device will be used by QMI APDU backend.
* `UIM_SLOT`: specify which UIM slot will be used by QMI and MBIM APDU backends. (default: 1, slot number starts from 1)
* `DRIVER_IFID`: specify which PC/SC interface will be used by PC/SC APDU backend.
* `MBIM_DEVICE`: specify which MBIM device will be used by MBIM APDU backend. (default: "/dev/cdc-wdm0")
* `MBIM_USE_PROXY`: tell the MBIM APDU backend to use the mbim-proxy. (default: 0, anything other than 0 means true)

## Debug

* `LIBEUICC_DEBUG_APDU`: enable debug output for APDU.
* `LIBEUICC_DEBUG_HTTP`: enable debug output for HTTP.
* `AT_DEBUG`: enable debug output for AT APDU backend.
* `GBINDER_APDU_DEBUG`: enable debug output for GBinder APDU backend. MUST be `true` to take effect.
