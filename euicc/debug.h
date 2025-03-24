#pragma once
#include "stdlib.h"

void euicc_apdu_request_print(const struct apdu_request *r, uint32_t length);

void euicc_apdu_response_print(const struct apdu_response *r);

void euicc_http_request_print(char *full_url, const char *tx);

void euicc_http_response_print(uint32_t rcode, const char *rx);

void euicc_derutil_print_unhandled_tag(const struct euicc_derutil_node *node);
