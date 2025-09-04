#include "logger.h"

inline void euicc_apdu_request_print(FILE *fp, const struct apdu_request *req, const uint32_t request_len) {
    if (fp == NULL)
        return;
    fprintf(fp, "[DEBUG] [APDU] [TX] CLA: %02X, INS: %02X, P1: %02X, P2: %02X, Lc: %02X, Data: ", req->cla, req->ins,
            req->p1, req->p2, req->length);
    for (uint32_t i = 0; i < (request_len - sizeof(struct apdu_request)); i++)
        fprintf(fp, "%02X ", (req->data[i] & 0xFF));
    fprintf(fp, "\n");
}

inline void euicc_apdu_response_print(FILE *fp, const struct apdu_response *resp) {
    if (fp == NULL)
        return;
    fprintf(fp, "[DEBUG] [APDU] [RX] SW1: %02X, SW2: %02X, Data: ", resp->sw1, resp->sw2);
    for (uint32_t i = 0; i < resp->length; i++)
        fprintf(fp, "%02X ", (resp->data[i] & 0xFF));
    fprintf(fp, "\n");
}

inline void euicc_http_request_print(FILE *fp, const char *url, const char *tx) {
    if (fp == NULL)
        return;
    fprintf(fp, "[DEBUG] [HTTP] [TX] url: %s, data: %s\n", url, tx);
}

inline void euicc_http_response_print(FILE *fp, const uint32_t rcode, const char *rx) {
    if (fp == NULL)
        return;
    fprintf(fp, "[DEBUG] [HTTP] [RX] rcode: %d, data: %s\n", rcode, rx);
}

inline void euicc_unhandled_tag(FILE *fp, const struct euicc_derutil_node *node) {
    if (fp == NULL)
        return;
    fprintf(fp, "\n[PLEASE REPORT][TODO][TAG %02X]: ", node->tag);
    for (uint32_t i = 0; i < node->self.length; i++)
        fprintf(fp, "%02X ", node->self.ptr[i]);
    fprintf(fp, "\n");
}
