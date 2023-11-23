#include "interface.private.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int lc(struct apdu_request *apdu, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t datalen)
{
    apdu->cla = cla;
    apdu->ins = ins;
    apdu->p1 = p1;
    apdu->p2 = p2;
    apdu->length = datalen;

    return datalen + sizeof(struct apdu_request);
}

static int le(struct apdu_request *apdu, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t requestlen)
{
    apdu->cla = cla;
    apdu->ins = ins;
    apdu->p1 = p1;
    apdu->p2 = p2;
    apdu->length = requestlen;

    return sizeof(struct apdu_request);
}

int euicc_apdu_lc(struct euicc_ctx *ctx, struct apdu_request **apdu, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t datalen)
{
    *apdu = (struct apdu_request *)ctx->g_apdu_request_buf;
    return lc(*apdu, cla, ins, p1, p2, datalen);
}

int euicc_apdu_le(struct euicc_ctx *ctx, struct apdu_request **apdu, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t requestlen)
{
    *apdu = (struct apdu_request *)ctx->g_apdu_request_buf;
    return le(*apdu, cla, ins, p1, p2, requestlen);
}

int euicc_apdu_transmit(struct euicc_ctx *ctx, struct apdu_response *response, const struct apdu_request *request, uint32_t request_len)
{
    struct euicc_apdu_interface *in = ctx->interface.apdu;

    memset(response, 0x00, sizeof(*response));

    if (in->transmit(ctx, &response->data, &response->length, (uint8_t *)request, request_len) < 0)
        return -1;

    if (response->length < 2)
        return -1;

    response->sw1 = response->data[response->length - 2];
    response->sw2 = response->data[response->length - 1];
    response->length -= 2;

    return 0;
}

void euicc_apdu_response_free(struct apdu_response *resp)
{
    free(resp->data);
    resp->data = NULL;
    resp->length = 0;
}

void euicc_apdu_request_print(struct apdu_request *req)
{
    fprintf(stderr, "CLA: %02X, INS: %02X, P1: %02X, P2: %02X, Lc: %02X, Data: ", req->cla, req->ins, req->p1, req->p2, req->length);
    for (int i = 0; i < req->length; i++)
        fprintf(stderr, "%02X ", req->data[i]);
    fprintf(stderr, "\n");
}

void euicc_apdu_response_print(struct apdu_response *resp)
{
    fprintf(stderr, "SW1: %02X, SW2: %02X, Data: ", resp->sw1, resp->sw2);
    for (int i = 0; i < resp->length; i++)
        fprintf(stderr, "%02X ", resp->data[i]);
    fprintf(stderr, "\n");
}
