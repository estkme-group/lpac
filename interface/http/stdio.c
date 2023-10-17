#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cjson/cJSON.h>

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

static int json_print(cJSON *jpayload)
{
    cJSON *jroot = NULL;
    char *jstr = NULL;

    if (jpayload == NULL)
    {
        goto err;
    }

    jroot = cJSON_CreateObject();
    if (jroot == NULL)
    {
        goto err;
    }

    if (cJSON_AddStringToObject(jroot, "type", "http") == NULL)
    {
        goto err;
    }

    if (cJSON_AddItemReferenceToObject(jroot, "payload", jpayload) == 0)
    {
        goto err;
    }

    jstr = cJSON_PrintUnformatted(jroot);

    if (jstr == NULL)
    {
        goto err;
    }
    cJSON_Delete(jroot);

    fprintf(stdout, "%s\n", jstr);
    fflush(stdout);

    free(jstr);
    jstr = NULL;

    return 0;

err:
    cJSON_Delete(jroot);
    free(jstr);
    return -1;
}

static int json_request(const char *url, const uint8_t *tx, uint32_t tx_len)
{
    int fret = 0;
    char *tx_hex = NULL;
    cJSON *jpayload = NULL;

    tx_hex = malloc((2 * tx_len) + 1);
    if (tx_hex == NULL)
    {
        goto err;
    }
    if (hexutil_bin2hex(tx_hex, (2 * tx_len) + 1, tx, tx_len) < 0)
    {
        goto err;
    }

    jpayload = cJSON_CreateObject();
    if (jpayload == NULL)
    {
        goto err;
    }
    if (cJSON_AddStringToObject(jpayload, "url", url) == NULL)
    {
        goto err;
    }
    if (cJSON_AddStringToObject(jpayload, "tx", tx_hex) == NULL)
    {
        goto err;
    }
    free(tx_hex);
    tx_hex = NULL;

    fret = json_print(jpayload);
    cJSON_Delete(jpayload);
    jpayload = NULL;
    goto exit;

err:
    fret = -1;
exit:
    cJSON_Delete(jpayload);
    free(tx_hex);
    return fret;
}

int json_parse(uint8_t **rx, uint32_t *rx_len, const char *rx_json)
{
}

static int http_interface_transmit(const char *url, uint32_t *rcode, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len)
{
    *rx = NULL;
    json_request(url, tx, tx_len);
    return -1;
}

int libhttpinterface_main(struct euicc_http_interface *ifstruct)
{
    ifstruct->transmit = http_interface_transmit;

    return 0;
}
