#include "stdio.h"
#include "stdio_helpers.h"

#include <cjson/cJSON_ex.h>
#include <euicc/interface.h>
#include <lpac/utils.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool json_request(const struct stdio_userdata *userdata, const char *url, const uint8_t *tx,
                         const uint32_t tx_len, const char **headers) {
    _cleanup_free_ char *tx_encoded = NULL;
    cJSON *jpayload = NULL;
    cJSON *jheaders = NULL;

    tx_encoded = malloc(userdata->encode_len((int)tx_len));
    if (tx_encoded == NULL)
        return false;
    if (userdata->encode(tx_encoded, tx, (int)tx_len) < 0)
        return false;

    jpayload = cJSON_CreateObject();
    if (jpayload == NULL)
        return false;
    if (cJSON_AddStringOrNullToObject(jpayload, "url", url) == NULL)
        return false;
    if (cJSON_AddStringOrNullToObject(jpayload, "tx", tx_encoded) == NULL)
        return false;

    jheaders = cJSON_AddArrayToObject(jpayload, "headers");
    if (jheaders == NULL)
        return false;

    for (int i = 0; headers[i] != NULL; i++) {
        cJSON *jh = cJSON_CreateString(headers[i]);
        if (jh == NULL)
            return false;
        cJSON_AddItemToArray(jheaders, jh);
    }

    return json_print("http", jpayload);
}

static int json_response(const struct stdio_userdata *userdata, const char *rx_json, uint32_t *rcode, uint8_t **rx,
                         uint32_t *rx_len) {
    _cleanup_cjson_ cJSON *rx_jroot = cJSON_Parse(rx_json);
    if (rx_jroot == NULL)
        return -1;
    const cJSON *jtmp = cJSON_GetObjectItem(rx_jroot, "type");
    if (!(jtmp && cJSON_IsString(jtmp)))
        return -1;
    if (strcmp("http", cJSON_GetStringValue(jtmp)) != 0)
        return -1;
    const cJSON *rx_payload = cJSON_GetObjectItem(rx_jroot, "payload");
    if (!(rx_payload && cJSON_IsObject(rx_payload)))
        return -1;
    jtmp = cJSON_GetObjectItem(rx_payload, "rcode"); // payload.rcodes
    if (!(jtmp && cJSON_IsNumber(jtmp)))
        return -1;
    *rcode = (int)cJSON_GetNumberValue(jtmp);

    jtmp = cJSON_GetObjectItem(rx_payload, "rx"); // payload.rx
    if (!(jtmp && cJSON_IsString(jtmp)))
        return -1;

    *rx_len = userdata->decode_len(cJSON_GetStringValue(jtmp));
    *rx = malloc(*rx_len);
    if (*rx == NULL)
        return -1;
    if (userdata->decode(*rx, cJSON_GetStringValue(jtmp)) < 0)
        return -1;
    return 0;
}

// {"type":"http","payload":{"rcode":404,"rx":"333435"}}
static int http_interface_transmit(struct euicc_ctx *ctx, const char *url, uint32_t *rcode, uint8_t **rx,
                                   uint32_t *rx_len, const uint8_t *tx, const uint32_t tx_len, const char **headers) {
    const struct stdio_userdata *userdata = ctx->http.interface->userdata;

    *rx = NULL;
    *rx_len = 0;

    json_request(userdata, url, tx, tx_len, headers);

    _cleanup_free_ char *rx_json;
    if (afgets(&rx_json, stdin) < 0)
        return -1;
    if (json_response(userdata, rx_json, rcode, rx, rx_len) >= 0)
        return 0;
    *rcode = 500;
    *rx_len = 0;
    if (*rx != NULL)
        free(*rx);
    return -1;
}

static int libhttpinterface_init(struct euicc_http_interface *ifstruct) {
    memset(ifstruct, 0, sizeof(struct euicc_http_interface));

    ifstruct->transmit = http_interface_transmit;

    return stdio_setup_userdata((struct stdio_userdata **)&ifstruct->userdata,
                                getenv_or_default(ENV_STDIO_HTTP_TYPE, "hexify"));
}

static void libhttpinterface_fini(struct euicc_http_interface *ifstruct) {}

const struct euicc_driver driver_http_stdio = {
    .type = DRIVER_HTTP,
    .name = "stdio",
    .init = (int (*)(void *))libhttpinterface_init,
    .main = NULL,
    .fini = (void (*)(void *))libhttpinterface_fini,
};
