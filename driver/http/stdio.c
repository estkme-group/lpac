#include <cjson-ext/cJSON_ex.h>
#include <driver.h>
#include <euicc/hexutil.h>
#include <euicc/interface.h>
#include <lpac/utils.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

static bool json_request(const char *url, const uint8_t *tx, uint32_t tx_len, const char **headers) {
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
static int http_interface_transmit(struct euicc_ctx *ctx, const char *url, uint32_t *rcode, uint8_t **rx,
                                   uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len, const char **headers) {
    int fret = 0;
    _cleanup_free_ char *rx_json;
    _cleanup_cjson_ cJSON *rx_jroot;
    cJSON *rx_payload;
    cJSON *jtmp;

    *rx = NULL;

    json_request(url, tx, tx_len, headers);
    if (afgets(&rx_json, stdin) < 0) {
        return -1;
    }

    rx_jroot = cJSON_Parse(rx_json);
    if (rx_jroot == NULL) {
        return -1;
    }

    jtmp = cJSON_GetObjectItem(rx_jroot, "type");
    if (!jtmp) {
        goto err;
    }
    if (!cJSON_IsString(jtmp)) {
        goto err;
    }
    if (strcmp("http", jtmp->valuestring) != 0) {
        goto err;
    }

    rx_payload = cJSON_GetObjectItem(rx_jroot, "payload");
    if (!rx_payload) {
        goto err;
    }
    if (!cJSON_IsObject(rx_payload)) {
        goto err;
    }

    jtmp = cJSON_GetObjectItem(rx_payload, "rcode");
    if (!jtmp) {
        goto err;
    }
    if (!cJSON_IsNumber(jtmp)) {
        goto err;
    }
    *rcode = jtmp->valueint;

    jtmp = cJSON_GetObjectItem(rx_payload, "rx");
    if (!jtmp) {
        goto err;
    }
    if (!cJSON_IsString(jtmp)) {
        goto err;
    }
    *rx_len = strlen(jtmp->valuestring) / 2;
    *rx = malloc(*rx_len);
    if (!*rx) {
        goto err;
    }
    if (euicc_hexutil_hex2bin_r(*rx, *rx_len, jtmp->valuestring, strlen(jtmp->valuestring)) < 0) {
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
    return fret;
}

static int libhttpinterface_init(struct euicc_http_interface *ifstruct) {
    memset(ifstruct, 0, sizeof(struct euicc_http_interface));

    ifstruct->transmit = http_interface_transmit;

    return 0;
}

DRIVER_INTERFACE = {
    .type = DRIVER_HTTP,
    .name = "stdio",
    .init = (int (*)(void *))libhttpinterface_init,
    .main = NULL,
    .fini = NULL,
};
