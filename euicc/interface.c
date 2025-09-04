#include "interface.private.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>

static int lc(struct apdu_request *apdu, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t datalen) {
    apdu->cla = cla;
    apdu->ins = ins;
    apdu->p1 = p1;
    apdu->p2 = p2;
    apdu->length = datalen;

    return datalen + sizeof(struct apdu_request);
}

static int le(struct apdu_request *apdu, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t requestlen) {
    apdu->cla = cla;
    apdu->ins = ins;
    apdu->p1 = p1;
    apdu->p2 = p2;
    apdu->length = requestlen;

    return sizeof(struct apdu_request);
}

int euicc_apdu_lc(struct euicc_ctx *ctx, struct apdu_request **apdu, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2,
                  uint8_t datalen) {
    *apdu = (struct apdu_request *)&ctx->apdu._internal.request_buffer;
    return lc(*apdu, cla, ins, p1, p2, datalen);
}

int euicc_apdu_le(struct euicc_ctx *ctx, struct apdu_request **apdu, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2,
                  uint8_t requestlen) {
    *apdu = (struct apdu_request *)&ctx->apdu._internal.request_buffer;
    return le(*apdu, cla, ins, p1, p2, requestlen);
}

int euicc_apdu_transmit(struct euicc_ctx *ctx, struct apdu_response *response, const struct apdu_request *request,
                        uint32_t request_len) {
    const struct euicc_apdu_interface *in = ctx->apdu.interface;

    memset(response, 0x00, sizeof(*response));

    euicc_apdu_request_print(ctx->apdu.log_fp, request, request_len);

    if (in->transmit(ctx, &response->data, &response->length, (uint8_t *)request, request_len) < 0)
        return -1;

    if (response->length < 2)
        return -1;

    response->sw1 = response->data[response->length - 2];
    response->sw2 = response->data[response->length - 1];
    response->length -= 2;

    euicc_apdu_response_print(ctx->apdu.log_fp, response);

    return 0;
}

void euicc_apdu_response_free(struct apdu_response *resp) {
    free(resp->data);
    resp->data = NULL;
    resp->length = 0;
}
