#include "helpers.h"
#include "stdio.h"

#include <cjson-ext/cJSON_ex.h>
#include <euicc/hexutil.h>
#include <euicc/interface.h>
#include <lpac/utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool json_request(const char *url, const uint8_t *tx, const uint32_t tx_len, const char **headers) {
    _cleanup_free_ char *tx_hex = NULL;
    _cleanup_cjson_ cJSON *jpayload = NULL;
    cJSON *jheaders = NULL;

    tx_hex = malloc((2 * tx_len) + 1);
    if (tx_hex == NULL) {
        return false;
    }
    if (euicc_hexutil_bin2hex(tx_hex, (2 * tx_len) + 1, tx, tx_len) < 0) {
        return false;
    }

    jpayload = cJSON_CreateObject();
    if (jpayload == NULL) {
        return false;
    }
    if (cJSON_AddStringOrNullToObject(jpayload, "url", url) == NULL) {
        return false;
    }
    if (cJSON_AddStringOrNullToObject(jpayload, "tx", tx_hex) == NULL) {
        return false;
    }

    jheaders = cJSON_AddArrayToObject(jpayload, "headers");
    if (jheaders == NULL) {
        return false;
    }

    for (int i = 0; headers[i] != NULL; i++) {
        cJSON *jh = cJSON_CreateString(headers[i]);
        if (jh == NULL) {
            return false;
        }
        cJSON_AddItemToArray(jheaders, jh);
    }

    return json_print("http", jpayload);
}

// {"type":"http","payload":{"rcode":404,"rx":"333435"}}
static int http_interface_transmit(__attribute__((unused)) struct euicc_ctx *ctx, const char *url, uint32_t *rcode,
                                   uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, const uint32_t tx_len,
                                   const char **headers) {
    *rx = NULL;

    _cleanup_cjson_ cJSON *rx_payload = NULL;

    json_request(url, tx, tx_len, headers);
    if (receive_payload(stdin, "http", &rx_payload) < 0) {
        goto err;
    }

    const cJSON *rx_rcode = cJSON_GetObjectItem(rx_payload, "rcode");
    const cJSON *rx_data = cJSON_GetObjectItem(rx_payload, "rx");
    if (!cJSON_IsNumber(rx_rcode) || !cJSON_IsString(rx_data)) {
        goto err;
    }
    *rcode = (int)cJSON_GetNumberValue(rx_rcode);

    const char *input_data = cJSON_GetStringValue(rx_data);
    const size_t input_data_len = strlen(input_data);

    *rx_len = input_data_len / 2;
    *rx = malloc(*rx_len);

    if (*rx == NULL) {
        goto err;
    }
    if (euicc_hexutil_hex2bin_r(*rx, *rx_len, input_data, input_data_len) < 0) {
        goto err;
    }

    return 0;
err:
    free(*rx);
    *rx = NULL;
    *rx_len = 0;
    *rcode = 500;
    return -1;
}

static int libhttpinterface_init(struct euicc_http_interface *ifstruct) {
    memset(ifstruct, 0, sizeof(struct euicc_http_interface));

    ifstruct->transmit = http_interface_transmit;

    return 0;
}

const struct euicc_driver driver_http_stdio = {
    .type = DRIVER_HTTP,
    .name = "stdio",
    .init = (int (*)(void *))libhttpinterface_init,
    .main = NULL,
    .fini = NULL,
};
