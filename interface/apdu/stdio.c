#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <euicc/interface.h>

static int apdu_interface_connect(void)
{

}

static void apdu_interface_disconnect(void)
{

}

static int apdu_interface_logic_channel_open(const uint8_t *aid, uint8_t aid_len)
{

}

static void apdu_interface_logic_channel_close(uint8_t channel)
{

}

static int apdu_interface_transmit(uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len)
{

}

int libapduinterface_main(struct euicc_apdu_interface *ifstruct)
{
    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = apdu_interface_disconnect;
    ifstruct->logic_channel_open = apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = apdu_interface_logic_channel_close;
    ifstruct->transmit = apdu_interface_transmit;

    return 0;
}
