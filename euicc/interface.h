#pragma once

#define EUICC_INTERFACE_BUFSZ 264

struct euicc_apdu_interface
{
    int (*connect)(void);
    void (*disconnect)(void);
    int (*logic_channel_open)(const unsigned char *aid, unsigned char aid_len);
    void (*logic_channel_close)(unsigned char channel);
    int (*transmit)(unsigned char *tx, unsigned int tx_len, unsigned char *rx, unsigned long *rx_len);
};

struct euicc_es9p_interface
{
    int (*transmit)(unsigned int *rcode, unsigned char **rbuf, const char *url, const unsigned char *sbuf, unsigned int slen);
};
