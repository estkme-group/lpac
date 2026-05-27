# MBIM APDU Backend

MBIM is a protocol used to communicate with mobile broadband devices, such as cellular modems.
It is commonly used in Linux-based systems to manage mobile broadband connections.

The backend uses the Microsoft UICC low-level access service for APDU transport.
Some MBIM devices do not allow changing the device slot mapping but still allow UICC low-level access on the modem's
currently selected slot. For those devices, slot mapping can be skipped with `LPAC_APDU_MBIM_SKIP_SLOT_MAPPING`.

## Environment Variables

- `LPAC_APDU_MBIM_UIM_SLOT`: specify which UIM slot will be used by MBIM APDU backend. \
  ([integer](types.md#integer-type), default: `1`, slot number starts from `1`)
- `LPAC_APDU_MBIM_USE_PROXY`: tell the MBIM APDU backend to use the mbim-proxy. \
  ([boolean](types.md#boolean-type), default: `false`)
- `LPAC_APDU_MBIM_DEVICE`: specify which MBIM device will be used by MBIM APDU backend. \
  ([string](types.md#string-type), default: `/dev/cdc-wdm0`)
- `LPAC_APDU_MBIM_SKIP_SLOT_MAPPING`: skip MBIM device slot mapping and use the modem's currently selected slot. \
  `LPAC_APDU_MBIM_UIM_SLOT` is ignored while this option is enabled. \
  ([boolean](types.md#boolean-type), default: `false`)
