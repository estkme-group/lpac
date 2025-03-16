#include "at.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <helpers.h>
#include <euicc/euicc.h>
#include <euicc/interface.h>
#include <euicc/hexutil.h>

#define AT_BUFFER_SIZE 20480

static FILE *fuart;
static int logic_channel = 0;
static char *buffer;
static int at_debug;

struct at_userdata {
    int debug;
    char *device;
};

#define USERDATA(ctx) ((struct at_userdata *)ctx->apdu.interface->userdata)

static void free_userdata(struct at_userdata *userdata)
{
    if (userdata == NULL) return;
    free(userdata->device);
    free(userdata);
}

static int at_expect(char **response, const char *expected)
{
    memset(buffer, 0, AT_BUFFER_SIZE);

    if (response)
        *response = NULL;

    while (1)
    {
        fgets(buffer, AT_BUFFER_SIZE, fuart);
        buffer[strcspn(buffer, "\r\n")] = 0;
        if (at_debug)
            printf("AT_DEBUG: %s\n", buffer);
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

static int apdu_interface_connect(const struct euicc_ctx *ctx)
{
    at_debug = USERDATA(ctx)->debug;
    const char *device = USERDATA(ctx)->device;

    logic_channel = 0;

    fuart = fopen(device, "r+");
    if (fuart == NULL)
    {
        fprintf(stderr, "Failed to open device: %s\n", device);
        return -1;
    }
    setbuf(fuart, NULL);

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
    at_debug = USERDATA(ctx)->debug;
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
    at_debug = USERDATA(ctx)->debug;
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
    at_debug = USERDATA(ctx)->debug;
    if (!logic_channel)
    {
        return;
    }
    fprintf(fuart, "AT+CCHC=%d\r\n", logic_channel);
    at_expect(NULL, NULL);
}

static void *libapduinterface_userdata() {
    struct at_userdata *userdata = malloc(sizeof(struct at_userdata));
    memset(userdata, 0, sizeof(struct at_userdata));

    const char *at_debug_name = APDU_ENV_NAME("AT", "DEBUG");
    const char *at_device_name = APDU_ENV_NAME("AT", "DEVICE");

    set_deprecated_env(at_debug_name, "AT_DEBUG");
    set_deprecated_env(at_device_name, "AT_DEVICE");

    char *value = NULL;

    value = getenv(at_debug_name);
    userdata->debug = value != NULL;

    userdata->device = "/dev/ttyUSB0";
    value = getenv(at_device_name);
    if (value != NULL) {
        userdata->device = value;
    }
    return userdata;
}

static int libapduinterface_init(struct euicc_apdu_interface *ifstruct)
{
    memset(ifstruct, 0, sizeof(struct euicc_apdu_interface));

    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = apdu_interface_disconnect;
    ifstruct->logic_channel_open = apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = apdu_interface_logic_channel_close;
    ifstruct->transmit = apdu_interface_transmit;

    buffer = malloc(AT_BUFFER_SIZE);
    if (!buffer)
    {
        fprintf(stderr, "Failed to allocate memory\n");
        return -1;
    }

    return 0;
}

static int libapduinterface_main(int argc, char **argv)
{
    return 0;
}

static void libapduinterface_fini(struct euicc_apdu_interface *ifstruct)
{
    free(buffer);
    free_userdata((struct at_userdata *) ifstruct->userdata);
}

const struct euicc_driver driver_apdu_at = {
    .type = DRIVER_APDU,
    .name = "at",
#ifdef LPAC_ENV_VARS
    .userdata = (void *(*)())libapduinterface_userdata,
#else
    .userdata = NULL,
#endif
    .init = (int (*)(void *))libapduinterface_init,
    .main = libapduinterface_main,
    .fini = (void (*)(void *))libapduinterface_fini,
};
