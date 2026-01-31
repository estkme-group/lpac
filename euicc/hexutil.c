#include "hexutil.h"

#include <stdlib.h>
#include <string.h>

#ifndef LIBEUICC_REDUCED_STDLIB_CALL
#    include <ctype.h>
#    include <stddef.h>
#endif

#define SWAP(a, b)            \
    do {                      \
        typeof(a) temp = (a); \
        (a) = (b);            \
        (b) = temp;           \
    } while (0)

static inline void gsmbcd_swap_chars(char *restrict input, const uint32_t input_len) {
    for (uint32_t i = 0; i < input_len; i += 2) {
        SWAP(input[i], input[i + 1]);
    }
}

int euicc_hexutil_bin2hex(char *restrict output, const uint32_t output_len, const uint8_t *bin,
                          const uint32_t bin_len) {
    if (output == NULL || bin == NULL || output_len < ((2 * bin_len) + 1)) {
        return -1;
    }
    static const char digits[] = "0123456789abcdef";
    uint32_t n = 0;
    for (uint32_t i = 0; i < bin_len; i++) {
        output[n++] = digits[bin[i] >> 4];
        output[n++] = digits[bin[i] & 0xf];
    }
    output[n] = '\0';
    return (int)n;
}

inline int euicc_hexutil_hex2bin(uint8_t *restrict output, const uint32_t output_len, const char *restrict input) {
    return euicc_hexutil_hex2bin_r(output, output_len, input, strlen(input));
}

int euicc_hexutil_hex2bin_r(uint8_t *restrict output, const uint32_t output_len, const char *restrict input,
                            const uint32_t input_len) {
    if (output == NULL || input == NULL || (input_len % 2) != 0 || output_len < (input_len / 2)) {
        return -1;
    }
    uint32_t bytes = 0;
#ifdef LIBEUICC_REDUCED_STDLIB_CALL
    uint8_t c, msb = 0;
    for (uint32_t i = 0; i < input_len; i++) {
        c = input[i] | ' ';
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
#else
    char hex[3] = "\0\0";
    for (uint32_t i = 0; i < input_len; i += 2) {
        hex[0] = input[i + 0];
        hex[1] = input[i + 1];
        if (!(isxdigit(hex[0]) && isxdigit(hex[1]))) {
            return -1; // invalid character
        }
        output[bytes++] = (uint8_t)strtol(hex, NULL, 16);
    }
#endif
    return (int)bytes;
}

int euicc_hexutil_gsmbcd2bin(uint8_t *restrict output, const uint32_t output_len, const char *restrict input,
                             const uint32_t padding_to) {
    if (output == NULL || input == NULL || output_len < padding_to) {
        return -1;
    }
    const uint32_t n = strlen(input);
    char *bin = calloc(n + (n % 2), sizeof(char)); // +1 if odd
    if (bin == NULL) {
        return -1;
    }
    memcpy(bin, input, n);
    memset(bin + n, 'f', n % 2); // pad with 'f' if odd
    gsmbcd_swap_chars(bin, n + (n % 2));
    const int bytes = euicc_hexutil_hex2bin_r(output, output_len, bin, n + (n % 2));
    free(bin);
    if (bytes == -1) {
        return -1;
    }
    if (padding_to > (uint32_t)bytes) {
        memset(output + bytes, 0xff, padding_to - bytes);
    }
    return bytes;
}

int euicc_hexutil_bin2gsmbcd(char *restrict output, const uint32_t output_len, const uint8_t *restrict bin,
                             const uint32_t bin_len) {
    int n = euicc_hexutil_bin2hex(output, output_len, bin, bin_len);
    if (n < 0) {
        return -1;
    }
    n -= 1; // ignore NUL terminator
    gsmbcd_swap_chars(output, n);
    // trim trailing 'f'
    while (n > 0 && output[n] == 'f') {
        output[n--] = '\0';
    }
    return n;
}
