# MBIM APDU Backend

MBIM is a protocol used to communicate with mobile broadband devices, such as cellular modems.
It is commonly used in Linux-based systems to manage mobile broadband connections.

## Environment Variables

- `LPAC_APDU_MBIM_UIM_SLOT`: specify which UIM slot will be used by MBIM APDU backend. \
  ([integer](types.md#integer-type), default: `1`, slot number starts from `1`)
- `LPAC_APDU_MBIM_USE_PROXY`: tell the MBIM APDU backend to use the mbim-proxy. \
  ([boolean](types.md#boolean-type), default: `false`)
- `LPAC_APDU_MBIM_DEVICE`: specify which MBIM device will be used by MBIM APDU backend. \
  ([string](types.md#string-type), default: `/dev/cdc-wdm0`)
