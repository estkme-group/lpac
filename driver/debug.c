#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <euicc/euicc.h>
#include <euicc/interface.h>

static void print_hex(const uint8_t *data, const size_t start, const size_t end) {
  for (size_t i = start; i < end; i++) {
    fprintf(stderr, "%02X", data[i]);
  }
}

static void print_apdu_request(const uint8_t *tx, const uint32_t tx_len) {
  fprintf(stderr, "[DEBUG] [APDU] [TX] ");
  fprintf(stderr, "CLA: %02X, INS: %02X, ", tx[0], tx[1]);
  fprintf(stderr, "P1: %02X, P2: %02X, ", tx[2], tx[3]);
  fprintf(stderr, "Lc: %02X", tx[4]);
  if (tx_len > 5) {
    fprintf(stderr, ", Data: ");
    print_hex(tx, 5, tx_len);
  }
  fprintf(stderr, "\n");
}

static void print_apdu_response(const uint8_t *rx, const uint32_t rx_len) {
  if (rx_len < 2) return;
  const uint8_t sw1 = rx[rx_len - 2];
  const uint8_t sw2 = rx[rx_len - 1];
  fprintf(stderr, "[DEBUG] [APDU] [RX] SW1: %02X, SW2: %02X", sw1, sw2);
  if (rx_len > 2) {
    fprintf(stderr, ", Data: ");
    print_hex(rx, 2, rx_len);
  }
  fprintf(stderr, "\n");
}

static int apdu_interface_transmit(
  struct euicc_ctx *ctx,
  uint8_t **rx, uint32_t *rx_len,
  const uint8_t *tx, const uint32_t tx_len
) {
  print_apdu_request(tx, tx_len);

  const int ret = ctx->apdu.interface->parent->transmit(ctx, rx, rx_len, tx, tx_len);

  if (rx != NULL && rx_len != NULL) {
    print_apdu_response(*rx, *rx_len);
  }

  return ret;
}

static int http_interface_transmit(
  struct euicc_ctx *ctx,
  const char *url, uint32_t *rcode,
  uint8_t **rx, uint32_t *rx_len,
  const uint8_t *tx, const uint32_t tx_len,
  const char **headers
) {
  fprintf(stderr, "[DEBUG] [HTTP] [TX] url: %s, data: %s\n", url, (const char *) tx);
  const int ret = ctx->http.interface->parent->transmit(ctx, url, rcode, rx, rx_len, tx, tx_len, headers);
  if (rcode != NULL && rx != NULL) {
    fprintf(stderr, "[DEBUG] [HTTP] [RX] rcode: %d, data: %s\n", *rcode, (const char *) *rx);
  }
  return ret;
}

void set_apdu_interface_debug(struct euicc_apdu_interface *iface) {
  struct euicc_apdu_interface *parent = malloc(sizeof(struct euicc_apdu_interface));
  memcpy(parent, iface, sizeof(struct euicc_apdu_interface));
  iface->transmit = apdu_interface_transmit;
  iface->userdata = NULL;
  iface->parent = parent;
}

void set_http_interface_debug(struct euicc_http_interface *iface) {
  struct euicc_http_interface *parent = malloc(sizeof(struct euicc_http_interface));
  memcpy(parent, iface, sizeof(struct euicc_http_interface));
  iface->transmit = http_interface_transmit;
  iface->userdata = NULL;
  iface->parent = parent;
}
