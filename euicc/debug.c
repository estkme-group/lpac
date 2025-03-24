#include "debug.h"
#include <stdlib.h>
#include <stdio.h>

#include "derutil.h"
#include "interface.private.h"

#define ENV_DEBUG_APDU "LIBEUICC_DEBUG_APDU"
#define ENV_DEBUG_HTTP "LIBEUICC_DEBUG_HTTP"

static void print_hex(const uint8_t *data, const uint32_t n) {
    for (uint32_t i = 0; i < n; i++)
        fprintf(stderr, "%02X", data[i] & 0xFF);
}

void euicc_apdu_request_print(const struct apdu_request *r, const uint32_t length) {
#ifdef LIBEUICC_DEBUG_APDU
    if (getenv(ENV_DEBUG_APDU) == NULL) return;
    fprintf(stderr, "[DEBUG] [APDU] [TX] CLA: %02X, INS: %02X, P1: %02X, P2: %02X, Lc: %02X, Data: ",
            r->cla, r->ins, r->p1, r->p2, r->length);
    print_hex(r->data, length);
    fprintf(stderr, "\n");
#endif
}

void euicc_apdu_response_print(const struct apdu_response *r) {
#ifdef LIBEUICC_DEBUG_APDU
    if (getenv(ENV_DEBUG_APDU) == NULL) return;
    fprintf(stderr, "[DEBUG] [APDU] [RX] SW1: %02X, SW2: %02X, Data: ", r->sw1, r->sw2);
    print_hex(r->data, r->length);
    fprintf(stderr, "\n");
#endif
}

void euicc_http_request_print(char *full_url, const char *tx) {
#ifdef LIBEUICC_DEBUG_HTTP
    if (getenv(ENV_DEBUG_HTTP) == NULL) return;
    fprintf(stderr, "[DEBUG] [HTTP] [TX] url: %s, data: %s\n", full_url, tx);
#endif
}

void euicc_http_response_print(const uint32_t rcode, const char *rx) {
#ifdef LIBEUICC_DEBUG_HTTP
    if (getenv(ENV_DEBUG_HTTP) == NULL) return;
    fprintf(stderr, "[DEBUG] [HTTP] [RX] rcode: %d, data: %s\n", rcode, rx);
#endif
}

void euicc_derutil_print_unhandled_tag(const struct euicc_derutil_node *node) {
#ifdef LIBEUICC_DEBUG_APDU
    if (getenv(ENV_DEBUG_APDU) == NULL) return;
    fprintf(stderr, "\n[PLEASE REPORT][TODO][TAG %02X]: ", node->tag);
    print_hex(node->self.ptr, node->self.length);
    fprintf(stderr, "\n");
#endif
}
