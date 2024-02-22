#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cjson/cJSON_ex.h>

#include <euicc/interface.h>

static int hexutil_bin2hex(char *output, uint32_t output_len, const uint8_t *bin, uint32_t bin_len)
{
    const char hexDigits[] = "0123456789abcdef";

    if (!bin || !output)
    {
        return -1;
    }

    if (output_len < (2 * bin_len + 1))
    {
        return -1;
    }

    for (uint32_t i = 0; i < bin_len; ++i)
    {
        char byte = bin[i];
        output[2 * i] = hexDigits[(byte >> 4) & 0x0F];
        output[2 * i + 1] = hexDigits[byte & 0x0F];
    }
    output[2 * bin_len] = '\0';

    return 0;
}

static int hexutil_hex2bin(uint8_t *output, uint32_t output_len, const char *str, uint32_t str_len)
{
    uint32_t length;

    if (!str || !output || str_len % 2 != 0)
    {
        return -1;
    }

    length = str_len / 2;
    if (length > output_len)
    {
        return -1;
    }

    for (uint32_t i = 0; i < length; ++i)
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
    uint32_t len = 0;
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
        uint32_t fgets_len = strlen(buffer);

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

    (*obuf)[strcspn(*obuf, "\r\n")] = 0;

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

    if (cJSON_AddStringOrNullToObject(jroot, "type", "http") == NULL)
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

static int json_request(const char *url, const uint8_t *tx, uint32_t tx_len, const char **headers)
{
    int fret = 0;
    char *tx_hex = NULL;
    cJSON *jpayload = NULL;
    cJSON *jheaders = NULL;

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
    if (cJSON_AddStringOrNullToObject(jpayload, "url", url) == NULL)
    {
        goto err;
    }
    if (cJSON_AddStringOrNullToObject(jpayload, "tx", tx_hex) == NULL)
    {
        goto err;
    }
    free(tx_hex);
    tx_hex = NULL;

    jheaders = cJSON_AddArrayToObject(jpayload, "headers");
    if (jheaders == NULL)
    {
        goto err;
    }

    for (int i = 0; headers[i] != NULL; i++)
    {
        cJSON *jh = cJSON_CreateString(headers[i]);
        if (jh == NULL)
        {
            goto err;
        }
        cJSON_AddItemToArray(jheaders, jh);
    }

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

// {"type":"http","payload":{"rcode":404,"rx":"333435"}}
static int http_interface_transmit(struct euicc_ctx *ctx, const char *url, uint32_t *rcode, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len, const char **headers)
{
    int fret = 0;
    char *rx_json;
    cJSON *rx_jroot;
    cJSON *rx_payload;
    cJSON *jtmp;

    *rx = NULL;

    json_request(url, tx, tx_len, headers);
    if (afgets(&rx_json, stdin) < 0)
    {
        return -1;
    }

    rx_jroot = cJSON_Parse(rx_json);
    free(rx_json);
    rx_json = NULL;
    if (rx_jroot == NULL)
    {
        return -1;
    }

    jtmp = cJSON_GetObjectItem(rx_jroot, "type");
    if (!jtmp)
    {
        goto err;
    }
    if (!cJSON_IsString(jtmp))
    {
        goto err;
    }
    if (strcmp("http", jtmp->valuestring) != 0)
    {
        goto err;
    }

    rx_payload = cJSON_GetObjectItem(rx_jroot, "payload");
    if (!rx_payload)
    {
        goto err;
    }
    if (!cJSON_IsObject(rx_payload))
    {
        goto err;
    }

    jtmp = cJSON_GetObjectItem(rx_payload, "rcode");
    if (!jtmp)
    {
        goto err;
    }
    if (!cJSON_IsNumber(jtmp))
    {
        goto err;
    }
    *rcode = jtmp->valueint;

    jtmp = cJSON_GetObjectItem(rx_payload, "rx");
    if (!jtmp)
    {
        goto err;
    }
    if (!cJSON_IsString(jtmp))
    {
        goto err;
    }
    *rx_len = strlen(jtmp->valuestring) / 2;
    *rx = malloc(*rx_len);
    if (!*rx)
    {
        goto err;
    }
    if (hexutil_hex2bin(*rx, *rx_len, jtmp->valuestring, strlen(jtmp->valuestring)) < 0)
    {
        goto err;
    }

    fret = 0;
    goto exit;

err:
    fret = -1;
    free(*rx);
    *rx = NULL;
    *rx_len = 0;
    *rcode = 500;
exit:
    free(rx_json);
    cJSON_Delete(rx_jroot);
    return fret;
}

EUICC_SHARED_EXPORT int libhttpinterface_init(struct euicc_http_interface *ifstruct)
{
    memset(ifstruct, 0, sizeof(struct euicc_http_interface));

    ifstruct->transmit = http_interface_transmit;

    return 0;
}

EUICC_SHARED_EXPORT int libhttpinterface_main(int argc, char **argv)
{
    return 0;
}
