#include "at.h"
#include "at_cmd.h"
#include "at_common.h"

#include <cjson/cJSON_ex.h>
#include <euicc/hexutil.h>
#include <euicc/interface.h>
#include <lpac/utils.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *at_channel_get(struct at_userdata *userdata, int index) {
    index -= 1; // Convert to 0-based index
    if (index < 0 || index >= AT_MAX_LOGICAL_CHANNELS)
        return NULL;
    char **channels = at_channels(userdata);
    return strdup(channels[index]);
}

static bool at_channel_set(struct at_userdata *userdata, int index, const char *identifier) {
    index -= 1; // Convert to 0-based index
    if (index < 0 || index >= AT_MAX_LOGICAL_CHANNELS)
        return false;
    char **channels = at_channels(userdata);
    channels[index] = strdup(identifier);
    return true;
}

static int at_channel_next_id(struct at_userdata *userdata) {
    int index = 0;
    char **channels = at_channels(userdata);
    while (channels[index] != NULL)
        index++;
    return index + 1; // Convert to 1-based index
}

static int apdu_interface_connect(struct euicc_ctx *ctx) {
    struct at_userdata *userdata = ctx->apdu.interface->userdata;

    const char *device = getenv_or_default(ENV_AT_DEVICE, get_at_default_device(userdata));
    if (at_device_open(userdata, device) != 0) {
        fprintf(stderr, "Failed to open device: %s\n", device);
        return -1;
    }

    static const char *commands[] = {"AT+CCHO=?", // TS 127 007, 8.45 Open logical channel +CCHO
                                     "AT+CCHC=?", // TS 127 007, 8.46 Close logical channel +CCHC
                                     "AT+CGLA=?", // TS 127 007, 8.43 Generic UICC logical channel access +CGLA
                                     NULL};
    if (at_test_commands(userdata, commands) == false)
        return -1;
    return 0;
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

    // AT+CGLA=<channel>,<length>,<command>
    at_emit_command(userdata, "AT+CGLA=%s,%u,\"%s\"", logic_channel, tx_len * 2, encoded);
    // +CGLA: <length>,<response>
    if (at_expect(userdata, &response, "+CGLA: ") != 0 || response == NULL)
        goto err;
    // Parse the response
    {
        strtok(response, ",");        // Skip the length part
        response = strtok(NULL, ","); // Get the response part
        // unquote strings
        const size_t n = strlen(response);
        if (response[0] == '"' && response[n - 1] == '"') {
            memmove(response, response + 1, n - 2);
            response[n - 2] = '\0'; // Null-terminate the new string
        }
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
    return fret;
}

static int apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, const uint8_t aid_len) {
    struct at_userdata *userdata = ctx->apdu.interface->userdata;

    _cleanup_free_ char *aid_hex = malloc(aid_len * 2 + 1);
    euicc_hexutil_bin2hex(aid_hex, aid_len * 2 + 1, aid, aid_len);

    // AT+CCHO: Open logical channel
    at_emit_command(userdata, "AT+CCHO=\"%s\"", aid_hex);
    // +CCHO: <channel>
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

    at_emit_command(userdata, "AT+CCHC=%d", channel);
    if (at_expect(userdata, NULL, NULL) != 0) {
        fprintf(stderr, "Failed to close logical channel %d\n", channel);
        return;
    }

    at_channel_set(userdata, channel, NULL);
}

static int libapduinterface_init(struct euicc_apdu_interface *ifstruct) {
    set_deprecated_env_name(ENV_AT_DEBUG, "AT_DEBUG");
    set_deprecated_env_name(ENV_AT_DEVICE, "AT_DEVICE");

    memset(ifstruct, 0, sizeof(struct euicc_apdu_interface));

    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = at_interface_disconnect;
    ifstruct->logic_channel_open = apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = apdu_interface_logic_channel_close;
    ifstruct->transmit = apdu_interface_transmit;
    ifstruct->userdata = NULL;

    return at_setup_userdata((struct at_userdata **)&ifstruct->userdata);
}

const struct euicc_driver driver_apdu_at = {
    .type = DRIVER_APDU,
    .name = "at",
    .init = (int (*)(void *))libapduinterface_init,
    .main = (int (*)(void *, int, char **))at_interface_main_entry,
    .fini = (void (*)(void *))at_interface_finished,
};
