# QMI APDU Backend

QMI is a proprietary protocol used by Qualcomm devices.
It is commonly used for communication with cellular modems.

## OpenWrt Specific

On OpenWrt, the `uqmi` command line tool can be used to interact with QMI devices.

To use `uqmi` as the APDU backend, set the `LPAC_APDU` environment variable to `uqmi`.

## Environment Variables

- `LPAC_APDU_QMI_UIM_SLOT`: specify which UIM slot will be used by QMI APDU backend. \
  ([integer](types.md#integer-type), default: `1`, slot number starts from `1`)
- `LPAC_APDU_QMI_DEVICE`: specify which QMI device will be used by QMI APDU backend. \
  ([string](types.md#string-type), default: none)
