# Device-specific Caveats

The device-specific caveats section outlines any known issues, limitations, or special considerations that apply to particular devices when using this software.

## OpenWrt

Discussion thread: https://github.com/openwrt/packages/issues/24963

Problem: OpenWrt 23.05 or older may not support TLS 1.3 due to the SSL library linked by libcurl.

Solution: Upgrade the SSL library linked by libcurl to a version that supports TLS 1.3, such as mbedTLS 3.6.0 or newer. Alternatively, upgrade your OpenWrt system to a version that includes a compatible SSL library.

## Fibocom FM350-GL

Discussion thread: https://github.com/estkme-group/lpac/pull/449

Problem: When opening a logical channel with `AT+CCHO`, the Fibocom FM350-GL replies with a bare decimal channel number (e.g. `1`) instead of the standard `+CCHO: <id>` response defined in 3GPP TS 27.007, so the AT driver cannot find the channel ID and opening the logical channel fails.

Solution: The AT driver accepts a bare decimal line as the channel ID in addition to the standard `+CCHO: <id>` response, so no action is needed. Confirmed on an FM350-GL in USB mode.
