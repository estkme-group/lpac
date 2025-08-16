#include "stdio.h"

#include "euicc/base64.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cjson/cJSON_ex.h>
#include <euicc/hexutil.h>
#include <euicc/interface.h>
#include <lpac/utils.h>

#define ENV_STDIO_APDU_TYPE APDU_ENV_NAME(STDIO, TYPE)

struct stdio_userdata {
    int (*encode)(char *encoded, const unsigned char *tx, int tx_len);
    int (*encode_len)(int input);
    int (*decode)(uint8_t *rx, const char *encoded);
    int (*decode_len)(const char *encoded);
};

static int hexify_encode_len(const int input) {
    return (input * 2) + 1; // +1 for the null terminator
}

static int hexify_encode(char *encoded, const unsigned char *tx, const int tx_len) {
    return euicc_hexutil_bin2hex(encoded, hexify_encode_len(tx_len), tx, tx_len);
}

static int hexify_decode_len(const char *encoded) { return (int)(strlen(encoded) / 2); }

static int hexify_decode(uint8_t *rx, const char *encoded) {
    return euicc_hexutil_hex2bin(rx, hexify_decode_len(encoded), encoded);
}

// getline is a GNU extension, Mingw32 macOS and FreeBSD don't have (a working) one
static int afgets(char **obuf, FILE *fp) {
    uint32_t len = 0;
    char buffer[2];
    char *obuf_new = NULL;

    *obuf = malloc(1);
    if ((*obuf) == NULL) {
        goto err;
    }
    (*obuf)[0] = '\0';

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        uint32_t fgets_len = strlen(buffer);

        len += fgets_len + 1;
        obuf_new = realloc(*obuf, len);
        if (obuf_new == NULL) {
            goto err;
        }
        *obuf = obuf_new;
        strcat(*obuf, buffer);

        if (buffer[fgets_len - 1] == '\n') {
            break;
        }
    }

    (*obuf)[strcspn(*obuf, "\n")] = 0;

    return 0;

err:
    free(*obuf);
    *obuf = NULL;
    return -1;
}

static bool json_request(const struct stdio_userdata *userdata, const char *func, const uint8_t *param,
                         const int param_len) {
    _cleanup_free_ char *param_hex = NULL;
    _cleanup_cjson_ cJSON *jpayload = NULL;

    if (param && param_len) {
        param_hex = malloc(userdata->encode_len(param_len));
        if (param_hex == NULL) {
            return false;
        }
        if (userdata->encode(param_hex, param, param_len) < 0) {
            return false;
        }
    } else {
        param_hex = NULL;
    }

    jpayload = cJSON_CreateObject();
    if (jpayload == NULL) {
        return false;
    }
    if (cJSON_AddStringOrNullToObject(jpayload, "func", func) == NULL) {
        return false;
    }
    if (cJSON_AddStringOrNullToObject(jpayload, "param", param_hex) == NULL) {
        return false;
    }

    return json_print("apdu", jpayload);
}

static int json_response(const struct stdio_userdata *userdata, int *ecode, uint8_t **data, uint32_t *data_len) {
    int fret = 0;
    _cleanup_free_ char *data_json;
    _cleanup_cjson_ cJSON *data_jroot;
    cJSON *data_payload;
    cJSON *jtmp;

    if (data) {
        *data = NULL;
    }

    if (afgets(&data_json, stdin) < 0) {
        return -1;
    }

    data_jroot = cJSON_Parse(data_json);
    if (data_jroot == NULL) {
        return -1;
    }

    jtmp = cJSON_GetObjectItem(data_jroot, "type");
    if (!jtmp) {
        goto err;
    }
    if (!cJSON_IsString(jtmp)) {
        goto err;
    }
    if (strcmp("apdu", jtmp->valuestring) != 0) {
        goto err;
    }

    data_payload = cJSON_GetObjectItem(data_jroot, "payload");
    if (!data_payload) {
        goto err;
    }
    if (!cJSON_IsObject(data_payload)) {
        goto err;
    }

    jtmp = cJSON_GetObjectItem(data_payload, "ecode");
    if (!jtmp) {
        goto err;
    }
    if (!cJSON_IsNumber(jtmp)) {
        goto err;
    }
    *ecode = jtmp->valueint;

    jtmp = cJSON_GetObjectItem(data_payload, "data");
    if (jtmp != NULL && cJSON_IsString(jtmp) && data != NULL && data_len != NULL) {
        *data_len = userdata->decode_len(jtmp->valuestring);
        *data = malloc(*data_len);
        if (!*data) {
            goto err;
        }
        if (userdata->decode(*data, jtmp->valuestring) < 0) {
            goto err;
        }
    }

    fret = 0;
    goto exit;

err:
    fret = -1;
    if (data != NULL && *data != NULL) {
        free(*data);
        *data = NULL;
    }
    if (data_len) {
        *data_len = 0;
    }
    *ecode = -1;
exit:
    return fret;
}

// {"type":"apdu","payload":{"ecode":0}}
static int apdu_interface_connect(struct euicc_ctx *ctx) {
    const struct stdio_userdata *userdata = ctx->http.interface->userdata;

    int ecode;

    if (json_request(userdata, "connect", NULL, 0)) {
        return -1;
    }

    if (json_response(userdata, &ecode, NULL, NULL)) {
        return -1;
    }

    return ecode;
}

// {"type":"apdu","payload":{"ecode":0}}
static void apdu_interface_disconnect(struct euicc_ctx *ctx) {
    const struct stdio_userdata *userdata = ctx->http.interface->userdata;

    int ecode;

    json_request(userdata, "disconnect", NULL, 0);
    json_response(userdata, &ecode, NULL, NULL);
}

// {"type":"apdu","payload":{"ecode":1}}
static int apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, uint8_t aid_len) {
    const struct stdio_userdata *userdata = ctx->http.interface->userdata;

    int ecode;

    if (json_request(userdata, "logic_channel_open", aid, aid_len)) {
        return -1;
    }

    if (json_response(userdata, &ecode, NULL, NULL)) {
        return -1;
    }

    return ecode;
}

// {"type":"apdu","payload":{"ecode":0}}
static void apdu_interface_logic_channel_close(struct euicc_ctx *ctx, const uint8_t channel) {
    const struct stdio_userdata *userdata = ctx->http.interface->userdata;

    int ecode;

    json_request(userdata, "logic_channel_close", &channel, sizeof(channel));
    json_response(userdata, &ecode, NULL, NULL);
}

// {"type":"apdu","payload":{"ecode":0,"data":"BF3E125A10890490320010012345000123456789019000"}}
// {"type":"apdu","payload":{"ecode":0,"data":"BF3C17811574657374726F6F74736D64732E67736D612E636F6D9000"}}
// {"type":"apdu","payload":{"ecode":0,"data":"BF2281C6810302010082030202008303040600840F8101008204000628248304000019228504067F36C08603090200870302030088020490A916041481370F5125D0B1D408D4C3B232E6D25E795BEBFBAA16041481370F5125D0B1D408D4C3B232E6D25E795BEBFB990206C004030000010C0D47492D42412D55502D30343139AC48801F312E322E3834302E313233343536372F6D79506C6174666F726D4C6162656C812568747470733A2F2F6D79636F6D70616E792E636F6D2F6D79444C4F415265676973747261729000"}}
static int apdu_interface_transmit(struct euicc_ctx *ctx, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx,
                                   const uint32_t tx_len) {
    const struct stdio_userdata *userdata = ctx->http.interface->userdata;

    int ecode;

    if (json_request(userdata, "transmit", tx, (int)tx_len)) {
        return -1;
    }

    if (json_response(userdata, &ecode, rx, rx_len)) {
        return -1;
    }

    return ecode;
}

static int libapduinterface_init(struct euicc_apdu_interface *ifstruct) {
    const char *http_type = getenv_or_default(ENV_STDIO_APDU_TYPE, "hexify");

    struct stdio_userdata *userdata = malloc(sizeof(struct stdio_userdata));
    memset(userdata, 0, sizeof(struct stdio_userdata));

    if (strcmp(http_type, "hexify") == 0) {
        userdata->encode = hexify_encode;
        userdata->encode_len = hexify_encode_len;
        userdata->decode = hexify_decode;
        userdata->decode_len = hexify_decode_len;
    } else if (strcmp(http_type, "base64") == 0) {
        userdata->encode = euicc_base64_encode;
        userdata->encode_len = euicc_base64_encode_len;
        userdata->decode = euicc_base64_decode;
        userdata->decode_len = euicc_base64_decode_len;
    } else {
        fprintf(stderr, "Unknown APDU type: %s\n", http_type);
        free(userdata);
        return -1;
    }

    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = apdu_interface_disconnect;
    ifstruct->logic_channel_open = apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = apdu_interface_logic_channel_close;
    ifstruct->transmit = apdu_interface_transmit;
    ifstruct->userdata = userdata;

    return 0;
}

static void libapduinterface_fini(const struct euicc_apdu_interface *ifstruct) {
    struct stdio_userdata *userdata = ifstruct->userdata;
    if (userdata == NULL)
        return;
    free(userdata);
}

const struct euicc_driver driver_apdu_stdio = {
    .type = DRIVER_APDU,
    .name = "stdio",
    .init = (int (*)(void *))libapduinterface_init,
    .main = NULL,
    .fini = (void (*)(void *))libapduinterface_fini,
};
