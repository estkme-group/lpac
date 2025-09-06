#include "logger.h"

#include <unistd.h>

// Refs: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
#define COLOR_RED 31
#define COLOR_GREEN 32
#define COLOR_CLEAR 0

static void colorize(FILE *fp, const int color) {
#if _WIN32
    // https://devblogs.microsoft.com/commandline/windows-command-line-introducing-the-windows-pseudo-console-conpty/
    // Minimum supported version: Windows 10 version 1809
    if (_isatty(_fileno(fp)))
#else
    if (isatty(fileno(fp)))
#endif
        fprintf(fp, "\033[0;%dm", color);
}

static void euicc_hex_print(FILE *fp, const uint8_t *data, const uint32_t length) {
    for (uint32_t i = 0; i < length; i++)
        fprintf(fp, " %02X", data[i] & 0xFF);
}

inline void euicc_apdu_request_print(FILE *fp, const struct apdu_request *req, const uint32_t req_len) {
    if (fp == NULL)
        return;
    colorize(fp, COLOR_GREEN);
    fprintf(fp, "[DEBUG] [APDU] [TX] CLA: %02X, INS: %02X, P1: %02X, P2: %02X, Lc: %02X, Data:", req->cla, req->ins,
            req->p1, req->p2, req->length);
    euicc_hex_print(fp, req->data, req_len - sizeof(struct apdu_request));
    colorize(fp, COLOR_CLEAR);
    fputc('\n', fp);
}

inline void euicc_apdu_response_print(FILE *fp, const struct apdu_response *resp) {
    if (fp == NULL)
        return;
    colorize(fp, COLOR_RED);
    fprintf(fp, "[DEBUG] [APDU] [RX] SW1: %02X, SW2: %02X, Data:", resp->sw1, resp->sw2);
    euicc_hex_print(fp, resp->data, resp->length);
    colorize(fp, COLOR_CLEAR);
    fputc('\n', fp);
}

inline void euicc_http_request_print(FILE *fp, const char *url, const char *tx) {
    if (fp == NULL)
        return;
    colorize(fp, COLOR_GREEN);
    fprintf(fp, "[DEBUG] [HTTP] [TX] URL: %s, Data: %s", url, tx);
    colorize(fp, COLOR_CLEAR);
    fputc('\n', fp);
}

inline void euicc_http_response_print(FILE *fp, const uint32_t rcode, const char *rx) {
    if (fp == NULL)
        return;
    colorize(fp, COLOR_RED);
    fprintf(fp, "[DEBUG] [HTTP] [RX] RCode: %d, Data: %s", rcode, rx);
    colorize(fp, COLOR_CLEAR);
    fputc('\n', fp);
}

inline void euicc_unhandled_tag_print(FILE *fp, const struct euicc_derutil_node *node) {
    if (fp == NULL)
        return;
    fputc('\n', fp);
    colorize(fp, COLOR_RED);
    fprintf(fp, "[PLEASE REPORT] [TODO] [TAG %02X]:", node->tag);
    euicc_hex_print(fp, node->self.ptr, node->self.length);
    colorize(fp, COLOR_CLEAR);
    fputc('\n', fp);
}
