# Environment Variables

## General

- `LPAC_CUSTOM_ES10X_MSS`: specify maximum segment size for ES10x APDU backend. \
  ([integer](backends/types.md#integer-type), default: 120, min: 6, max: 255)
- `LPAC_CUSTOM_ISD_R_AID`: specify which AID will be used to open the logic channel. \
  ([string](backends/types.md#string-type), hexadecimal string, default: `A0000005591010FFFFFFFF8900000100`)
- `LPAC_APDU_DEBUG`: enable debug output for APDU. \
  ([boolean](backends/types.md#boolean-type), default: `false`)
- `LPAC_HTTP_DEBUG`: enable debug output for HTTP. \
  ([boolean](backends/types.md#boolean-type), default: `false`)

## Backends

> [!NOTE]
>
> By default, lpac only tries to use `pcsc` and `stdio` in order.
>
> If you want to use another APDU backend, specify with `LPAC_APDU` explicit instead.

* `LPAC_APDU`: specify which APDU backend will be used. Values:
  - [`at`](backends/at.md): AT command interface (using AT+CCHO/CCHC/CGLA)  
  - [`at_csim`](backends/at.md): AT command interface (using AT+CSIM)  
  - [`pcsc`](backends/pcsc.md): PC/SC Smartcard
  - [`stdio`](backends/stdio.md): Standard input/output
  - [`qmi`](backends/qmi.md): Qualcomm MSM Interface
  - [`qmi_qrtr`](backends/qmi.md): QMI over QRTR
  - [`uqmi`](backends/qmi.md#openwrt-specific): uQMI command line tool
  - [`mbim`](backends/mbim.md): Mobile Broadband Interface Model
  - [`gbinder_hidl`](backends/gbinder.md): GBinder HIDL
* `LPAC_HTTP`: specify which HTTP backend will be used. Values:
  - [`curl`](backends/curl.md): cURL access to HTTPS resources
  - [`winhttp`](backends/winhttp.md): WinHTTP access to HTTPS resources
  - [`stdio`](backends/stdio.md): Standard input/output
