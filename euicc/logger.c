#include "logger.h"

#include "hexutil.h"

#include <string.h>
#include <unistd.h>

// Refs: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
#define COLOR_RED "\x1b[0;31m"
#define COLOR_GREEN "\x1b[0;32m"
#define COLOR_MAGENTA "\x1b[0;35m"
#define COLOR_CLEAR "\x1b[0m"

#ifdef _WIN32
#    define isatty _isatty
#    define fileno _fileno
#endif

#define EMIT_LOG(fp, COLOR, format, ...)                         \
    if (isatty(fileno(fp)))                                      \
        fprintf(fp, COLOR format COLOR_CLEAR "\n", __VA_ARGS__); \
    else                                                         \
        fprintf(fp, format "\n", __VA_ARGS__);

inline void euicc_apdu_request_print(FILE *fp, const struct apdu_request *req, const uint32_t req_len) {
    if (fp == NULL)
        return;
    const size_t n = req_len - sizeof(struct apdu_request);
    char *output = calloc((n * 2) + 1, sizeof(char));
    euicc_hexutil_bin2hex(output, (n * 2) + 1, req->data, n);
    EMIT_LOG(fp, COLOR_GREEN,
             "[DEBUG] [APDU] [TX] "
             "CLA: %02X, INS: %02X, P1: %02X, P2: %02X, Lc: %02X, Data: %s",
             req->cla, req->ins, req->p1, req->p2, req->length, output);
    free(output);
}

inline void euicc_apdu_response_print(FILE *fp, const struct apdu_response *resp) {
    if (fp == NULL)
        return;
    const size_t n = resp->length;
    char *output = calloc((n * 2) + 1, sizeof(char));
    euicc_hexutil_bin2hex(output, (n * 2) + 1, resp->data, n);
    EMIT_LOG(fp, COLOR_RED,
             "[DEBUG] [APDU] [RX] "
             "SW1: %02X, SW2: %02X, Data: %s",
             resp->sw1, resp->sw2, output);
    free(output);
}

inline void euicc_apdu_unhandled_tag_print(FILE *fp, const struct euicc_derutil_node *node) {
    if (fp == NULL)
        return;
    const size_t n = node->self.length;
    char *output = calloc((n * 2) + 1, sizeof(char));
    euicc_hexutil_bin2hex(output, (n * 2) + 1, node->self.ptr, n);
    EMIT_LOG(fp, COLOR_MAGENTA,
             "[DEBUG] [APDU] [UNHANDLED BER-TLV] "
             "[TAG %02X]: %s",
             node->tag, output);
    free(output);
}

inline void euicc_http_request_print(FILE *fp, const char *url, const char *tx) {
    if (fp == NULL)
        return;
    EMIT_LOG(fp, COLOR_GREEN,
             "[DEBUG] [HTTP] [TX] "
             "URL: %s, Data: %s",
             url, tx);
}

inline void euicc_http_response_print(FILE *fp, const uint32_t rcode, const char *rx) {
    if (fp == NULL)
        return;
    EMIT_LOG(fp, COLOR_RED,
             "[DEBUG] [HTTP] [RX] "
             "RCode: %d, Data: %s",
             rcode, rx);
}
