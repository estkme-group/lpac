#include "stdio_helpers.h"

#include "euicc/base64.h"
#include "euicc/hexutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON_ex.h>
#include <lpac/utils.h>

static int hexify_encode_len(const int input) { return (input * 2) + 1; }

static int hexify_encode(char *encoded, const unsigned char *tx, const int tx_len) {
    return euicc_hexutil_bin2hex(encoded, hexify_encode_len(tx_len), tx, tx_len);
}

static int hexify_decode_len(const char *encoded) { return (int)(strlen(encoded) / 2); }

static int hexify_decode(uint8_t *rx, const char *encoded) {
    const size_t n = strlen(encoded);
    return euicc_hexutil_hex2bin_r(rx, n / 2, encoded, n);
}

// getline is a GNU extension, Mingw32 macOS and FreeBSD don't have (a working) one
int afgets(char **obuf, FILE *fp) {
    uint32_t len = 0;
    char buffer[2];
    char *obuf_new = NULL;

    *obuf = malloc(1);
    if (*obuf == NULL)
        goto err;
    (*obuf)[0] = '\0';

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        const uint32_t fgets_len = strlen(buffer);

        len += fgets_len + 1;
        obuf_new = realloc(*obuf, len);
        if (obuf_new == NULL)
            goto err;
        *obuf = obuf_new;
        strcat(*obuf, buffer);

        if (buffer[fgets_len - 1] == '\n')
            break;
    }

    (*obuf)[strcspn(*obuf, "\n")] = 0;

    return 0;

err:
    free(*obuf);
    *obuf = NULL;
    return -1;
}

static struct stdio_userdata stdio_hexify = {
    .encode = hexify_encode,
    .encode_len = hexify_encode_len,
    .decode = hexify_decode,
    .decode_len = hexify_decode_len,
};

static struct stdio_userdata stdio_base64 = {
    .encode = euicc_base64_encode,
    .encode_len = euicc_base64_encode_len,
    .decode = euicc_base64_decode,
    .decode_len = euicc_base64_decode_len,
};

int stdio_setup_userdata(struct stdio_userdata **userdata, const char *type) {
    if (userdata == NULL)
        return -1;
    if (strcasecmp(type, "hexify") == 0) {
        *userdata = &stdio_hexify;
    } else if (strcasecmp(type, "base64") == 0) {
        *userdata = &stdio_base64;
    } else {
        fprintf(stderr, "Unknown Type: %s\n", type);
        return -1;
    }
    return 0;
}
