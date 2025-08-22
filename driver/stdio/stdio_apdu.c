#include "stdio.h"
#include "stdio_helpers.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <cjson/cJSON_ex.h>
#include <euicc/interface.h>
#include <lpac/utils.h>

static bool json_request(const struct stdio_userdata *userdata, const char *func, const uint8_t *param,
                         const int param_len) {
    _cleanup_free_ char *param_hex = NULL;
    _cleanup_cjson_ cJSON *jpayload = NULL;

    if (param != NULL && param_len != 0) {
        param_hex = malloc(userdata->encode_len(param_len));
        if (param_hex == NULL)
            return false;
        if (userdata->encode(param_hex, param, param_len) < 0)
            return false;
    } else
        param_hex = NULL;

    jpayload = cJSON_CreateObject();
    if (jpayload == NULL)
        return false;
    if (cJSON_AddStringOrNullToObject(jpayload, "func", func) == NULL)
        return false;
    if (cJSON_AddStringOrNullToObject(jpayload, "param", param_hex) == NULL)
        return false;

    return json_print("apdu", jpayload);
}

static int json_response(const struct stdio_userdata *userdata, int *ecode, uint8_t **data, uint32_t *data_len) {
    int fret = 0;
    _cleanup_free_ char *data_json;
    cJSON *data_payload;

    if (data != NULL)
        *data = NULL;

    if (afgets(&data_json, stdin) < 0)
        return -1;

    const cJSON *data_jroot = cJSON_Parse(data_json);
    if (data_jroot == NULL)
        return -1;

    cJSON *jtmp = cJSON_GetObjectItem(data_jroot, "type");
    if (!(jtmp && cJSON_IsString(jtmp)))
        goto err;
    if (strcmp("apdu", cJSON_GetStringValue(jtmp)) != 0)
        goto err;

    data_payload = cJSON_GetObjectItem(data_jroot, "payload");
    if (!(data_payload && cJSON_IsObject(data_payload)))
        goto err;

    jtmp = cJSON_GetObjectItem(data_payload, "ecode");
    if (!(jtmp && cJSON_IsNumber(jtmp)))
        goto err;
    *ecode = (int)cJSON_GetNumberValue(jtmp);

    jtmp = cJSON_GetObjectItem(data_payload, "data");
    if (!(jtmp == NULL || !cJSON_IsString(jtmp) || data == NULL || data_len == NULL)) {
        *data_len = userdata->decode_len(cJSON_GetStringValue(jtmp));
        *data = malloc(*data_len);
        if (!*data)
            goto err;
        if (userdata->decode(*data, cJSON_GetStringValue(jtmp)) < 0)
            goto err;
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

    if (json_request(userdata, "connect", NULL, 0))
        return -1;

    int ecode;

    if (json_response(userdata, &ecode, NULL, NULL))
        return -1;

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
static int apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, const uint8_t aid_len) {
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

    if (json_request(userdata, "transmit", tx, (int)tx_len))
        return -1;

    if (json_response(userdata, &ecode, rx, rx_len))
        return -1;

    return ecode;
}

static int libapduinterface_init(struct euicc_apdu_interface *ifstruct) {
    memset(ifstruct, 0, sizeof(struct euicc_apdu_interface));

    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = apdu_interface_disconnect;
    ifstruct->logic_channel_open = apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = apdu_interface_logic_channel_close;
    ifstruct->transmit = apdu_interface_transmit;
    ifstruct->userdata = NULL;

    return stdio_setup_userdata((struct stdio_userdata **)&ifstruct->userdata,
                                getenv_or_default(ENV_STDIO_APDU_TYPE, "hexify"));
}

const struct euicc_driver driver_apdu_stdio = {
    .type = DRIVER_APDU,
    .name = "stdio",
    .init = (int (*)(void *))libapduinterface_init,
    .main = NULL,
    .fini = NULL,
};
