#pragma once

#include "derutil.h"
#include "interface.private.h"

#include <stdio.h>
#include <stdlib.h>

void euicc_apdu_request_print(FILE *fp, const struct apdu_request *req, uint32_t req_len);

void euicc_apdu_response_print(FILE *fp, const struct apdu_response *resp);

void euicc_apdu_unhandled_tag_print(FILE *fp, const struct euicc_derutil_node *node);

void euicc_http_request_print(FILE *fp, const char *url, const char *tx);

void euicc_http_response_print(FILE *fp, uint32_t rcode, const char *rx);
