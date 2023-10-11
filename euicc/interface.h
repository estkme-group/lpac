#pragma once
#include <inttypes.h>

struct euicc_apdu_interface
{
    int (*connect)(void);
    void (*disconnect)(void);
    int (*logic_channel_open)(const uint8_t *aid, uint8_t aid_len);
    void (*logic_channel_close)(uint8_t channel);
    int (*transmit)(uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len);
};

struct euicc_http_interface
{
    int (*transmit)(const char *url, uint32_t *rcode, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len);
};
