#pragma once
#include <inttypes.h>

struct euicc_ctx;

struct euicc_apdu_interface
{
    int (*connect)(struct euicc_ctx *ctx);
    void (*disconnect)(struct euicc_ctx *ctx);
    int (*logic_channel_open)(struct euicc_ctx *ctx, const uint8_t *aid, uint8_t aid_len);
    void (*logic_channel_close)(struct euicc_ctx *ctx, uint8_t channel);
    int (*transmit)(struct euicc_ctx *ctx, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len);
    void *userdata;
};

struct euicc_http_interface
{
    int (*transmit)(struct euicc_ctx *ctx, const char *url, uint32_t *rcode, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len, const char **headers);
    void *userdata;
};
