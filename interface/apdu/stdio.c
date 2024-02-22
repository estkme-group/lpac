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

    if (cJSON_AddStringOrNullToObject(jroot, "type", "apdu") == NULL)
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

static int json_request(const char *func, const uint8_t *param, unsigned param_len)
{
    int fret = 0;
    char *param_hex = NULL;
    cJSON *jpayload = NULL;

    if (param && param_len)
    {
        param_hex = malloc((2 * param_len) + 1);
        if (param_hex == NULL)
        {
            goto err;
        }
        if (hexutil_bin2hex(param_hex, (2 * param_len) + 1, param, param_len) < 0)
        {
            goto err;
        }
    }
    else
    {
        param_hex = NULL;
    }

    jpayload = cJSON_CreateObject();
    if (jpayload == NULL)
    {
        goto err;
    }
    if (cJSON_AddStringOrNullToObject(jpayload, "func", func) == NULL)
    {
        goto err;
    }
    if (cJSON_AddStringOrNullToObject(jpayload, "param", param_hex) == NULL)
    {
        goto err;
    }
    free(param_hex);
    param_hex = NULL;

    fret = json_print(jpayload);
    cJSON_Delete(jpayload);
    jpayload = NULL;
    goto exit;

err:
    fret = -1;
exit:
    cJSON_Delete(jpayload);
    free(param_hex);
    return fret;
}

static int json_response(int *ecode, uint8_t **data, uint32_t *data_len)
{
    int fret = 0;
    char *data_json;
    cJSON *data_jroot;
    cJSON *data_payload;
    cJSON *jtmp;

    if (data)
    {
        *data = NULL;
    }

    if (afgets(&data_json, stdin) < 0)
    {
        return -1;
    }

    data_jroot = cJSON_Parse(data_json);
    free(data_json);
    data_json = NULL;
    if (data_jroot == NULL)
    {
        return -1;
    }

    jtmp = cJSON_GetObjectItem(data_jroot, "type");
    if (!jtmp)
    {
        goto err;
    }
    if (!cJSON_IsString(jtmp))
    {
        goto err;
    }
    if (strcmp("apdu", jtmp->valuestring) != 0)
    {
        goto err;
    }

    data_payload = cJSON_GetObjectItem(data_jroot, "payload");
    if (!data_payload)
    {
        goto err;
    }
    if (!cJSON_IsObject(data_payload))
    {
        goto err;
    }

    jtmp = cJSON_GetObjectItem(data_payload, "ecode");
    if (!jtmp)
    {
        goto err;
    }
    if (!cJSON_IsNumber(jtmp))
    {
        goto err;
    }
    *ecode = jtmp->valueint;

    jtmp = cJSON_GetObjectItem(data_payload, "data");
    if (jtmp && cJSON_IsString(jtmp) && data && data_len)
    {
        *data_len = strlen(jtmp->valuestring) / 2;
        *data = malloc(*data_len);
        if (!*data)
        {
            goto err;
        }
        if (hexutil_hex2bin(*data, *data_len, jtmp->valuestring, strlen(jtmp->valuestring)) < 0)
        {
            goto err;
        }
    }

    fret = 0;
    goto exit;

err:
    fret = -1;
    free(*data);
    if (data)
    {
        *data = NULL;
    }
    if (data_len)
    {
        *data_len = 0;
    }
    *ecode = -1;
exit:
    free(data_json);
    cJSON_Delete(data_jroot);
    return fret;
}

// {"type":"apdu","payload":{"ecode":0}}
static int apdu_interface_connect(struct euicc_ctx *ctx)
{
    int ecode;

    if (json_request("connect", NULL, 0))
    {
        return -1;
    }

    if (json_response(&ecode, NULL, NULL))
    {
        return -1;
    }

    return ecode;
}

// {"type":"apdu","payload":{"ecode":0}}
static void apdu_interface_disconnect(struct euicc_ctx *ctx)
{
    int ecode;

    json_request("disconnect", NULL, 0);
    json_response(&ecode, NULL, NULL);
}

// {"type":"apdu","payload":{"ecode":1}}
static int apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, uint8_t aid_len)
{
    int ecode;

    if (json_request("logic_channel_open", aid, aid_len))
    {
        return -1;
    }

    if (json_response(&ecode, NULL, NULL))
    {
        return -1;
    }

    return ecode;
}

// {"type":"apdu","payload":{"ecode":0}}
static void apdu_interface_logic_channel_close(struct euicc_ctx *ctx, uint8_t channel)
{
    int ecode;

    json_request("logic_channel_close", &channel, sizeof(channel));
    json_response(&ecode, NULL, NULL);
}

// {"type":"apdu","payload":{"ecode":0,"data":"BF3E125A10890490320010012345000123456789019000"}}
// {"type":"apdu","payload":{"ecode":0,"data":"BF3C17811574657374726F6F74736D64732E67736D612E636F6D9000"}}
// {"type":"apdu","payload":{"ecode":0,"data":"BF2281C6810302010082030202008303040600840F8101008204000628248304000019228504067F36C08603090200870302030088020490A916041481370F5125D0B1D408D4C3B232E6D25E795BEBFBAA16041481370F5125D0B1D408D4C3B232E6D25E795BEBFB990206C004030000010C0D47492D42412D55502D30343139AC48801F312E322E3834302E313233343536372F6D79506C6174666F726D4C6162656C812568747470733A2F2F6D79636F6D70616E792E636F6D2F6D79444C4F415265676973747261729000"}}
static int apdu_interface_transmit(struct euicc_ctx *ctx, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len)
{
    int ecode;

    if (json_request("transmit", tx, tx_len))
    {
        return -1;
    }

    if (json_response(&ecode, rx, rx_len))
    {
        return -1;
    }

    return ecode;
}

EUICC_SHARED_EXPORT int libapduinterface_init(struct euicc_apdu_interface *ifstruct)
{
    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = apdu_interface_disconnect;
    ifstruct->logic_channel_open = apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = apdu_interface_logic_channel_close;
    ifstruct->transmit = apdu_interface_transmit;

    return 0;
}

EUICC_SHARED_EXPORT int libapduinterface_main(int argc, char **argv)
{
    return 0;
}
