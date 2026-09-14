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
