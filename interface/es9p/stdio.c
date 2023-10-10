#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <euicc/interface.h>

static int es9p_interface_transmit(const char *url, uint32_t *rcode, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len)
{
}

int libes9pinterface_main(struct euicc_es9p_interface *ifstruct)
{
    ifstruct->transmit = es9p_interface_transmit;

    return 0;
}
