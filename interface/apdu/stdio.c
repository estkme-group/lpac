#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cjson/cJSON_ex.h>

#include <euicc/interface.h>

static int hexutil_bin2hex(char *output, unsigned output_len, const char *bin, int bin_len)
{
    const char hexDigits[] = "0123456789abcdef";

    if (!bin || !output)
    {
        return -1;
    }

    if (output_len < 2 * bin_len + 1)
    {
        return -1;
    }

    for (int i = 0; i < bin_len; ++i)
    {
        char byte = bin[i];
        output[2 * i] = hexDigits[(byte >> 4) & 0x0F];
        output[2 * i + 1] = hexDigits[byte & 0x0F];
    }
    output[2 * bin_len] = '\0';

    return 0;
}

static int hexutil_hex2bin(char *output, unsigned output_len, const char *str, unsigned str_len)
{
    int length;

    if (!str || !output || str_len % 2 != 0)
    {
        return -1;
    }

    length = str_len / 2;
    if (length > output_len)
    {
        return -1;
    }

    for (int i = 0; i < length; ++i)
    {
        char high = str[2 * i];
        char low = str[2 * i + 1];

        if (high >= '0' && high <= '9')
        {
            high -= '0';
        }
        else if (high >= 'a' && high <= 'f')
        {
            high = high - 'a' + 10;
        }
        else if (high >= 'A' && high <= 'F')
        {
            high = high - 'A' + 10;
        }
        else
        {
            return -1;
        }

        if (low >= '0' && low <= '9')
        {
            low -= '0';
        }
        else if (low >= 'a' && low <= 'f')
        {
            low = low - 'a' + 10;
        }
        else if (low >= 'A' && low <= 'F')
        {
            low = low - 'A' + 10;
        }
        else
        {
            return -1;
        }

        output[i] = (high << 4) + low;
    }

    return length;
}

// getline is a GNU extension, Mingw32 macOS and FreeBSD don't have (a working) one
static int afgets(char **obuf, FILE *fp)
{
    unsigned int len = 0;
    char buffer[2];
    char *obuf_new = NULL;

    *obuf = malloc(1);
    if ((*obuf) == NULL)
    {
        goto err;
    }
    (*obuf)[0] = '\0';

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        unsigned int fgets_len = strlen(buffer);

        len += fgets_len + 1;
        obuf_new = realloc(*obuf, len);
        if (obuf_new == NULL)
        {
            goto err;
        }
        *obuf = obuf_new;
        strcat(*obuf, buffer);

        if (buffer[fgets_len - 1] == '\n')
        {
            break;
        }
    }

    return 0;

err:
    free(*obuf);
    *obuf = NULL;
    return -1;
}

static int apdu_interface_connect(struct euicc_ctx *ctx)
{
}

static void apdu_interface_disconnect(struct euicc_ctx *ctx)
{
}

static int apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, uint8_t aid_len)
{
}

static void apdu_interface_logic_channel_close(struct euicc_ctx *ctx, uint8_t channel)
{
}

static int apdu_interface_transmit(struct euicc_ctx *ctx, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len)
{
}

int libapduinterface_init(struct euicc_apdu_interface *ifstruct)
{
    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = apdu_interface_disconnect;
    ifstruct->logic_channel_open = apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = apdu_interface_logic_channel_close;
    ifstruct->transmit = apdu_interface_transmit;

    return 0;
}

int libapduinterface_main(int argc, char **argv)
{
    return 0;
}
