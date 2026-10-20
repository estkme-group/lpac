# Device-specific Caveats

The device-specific caveats section outlines any known issues, limitations, or special considerations that apply to particular devices when using this software.

## OpenWrt

Discussion thread: https://github.com/openwrt/packages/issues/24963

Problem: OpenWrt 23.05 or older may not support TLS 1.3 due to the SSL library linked by libcurl.

Solution: Upgrade the SSL library linked by libcurl to a version that supports TLS 1.3, such as mbedTLS 3.6.0 or newer. Alternatively, upgrade your OpenWrt system to a version that includes a compatible SSL library.
