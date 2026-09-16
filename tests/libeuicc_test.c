#include <criterion/criterion.h>

#include <euicc/base64.h>
#include <euicc/euicc.private.h>
#include <euicc/hexutil.h>

#include <stdint.h>
#include <string.h>

Test(libeuicc, hex_decode_rejects_invalid_input) {
    uint8_t output[2] = {0};

    cr_assert_lt(euicc_hexutil_hex2bin(output, sizeof(output), "12xz"), 0);
    cr_assert_lt(euicc_hexutil_hex2bin(output, sizeof(output), "123"), 0);
}

/* Regression: even-length (18-digit) ICCIDs must be 0xFF-padded to the full
 * 10-byte SGP.22 Iccid (OCTET STRING (SIZE(10))) and the padded length must
 * be returned, so ES10c Enable/Disable/DeleteProfile echo the ICCID exactly
 * as GetProfilesInfo reports it. Before the fix, the 0xFF padding was
 * written but not counted, producing a 9-byte 5A TLV that eUICCs reject
 * with iccidOrAidNotFound. */
Test(libeuicc, gsmbcd_even_digit_iccid_padded_to_ten_bytes) {
    uint8_t id[16] = {0};

    const uint8_t expected[] = {0x98, 0x94, 0x10, 0x32, 0x54, 0x76, 0x98, 0x10, 0x14, 0xFF};

    cr_assert_eq(euicc_hexutil_gsmbcd2bin(id, sizeof(id), "894901234567890141", 10), 10);
    cr_assert_arr_eq(id, expected, sizeof(expected));
}

/* Odd-length (19-digit) ICCIDs carry their 'F' pad nibble inside the
 * converted bytes and were already correct; pin the behavior. */
Test(libeuicc, gsmbcd_odd_digit_iccid_uses_f_padding) {
    uint8_t id[16] = {0};

    const uint8_t expected[] = {0x98, 0x94, 0x10, 0x32, 0x54, 0x76, 0x98, 0x10, 0x14, 0xF5};

    cr_assert_eq(euicc_hexutil_gsmbcd2bin(id, sizeof(id), "8949012345678901415", 10), 10);
    cr_assert_arr_eq(id, expected, sizeof(expected));
}

/* The IMEI caller passes padding_to = 0: no padding bytes may be written
 * and the exact digit-pair count must be returned (buffer-overflow guard
 * from #415 must stay effective). */
Test(libeuicc, gsmbcd_padding_to_zero_writes_no_padding) {
    uint8_t imei[16];

    memset(imei, 0xA5, sizeof(imei));
    const uint8_t expected[] = {0x53, 0x94, 0x10, 0x32, 0x54, 0x76, 0x98, 0xF5};

    cr_assert_eq(euicc_hexutil_gsmbcd2bin(imei, sizeof(imei), "354901234567895", 0), 8);
    cr_assert_arr_eq(imei, expected, sizeof(expected));
    cr_assert_eq(imei[8], 0xA5);
    cr_assert_eq(imei[9], 0xA5);
}
