# Device-specific Caveats

The device-specific caveats section outlines any known issues, limitations, or special considerations that apply to particular devices when using this software.

## OpenWrt

Discussion thread: https://github.com/openwrt/packages/issues/24963

Problem: OpenWrt 23.05 or older may not support TLS 1.3 due to the SSL library linked by libcurl.

Solution: Upgrade the SSL library linked by libcurl to a version that supports TLS 1.3, such as mbedTLS 3.6.0 or newer. Alternatively, upgrade your OpenWrt system to a version that includes a compatible SSL library.

## Modems That Reject Lowercase AT+CSIM/AT+CGLA Payloads

Discussion thread: https://github.com/estkme-group/lpac/issues/428

Problem: Some modems parse the hexadecimal payloads of `AT+CSIM`/`AT+CGLA` (and
the AID parameter of `AT+CCHO`) case-sensitively and answer a bare `ERROR` (no
`+CME ERROR` code) to lowercase hex digits. With the `at` or `at_csim` backend
this surfaces as `{"type":"lpa","payload":{"code":-1,"message":"euicc_init","data":""}}`.

ETSI TS 127 007 defines these payloads as "hexadecimal character format; refer
+CSCS", and the `HEX` TE character set is defined — and every example given,
e.g. "032FE6" — with uppercase digits only. Strict parsers are therefore
conformant; the lowercase form is only accepted by tolerant implementations.

Affected modems reported so far (all Qualcomm-based):

| Module | Report |
| --- | --- |
| Sierra Wireless EM7565 | <https://gerrit.osmocom.org/c/pySim/+/24023>: `AT+CSIM=14,"00a40004023F00"` → `ERROR`, uppercased → `+CSIM: 4,"612F"` (fix merged in pySim since 2021) |
| Foxconn T99W696 | <https://github.com/estkme-group/lpac/issues/428> |
| Quectel RG502Q-EA | <https://github.com/estkme-group/lpac/pull/466>: same failure signature, wire-captured for both cases; with uppercase payloads a complete SGP.22 profile download ran over `AT+CSIM` in ~24 s |

Solution: set `LPAC_APDU_AT_HEX=UPPER` when using the `at` or `at_csim`
backend on these modems.

No modem that requires the lowercase form has been documented so far, but the
default stays `LOWER` since neither case is mandated for the TE.
