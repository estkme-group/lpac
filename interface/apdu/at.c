#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static FILE *fuart;
static int logic_channel;
static int logic_channel_available = 0;

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

static int at_expect(char **response, const char *expected)
{
    uint8_t buffer[1024];

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

int euicc_apdu_interface_connect(void)
{
    const char *device;

    logic_channel_available = 0;

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

void euicc_apdu_interface_disconnect(void)
{
    fclose(fuart);
    fuart = NULL;
    logic_channel_available = 0;
}

int euicc_apdu_interface_transmit(unsigned char *tx, unsigned int tx_len, unsigned char *rx, long unsigned int *rx_len)
{
    int fret = 0;
    int ret;
    char *response = NULL;
    char *hexstr;

    if (!logic_channel_available)
    {
        *rx_len = 2;
        rx[0] = 0x6F;
        rx[1] = 0x00;
        return 0;
    }

    fprintf(fuart, "AT+CGLA=%d,%u,\"", logic_channel, tx_len * 2);
    for (int i = 0; i < tx_len; i++)
    {
        fprintf(fuart, "%02X", tx[i] & 0xFF);
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
    hexstr[strcspn(hexstr, "\"")] = 0;

    ret = hexutil_hex2bin(rx, *rx_len, hexstr, strlen(hexstr));
    if (ret < 0)
    {
        goto err;
    }
    *rx_len = ret;

    goto exit;

err:
    fret = -1;
exit:
    free(response);
    return fret;
}

int euicc_apdu_interface_logic_channel_open(const unsigned char *aid, unsigned char aid_len)
{
    char *response;

    if (!logic_channel_available)
    {
        for (int i = 1; i <= 4; i++)
        {
            fprintf(fuart, "AT+CCHC=%d\r\n", i);
            at_expect(NULL, NULL);
        }
        fprintf(fuart, "AT+CCHO=\"");
        for (int i = 0; i < aid_len; i++)
        {
            fprintf(fuart, "%02X", aid[i] & 0xFF);
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
        logic_channel_available = 1;
    }

    return logic_channel;
}

void euicc_apdu_interface_logic_channel_close(unsigned char channel)
{
    fprintf(fuart, "AT+CCHC=%d\r\n", logic_channel);
    at_expect(NULL, NULL);
    logic_channel_available = 0;
}
