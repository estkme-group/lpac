#include "hexutil.h"

#include <stdlib.h>
#include <string.h>

inline int euicc_hexutil_hex2bin(uint8_t *output, const uint32_t output_len, const char *input) {
    return euicc_hexutil_hex2bin_r(output, output_len, input, strlen(input));
}

int euicc_hexutil_hex2bin_r(uint8_t *output, const uint32_t output_len, const char *input, const uint32_t input_len) {
    if (output == NULL || input == NULL || (input_len % 2) != 0 || output_len < (input_len / 2)) {
        return -1;
    }
    uint8_t c, msb = 0;
    uint32_t bytes = 0;
    memset(output, 0, output_len);
    for (uint32_t i = 0; i < input_len; i++) {
        c = input[i] | ' '; // to lower
        if ((c < '0' || c > '9') && (c < 'a' || c > 'f')) {
            return -1; // invalid character
        }
        c -= c > '9' ? 'a' - 10 : '0';
        if (i % 2 == 0) {
            msb = c;
        } else {
            output[bytes++] = msb << 4 | c;
        }
    }
    return (int)bytes;
}

int euicc_hexutil_bin2hex(char *output, const uint32_t output_len, const uint8_t *bin, const uint32_t bin_len) {
    if (output == NULL || bin == NULL || output_len < ((2 * bin_len) + 1)) {
        return -1;
    }
    uint8_t msb, lsb;
    memset(output, 0, output_len);
    for (uint32_t i = 0, j = 0; i < bin_len; ++i) {
        msb = bin[i] >> 4;
        lsb = bin[i] & 0xf;
        output[j++] = (char)(msb + (msb > 9 ? 'a' - 10 : '0'));
        output[j++] = (char)(lsb + (lsb > 9 ? 'a' - 10 : '0'));
    }
    return 0;
}

int euicc_hexutil_gsmbcd2bin(uint8_t *output, const uint32_t output_len, const char *input, const uint32_t padding_to) {
    if (output == NULL || input == NULL || output_len < padding_to) {
        return -1;
    }
    uint32_t n = strlen(input);
    char tmp[n + (n % 2)];
    memcpy(tmp, input, n);
    memset(tmp + n, 'f', n % 2); // pad with 'f' if odd
    const int bytes = euicc_hexutil_hex2bin_r(output, output_len, tmp, n + (n % 2));
    if (bytes == -1) {
        return -1;
    }
    for (int i = 0; i < bytes; i++) {
        output[i] = output[i] >> 4 | (output[i] & 0xf) << 4; // swap
    }
    memset(output + bytes, 0xff, padding_to - bytes);
    return bytes;
}

int euicc_hexutil_bin2gsmbcd(char *output, const uint32_t output_len, const uint8_t *bin, const uint32_t bin_len) {
    if (output == NULL || bin == NULL || output_len < ((2 * bin_len) + 1)) {
        return -1;
    }
    uint32_t i;
    uint8_t tmp[bin_len];
    for (i = 0; i < bin_len; i++) {
        tmp[i] = bin[i] >> 4 | (bin[i] & 0xf) << 4; // swap
    }
    if (euicc_hexutil_bin2hex(output, output_len, tmp, bin_len) != 0) {
        return -1;
    }
    for (i = (2 * bin_len) - 1; i > 0 && output[i] != 'f'; i--)
        ;
    output[i] = 0;
    return 0;
}
