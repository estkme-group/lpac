#include "at.h"
#include "at_cmd.h"

#include <cjson/cJSON_ex.h>
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

    at_emit_command(userdata, "AT+CSIM=?");
    if (at_expect(userdata, NULL, NULL) != 0) {
        fprintf(stderr, "Device missing +CSIM support\n");
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

    char *encoded = NULL;

    *rx = NULL;
    *rx_len = 0;

    encoded = malloc(tx_len * 2 + 1);
    euicc_hexutil_bin2hex(encoded, tx_len * 2 + 1, tx, tx_len);

    at_emit_command(userdata, "AT+CSIM=%u,\"%s\"", tx_len * 2, encoded);
    if (at_expect(userdata, &response, "+CSIM: ") != 0 || response == NULL)
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
    int ret = euicc_hexutil_hex2bin_r(*rx, *rx_len, response, strlen(response));
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

    // Send MANAGE CHANNEL open
    const uint8_t manage_open[] = {0x00, 0x70, 0x00, 0x00, 0x01};
    uint8_t *resp = NULL;
    uint32_t resp_len = 0;
    if (apdu_interface_transmit(ctx, &resp, &resp_len, manage_open, sizeof(manage_open)) != 0) {
        return -1;
    }
    if (resp_len != 3 || resp[1] != 0x90 || resp[2] != 0x00) {
        free(resp);
        return -1;
    }
    uint8_t channel_byte = resp[0];
    free(resp);

    // Select AID
    uint8_t *select_cmd = malloc(5 + aid_len);
    if (select_cmd == NULL) {
        return -1;
    }
    select_cmd[0] = channel_byte;
    select_cmd[1] = 0xA4;
    select_cmd[2] = 0x04;
    select_cmd[3] = 0x00;
    select_cmd[4] = aid_len;
    memcpy(select_cmd + 5, aid, aid_len);

    resp = NULL;
    resp_len = 0;
    if (apdu_interface_transmit(ctx, &resp, &resp_len, select_cmd, 5 + aid_len) != 0) {
        free(select_cmd);
        return -1;
    }
    free(select_cmd);

    if (resp_len < 2 || (resp[resp_len - 2] != 0x90 && resp[resp_len - 2] != 0x61)) {
        free(resp);
        return -1;
    }
    free(resp);

    // Register channel
    char channel_str[4];
    snprintf(channel_str, sizeof(channel_str), "%u", channel_byte);
    const int channel_id = at_channel_next_id(userdata);
    if (at_channel_set(userdata, channel_id, channel_str) != 0) {
        fprintf(stderr, "Failed to register channel %d with identifier '%s'\n", channel_id, channel_str);
        return -1;
    }
    return channel_id;
}

static void apdu_interface_logic_channel_close(struct euicc_ctx *ctx, const uint8_t channel) {
    struct at_userdata *userdata = ctx->apdu.interface->userdata;
    char *identifier = at_channel_get(userdata, channel);

    if (identifier == NULL) {
        return;
    }

    uint8_t channel_byte = (uint8_t)atoi(identifier);
    const uint8_t manage_close[] = {0x00, 0x70, 0x80, channel_byte, 0x00};
    uint8_t *resp = NULL;
    uint32_t resp_len = 0;
    apdu_interface_transmit(ctx, &resp, &resp_len, manage_close, sizeof(manage_close));
    free(resp);

    at_channel_set(userdata, channel, NULL);
}

static int libapduinterface_init(struct euicc_apdu_interface *ifstruct) {
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
        if (enumerate_serial_device != NULL) {
            enumerate_serial_device(devices);
        } else {
            fprintf(stderr, "Serial device enumeration not implemented on this platform.\n");
            fflush(stderr);
        }
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

const struct euicc_driver driver_apdu_at_csim = {
    .type = DRIVER_APDU,
    .name = "at_csim",
    .init = (int (*)(void *))libapduinterface_init,
    .main = (int (*)(void *, int, char **))libapduinterface_main,
    .fini = (void (*)(void *))libapduinterface_fini,
};
