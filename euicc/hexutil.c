#include "hexutil.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#pragma clang diagnostic ignored "-Wgcc-compat"

int euicc_hexutil_bin2hex(char *output, uint32_t output_len, const uint8_t *bin, uint32_t bin_len) {
    const char hexDigits[] = "0123456789abcdef";

    if (!bin || !output) {
        return -1;
    }

    if (output_len < 2 * bin_len + 1) {
        return -1;
    }

    for (uint32_t i = 0; i < bin_len; ++i) {
        char byte = bin[i];
        output[2 * i] = hexDigits[(byte >> 4) & 0x0F];
        output[2 * i + 1] = hexDigits[byte & 0x0F];
    }
    output[2 * bin_len] = '\0';

    return 0;
}

int euicc_hexutil_hex2bin(uint8_t *output, uint32_t output_len, const char *str) {
    return euicc_hexutil_hex2bin_r(output, output_len, str, strlen(str));
}

int euicc_hexutil_hex2bin_r(uint8_t *output, uint32_t output_len, const char *str, uint32_t str_len) {
    uint32_t length;

    if (!str || !output || str_len % 2 != 0) {
        return -1;
    }

    length = str_len / 2;
    if (length > output_len) {
        return -1;
    }

    for (uint32_t i = 0; i < length; ++i) {
        char high = str[2 * i];
        char low = str[2 * i + 1];

        if (high >= '0' && high <= '9') {
            high -= '0';
        } else if (high >= 'a' && high <= 'f') {
            high = high - 'a' + 10;
        } else if (high >= 'A' && high <= 'F') {
            high = high - 'A' + 10;
        } else {
            return -1;
        }

        if (low >= '0' && low <= '9') {
            low -= '0';
        } else if (low >= 'a' && low <= 'f') {
            low = low - 'a' + 10;
        } else if (low >= 'A' && low <= 'F') {
            low = low - 'A' + 10;
        } else {
            return -1;
        }

        output[i] = (high << 4) + low;
    }

    return length;
}

int euicc_hexutil_gsmbcd2bin(uint8_t *output, uint32_t output_len, const char *str, uint32_t padding_to) {
    uint32_t str_length;
    uint32_t idx = 0;

    str_length = strlen(str);

    if (output_len < (str_length + 1) / 2) {
        return -1;
    }

    if (output_len < padding_to) {
        return -1;
    }

    for (uint32_t i = 0; i < str_length; i += 2) {
        char high_nibble = (i + 1 < str_length) ? str[i + 1] : 'F';
        char low_nibble = str[i];

        uint8_t high_nibble_val = 0x0;
        uint8_t low_nibble_val = 0x0;

        if (low_nibble >= '0' && low_nibble <= '9') {
            low_nibble_val = low_nibble - '0';
        } else if (low_nibble == 'F' || low_nibble == 'f') {
            low_nibble_val = 0x0F;
        } else {
            return -1;
        }

        if (high_nibble >= '0' && high_nibble <= '9') {
            high_nibble_val = high_nibble - '0';
        } else if (high_nibble == 'F' || high_nibble == 'f') {
            high_nibble_val = 0xF;
        } else {
            return -1;
        }

        output[idx] = (high_nibble_val << 4) | low_nibble_val;
        idx++;
    }

    for (; idx < padding_to; idx++) {
        output[idx] = 0xFF;
    }

    return idx;
}

int euicc_hexutil_bin2gsmbcd(char *output, uint32_t output_len, const uint8_t *binData, uint32_t length) {
    if (euicc_hexutil_bin2hex(output, output_len, binData, length)) {
        return -1;
    }

    length = strlen(output);
    for (size_t i = 0; i < length - 1; i += 2) {
        char temp = output[i];
        output[i] = output[i + 1];
        output[i + 1] = temp;
    }

    for (uint32_t j = length - 1; j > 0; j--) {
        if (output[j] != 'f') {
            break;
        }
        output[j] = '\0';
    }

    return 0;
}
