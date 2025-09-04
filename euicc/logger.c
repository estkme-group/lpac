#include "logger.h"

#include <string.h>
#include <unistd.h>

#define BASH_RED 31
#define BASH_GREEN 32
#define BASH_CLEAR 0

static void bash_colorize(FILE *fp, const int color) {
    if (isatty(fileno(fp)))
        fprintf(fp, "\033[%dm", color);
}

static void print_hex(FILE *fp, const uint8_t *data, const uint32_t length) {
    if (length == 0)
        return;
    fputs(", Data:", fp);
    for (uint32_t i = 0; i < length; i++)
        fprintf(fp, " %02X", data[i] & 0xFF);
}

inline void euicc_apdu_request_print(FILE *fp, const struct apdu_request *req, const uint32_t req_len) {
    if (fp == NULL)
        return;
    bash_colorize(fp, BASH_GREEN);
    fprintf(fp, "[DEBUG] [APDU] [TX] CLA: %02X, INS: %02X, P1: %02X, P2: %02X, Lc: %02X", req->cla, req->ins, req->p1,
            req->p2, req->length);
    print_hex(fp, req->data, req_len - sizeof(struct apdu_request));
    bash_colorize(fp, BASH_CLEAR);
    fputc('\n', fp);
}

inline void euicc_apdu_response_print(FILE *fp, const struct apdu_response *resp) {
    if (fp == NULL)
        return;
    bash_colorize(fp, BASH_RED);
    fprintf(fp, "[DEBUG] [APDU] [RX] SW1: %02X, SW2: %02X", resp->sw1, resp->sw2);
    print_hex(fp, resp->data, resp->length);
    bash_colorize(fp, BASH_CLEAR);
    fputc('\n', fp);
}

inline void euicc_http_request_print(FILE *fp, const char *url, const char *tx) {
    if (fp == NULL)
        return;
    bash_colorize(fp, BASH_GREEN);
    fprintf(fp, "[DEBUG] [HTTP] [TX] URL: %s", url);
    if (strlen(tx) > 0)
        fprintf(fp, ", Data: %s", tx);
    bash_colorize(fp, BASH_CLEAR);
    fputc('\n', fp);
}

inline void euicc_http_response_print(FILE *fp, const uint32_t rcode, const char *rx) {
    if (fp == NULL)
        return;
    bash_colorize(fp, BASH_RED);
    fprintf(fp, "[DEBUG] [HTTP] [RX] RCode: %d", rcode);
    if (strlen(rx) > 0)
        fprintf(fp, ", Data: %s", rx);
    bash_colorize(fp, BASH_CLEAR);
    fputc('\n', fp);
}

inline void euicc_unhandled_tag_print(FILE *fp, const struct euicc_derutil_node *node) {
    if (fp == NULL)
        return;
    fputc('\n', fp);
    bash_colorize(fp, BASH_RED);
    fprintf(fp, "[PLEASE REPORT] [TODO] [TAG %02X]:", node->tag);
    for (uint32_t i = 0; i < node->self.length; i++)
        fprintf(fp, " %02X", node->self.ptr[i]);
    bash_colorize(fp, BASH_CLEAR);
    fputc('\n', fp);
}
