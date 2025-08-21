#pragma once

#define ENV_STDIO_APDU_TYPE APDU_ENV_NAME(STDIO, TYPE)
#define ENV_STDIO_HTTP_TYPE HTTP_ENV_NAME(STDIO, TYPE)

#include <stdint.h>
#include <stdio.h>

struct stdio_userdata {
    int (*encode)(char *encoded, const unsigned char *tx, int tx_len);
    int (*encode_len)(int input);
    int (*decode)(uint8_t *rx, const char *encoded);
    int (*decode_len)(const char *encoded);
};

int afgets(char **obuf, FILE *fp);

int stdio_setup_userdata(struct stdio_userdata **userdata, const char *type);
