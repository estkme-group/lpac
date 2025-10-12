#include "at_cmd.h"

#include <cjson-ext/cJSON_ex.h>
#include <driver.h>
#include <euicc/hexutil.h>
#include <euicc/interface.h>
#include <lpac/utils.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "at_helpers.h"

static int apdu_interface_connect(struct euicc_ctx *ctx) {
    struct at_userdata *userdata = ctx->apdu.interface->userdata;

    const char *device = getenv_or_default(ENV_AT_DEVICE, get_at_default_device(userdata));
    if (at_device_open(userdata, device) != 0) {
        fprintf(stderr, "Failed to open device: %s\n", device);
        return -1;
    }

    at_emit_command(userdata, "AT");
    if (at_expect(userdata, NULL, NULL) != 0) {
        fprintf(stderr, "Device not responding to AT commands\n");
        return false;
    }

    static const char *commands[] = {"AT+CCHO", "AT+CCHC", "AT+CGLA", NULL};
    for (int index = 0; commands[index] != NULL; index++) {
        at_emit_command(userdata, "%s=?", commands[index]);
        if (at_expect(userdata, NULL, NULL) == 0)
            continue;
        fprintf(stderr, "Device missing %s support\n", commands[index]);
        return false;
    }

    return true;
}

static void apdu_interface_disconnect(struct euicc_ctx *ctx) {
    struct at_userdata *userdata = ctx->apdu.interface->userdata;
    at_channel_set(userdata, ctx->apdu._internal.logic_channel, NULL);
    at_device_close(userdata);
}

static int apdu_interface_transmit(struct euicc_ctx *ctx, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx,
                                   const uint32_t tx_len) {
    struct at_userdata *userdata = ctx->apdu.interface->userdata;
    const char *logic_channel = at_channel_get(userdata, ctx->apdu._internal.logic_channel);

    int fret = 0;
    char *response = NULL;

    int ret;
    char *encoded = NULL;

    *rx = NULL;
    *rx_len = 0;

    encoded = malloc(tx_len * 2 + 1);
    euicc_hexutil_bin2hex(encoded, tx_len * 2 + 1, tx, tx_len);

    at_emit_command(userdata, "AT+CGLA=%s,%u,\"%s\"", logic_channel, tx_len * 2, encoded);
    if (at_expect(userdata, &response, "+CGLA: ") != 0 || response == NULL)
        goto err;

    strtok(response, ",");        // Skip length
    response = strtok(NULL, ","); // Get response
    size_t n = strlen(response);
    if (response[0] == '"' && response[n - 1] == '"') {
        memmove(response, response + 1, n - 2);
        response[n - 2] = '\0';
    }

    *rx_len = strlen(response) / 2;
    *rx = malloc(*rx_len);
    if (*rx == NULL)
        goto err;
    ret = euicc_hexutil_hex2bin_r(*rx, *rx_len, response, strlen(response));
    if (ret < 0)
        goto err;
    *rx_len = ret;

    goto exit;

err:
    fret = -1;
    free(*rx);
    *rx = NULL;
    *rx_len = 0;
exit:
    free(encoded);
    return fret;
}

static int apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, const uint8_t aid_len) {
    struct at_userdata *userdata = ctx->apdu.interface->userdata;
    char **channels = at_channels(userdata);

    _cleanup_free_ char *aid_hex = malloc(aid_len * 2 + 1);
    euicc_hexutil_bin2hex(aid_hex, aid_len * 2 + 1, aid, aid_len);

    for (int index = 0; index < AT_MAX_LOGICAL_CHANNELS; index++) {
        if (channels[index] == NULL)
            continue;
        at_emit_command(userdata, "AT+CCHC=%s", channels[index]);
        at_expect(userdata, NULL, NULL);
    }

    at_emit_command(userdata, "AT+CCHO=\"%s\"", aid_hex);
    _cleanup_free_ char *response = NULL;
    if (at_expect(userdata, &response, "+CCHO: ") != 0 || response == NULL)
        return -1;

    const int channel_id = at_channel_next_id(userdata);
    if (at_channel_set(userdata, channel_id, response) != 0) {
        fprintf(stderr, "Failed to register channel %d with identifier '%s'\n", channel_id, response);
        return -1;
    }
    return channel_id;
}

static void apdu_interface_logic_channel_close(struct euicc_ctx *ctx, const uint8_t channel) {
    struct at_userdata *userdata = ctx->apdu.interface->userdata;
    char *identifier = at_channel_get(userdata, channel);

    if (identifier == NULL)
        return;

    at_emit_command(userdata, "AT+CCHC=%s", identifier);
    at_expect(userdata, NULL, NULL);

    at_channel_set(userdata, channel, NULL);
}

static int libapduinterface_init(struct euicc_apdu_interface *ifstruct) {
    fprintf(stderr, AT_WARNING_MESSAGE);

    set_deprecated_env_name(ENV_AT_DEBUG, "AT_DEBUG");
    set_deprecated_env_name(ENV_AT_DEVICE, "AT_DEVICE");

    memset(ifstruct, 0, sizeof(struct euicc_apdu_interface));

    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = apdu_interface_disconnect;
    ifstruct->logic_channel_open = apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = apdu_interface_logic_channel_close;
    ifstruct->transmit = apdu_interface_transmit;
    ifstruct->userdata = NULL;

    return at_setup_userdata((struct at_userdata **)&ifstruct->userdata);
}

static int libapduinterface_main(const struct euicc_apdu_interface *ifstruct, const int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <list>\n", argv[0]);
        return -1;
    }

    if (strcmp(argv[1], "list") == 0) {
        cJSON *devices = cJSON_CreateArray();
        enumerate_serial_device(devices);
        cJSON *payload = cJSON_CreateObject();
        cJSON_AddStringOrNullToObject(payload, "env", ENV_AT_DEVICE);
        cJSON_AddItemToObject(payload, "data", devices);
        json_print("driver", payload);
    }

    return 0;
}

static void libapduinterface_fini(struct euicc_apdu_interface *ifstruct) {
    struct at_userdata *userdata = ifstruct->userdata;
    at_cleanup_userdata(&userdata);
}

DRIVER_INTERFACE = {
    .type = DRIVER_APDU,
    .name = "at",
    .init = (int (*)(void *))libapduinterface_init,
    .main = (int (*)(void *, int, char **))libapduinterface_main,
    .fini = (void (*)(void *))libapduinterface_fini,
};
