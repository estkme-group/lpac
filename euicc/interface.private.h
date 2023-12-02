#pragma once
#include "euicc.h"
#include "interface.h"

#include <inttypes.h>

enum apdu_sw1
{
    SW1_OK = 0x90,
    SW1_LAST = 0x61,
};

struct apdu_request
{
    uint8_t cla;
    uint8_t ins;
    uint8_t p1;
    uint8_t p2;
    uint8_t length;
    uint8_t data[];
} __attribute__((packed));

struct apdu_response
{
    uint8_t *data;
    uint32_t length;
    uint8_t sw1;
    uint8_t sw2;
};

int euicc_apdu_lc(struct euicc_ctx *ctx, struct apdu_request **apdu, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t datalen);
int euicc_apdu_le(struct euicc_ctx *ctx, struct apdu_request **apdu, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t requestlen);
int euicc_apdu_transmit(struct euicc_ctx *ctx, struct apdu_response *response, const struct apdu_request *req, uint32_t req_len);
void euicc_apdu_response_free(struct apdu_response *resp);
