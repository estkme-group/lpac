#include "logger.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void apdu_request(const struct euicc_logger *logger, const uint8_t *tx, const uint32_t tx_len) {
    FILE *fp = logger->userdata;
    const bool is_tty = isatty(fileno(fp));
    const uint8_t cla = tx[0];
    const uint8_t ins = tx[1];
    const uint8_t p1 = tx[2];
    const uint8_t p2 = tx[3];
    const uint8_t length = tx[4];
    if (is_tty)
        fprintf(fp, "\033[0;31m");
    fprintf(fp, "[DEBUG] [APDU] [TX] CLA: %02X, INS: %02X, P1: %02X, P2: %02X, Lc: %02X", cla, ins, p1, p2, length);
    if (tx_len > 5)
        fprintf(fp, ", Data: ");
    for (uint32_t i = 5; i < tx_len; i++)
        fprintf(fp, "%02X ", tx[i] & 0xFF);
    fprintf(fp, "\n");
    if (is_tty)
        fprintf(fp, "\033[0;0m");
    fflush(fp);
}

static void apdu_response(const struct euicc_logger *logger, const uint8_t *rx, const uint32_t rx_len) {
    FILE *fp = logger->userdata;
    const bool is_tty = isatty(fileno(fp));
    const uint8_t sw1 = rx[rx_len - 2];
    const uint8_t sw2 = rx[rx_len - 1];
    if (is_tty)
        fprintf(fp, "\033[0;32m");
    fprintf(fp, "[DEBUG] [APDU] [RX] SW1: %02X, SW2: %02X", sw1, sw2);
    if (rx_len > 2)
        fprintf(fp, ", Data: ");
    for (uint32_t i = 0, len = rx_len - 2; i < len; i++)
        fprintf(fp, "%02X ", rx[i] & 0xFF);
    fprintf(fp, "\n");
    if (is_tty)
        fprintf(fp, "\033[0;0m");
    fflush(fp);
}

static void http_request(const struct euicc_logger *logger, const char *url, const char *tx, const uint32_t tx_len) {
    FILE *fp = logger->userdata;
    const bool is_tty = isatty(fileno(fp));
    if (is_tty)
        fprintf(fp, "\033[0;31m");
    fprintf(fp, "[DEBUG] [HTTP] [TX] url: %s, bytes: %d, data: %s\n", url, tx_len, tx);
    if (is_tty)
        fprintf(fp, "\033[0;0m");
    fflush(fp);
}

static void http_response(const struct euicc_logger *logger, const uint32_t rcode, const uint8_t *rx,
                          const uint32_t rx_len) {
    FILE *fp = logger->userdata;
    const bool is_tty = isatty(fileno(fp));
    if (is_tty)
        fprintf(fp, "\033[0;32m");
    fprintf(fp, "[DEBUG] [HTTP] [RX] rcode: %d, bytes: %d, data: %s\n", rcode, rx_len, (char *)rx);
    if (is_tty)
        fprintf(fp, "\033[0;0m");
    fflush(fp);
}

static void unknown_asn1_tag(const struct euicc_logger *logger, const struct euicc_derutil_node *n_iter) {
    FILE *fp = logger->userdata;
    const bool is_tty = isatty(fileno(fp));
    if (is_tty)
        fprintf(fp, "\033[0;31m");
    fprintf(fp, "[PLEASE REPORT][TODO][TAG %02X]: ", n_iter->tag);
    for (uint32_t i = 0; i < n_iter->self.length; i++) {
        fprintf(fp, "%02X ", n_iter->self.ptr[i]);
    }
    fprintf(fp, "\n");
    if (is_tty)
        fprintf(fp, "\033[0;0m");
    fflush(fp);
}

struct euicc_logger *build_euicc_logger(FILE *fp, const bool apdu_debug, const bool http_debug) {
    struct euicc_logger *logger = malloc(sizeof(struct euicc_logger));
    logger->userdata = fp;
    logger->unknown_asn1_tag = unknown_asn1_tag;
    if (apdu_debug) {
        logger->apdu_request = apdu_request;
        logger->apdu_response = apdu_response;
    }
    if (http_debug) {
        logger->http_request = http_request;
        logger->http_response = http_response;
    }
    return logger;
}
