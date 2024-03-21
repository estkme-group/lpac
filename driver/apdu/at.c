#include "at.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <euicc/interface.h>
#include <euicc/hexutil.h>

static FILE *fuart;
static int logic_channel = 0;

static int at_expect(char **response, const char *expected)
{
    char buffer[1024];

    if (response)
        *response = NULL;

    while (1)
    {
        fgets(buffer, sizeof(buffer), fuart);
        buffer[strcspn(buffer, "\r\n")] = 0;
        if (getenv("AT_DEBUG"))
            printf("AT_DEBUG: %s\r\n", buffer);
        if (strcmp(buffer, "ERROR") == 0)
        {
            return -1;
        }
        else if (strcmp(buffer, "OK") == 0)
        {
            return 0;
        }
        else if (expected && strncmp(buffer, expected, strlen(expected)) == 0)
        {
            if (response)
                *response = strdup(buffer + strlen(expected));
        }
    }
    return 0;
}

static int apdu_interface_connect(struct euicc_ctx *ctx)
{
    const char *device;

    logic_channel = 0;

    if (!(device = getenv("AT_DEVICE")))
    {
        device = "/dev/ttyUSB0";
    }

    fuart = fopen(device, "r+");
    if (fuart == NULL)
    {
        fprintf(stderr, "Failed to open device: %s\n", device);
        return -1;
    }

    fprintf(fuart, "AT+CCHO=?\r\n");
    if (at_expect(NULL, NULL))
    {
        fprintf(stderr, "Device missing AT+CCHO support\n");
        return -1;
    }
    fprintf(fuart, "AT+CCHC=?\r\n");
    if (at_expect(NULL, NULL))
    {
        fprintf(stderr, "Device missing AT+CCHC support\n");
        return -1;
    }
    fprintf(fuart, "AT+CGLA=?\r\n");
    if (at_expect(NULL, NULL))
    {
        fprintf(stderr, "Device missing AT+CGLA support\n");
        return -1;
    }

    return 0;
}

static void apdu_interface_disconnect(struct euicc_ctx *ctx)
{
    fclose(fuart);
    fuart = NULL;
    logic_channel = 0;
}

static int apdu_interface_transmit(struct euicc_ctx *ctx, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len)
{
    int fret = 0;
    int ret;
    char *response = NULL;
    char *hexstr = NULL;

    *rx = NULL;
    *rx_len = 0;

    if (!logic_channel)
    {
        return -1;
    }

    fprintf(fuart, "AT+CGLA=%d,%u,\"", logic_channel, tx_len * 2);
    for (uint32_t i = 0; i < tx_len; i++)
    {
        fprintf(fuart, "%02X", (uint8_t)(tx[i] & 0xFF));
    }
    fprintf(fuart, "\"\r\n");
    if (at_expect(&response, "+CGLA: "))
    {
        goto err;
    }
    if (response == NULL)
    {
        goto err;
    }

    strtok(response, ",");
    hexstr = strtok(NULL, ",");
    if (!hexstr)
    {
        goto err;
    }
    if (hexstr[0] == '"')
    {
        hexstr++;
    }
    hexstr[strcspn(hexstr, "\"")] = '\0';

    *rx_len = strlen(hexstr) / 2;
    *rx = malloc(*rx_len);
    if (!*rx)
    {
        goto err;
    }

    ret = euicc_hexutil_hex2bin_r(*rx, *rx_len, hexstr, strlen(hexstr));
    if (ret < 0)
    {
        goto err;
    }
    *rx_len = ret;

    goto exit;

err:
    fret = -1;
    free(*rx);
    *rx = NULL;
    *rx_len = 0;
exit:
    free(response);
    return fret;
}

static int apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, uint8_t aid_len)
{
    char *response;

    if (logic_channel)
    {
        return logic_channel;
    }

    for (int i = 1; i <= 4; i++)
    {
        fprintf(fuart, "AT+CCHC=%d\r\n", i);
        at_expect(NULL, NULL);
    }
    fprintf(fuart, "AT+CCHO=\"");
    for (int i = 0; i < aid_len; i++)
    {
        fprintf(fuart, "%02X", (uint8_t)(aid[i] & 0xFF));
    }
    fprintf(fuart, "\"\r\n");
    if (at_expect(&response, "+CCHO: "))
    {
        return -1;
    }
    if (response == NULL)
    {
        return -1;
    }
    logic_channel = atoi(response);

    return logic_channel;
}

static void apdu_interface_logic_channel_close(struct euicc_ctx *ctx, uint8_t channel)
{
    if (!logic_channel)
    {
        return;
    }
    fprintf(fuart, "AT+CCHC=%d\r\n", logic_channel);
    at_expect(NULL, NULL);
}

static int libapduinterface_init(struct euicc_apdu_interface *ifstruct)
{
    memset(ifstruct, 0, sizeof(struct euicc_apdu_interface));

    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = apdu_interface_disconnect;
    ifstruct->logic_channel_open = apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = apdu_interface_logic_channel_close;
    ifstruct->transmit = apdu_interface_transmit;

    return 0;
}

static int libapduinterface_main(int argc, char **argv)
{
    return 0;
}

static void libapduinterface_fini(void)
{
}

const struct euicc_driver driver_apdu_at = {
    .type = DRIVER_APDU,
    .name = "at",
    .init = (int (*)(void *))libapduinterface_init,
    .main = libapduinterface_main,
    .fini = libapduinterface_fini,
};
