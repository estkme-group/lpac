#pragma once

#include "euicc.h"
#include "interface.private.h"

int es10x_command_iter(struct euicc_ctx *ctx, const uint8_t *der_req, unsigned req_len, int (*callback)(struct apdu_response *response, void *userdata), void *userdata);
int es10x_command(struct euicc_ctx *ctx, uint8_t **resp, unsigned *resp_len, const uint8_t *der_req, unsigned req_len);
