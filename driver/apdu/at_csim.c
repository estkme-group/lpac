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

static int apdu_interface_connect(struct euicc_ctx *ctx) {
    struct at_userdata *userdata = ctx->apdu.interface->userdata;

    const char *device = getenv_or_default(ENV_AT_DEVICE, get_at_default_device(userdata));
    if (at_device_open(userdata, device) != 0) {
        fprintf(stderr, "Failed to open device: %s\n", device);
        return -1;
    }

    static const char *commands[] = {"AT+CSIM=?", // TS 127 007, 8.17 Generic SIM access +CSIM
                                     NULL};
    if (at_test_commands(userdata, commands) == false)
        return -1;
    return 0;
}

static int apdu_interface_transmit(struct euicc_ctx *ctx, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx,
                                   const uint32_t tx_len) {
    struct at_userdata *userdata = ctx->apdu.interface->userdata;

    *rx = NULL;
    *rx_len = 0;

    char tx_hex[tx_len * 2 + 1];
    euicc_hexutil_bin2hex(tx_hex, tx_len * 2 + 1, tx, tx_len);
    at_emit_command(userdata, "AT+CSIM=%u,\"%s\"", tx_len * 2, tx_hex);

    char *response = NULL;
    if (at_expect(userdata, &response, "+CSIM: ") != 0 || response == NULL)
        return -1;

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
        return -1;
    if (euicc_hexutil_hex2bin_r(*rx, *rx_len, response, strlen(response)) < 0)
        return -1;
    return 0;
}

static int apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, const uint8_t aid_len) {
    uint8_t *resp;
    uint32_t resp_len;

    // Send MANAGE CHANNEL open
    resp = NULL;
    resp_len = 0;

    const uint8_t manage_open[] = {0x00, 0x70, 0x00, 0x00, 0x01};
    if (apdu_interface_transmit(ctx, &resp, &resp_len, manage_open, sizeof(manage_open)) != 0)
        return -1;
    if (resp_len != 3 || resp[1] != 0x90 || resp[2] != 0x00)
        return -1;

    const uint8_t channel_byte = resp[0];

    // Select AID

    resp = NULL;
    resp_len = 0;

    uint8_t *select_cmd = malloc(5 + aid_len);
    if (select_cmd == NULL)
        return -1;
    memcpy(select_cmd, (uint8_t[]){channel_byte, 0xA4, 0x04, 0x00, aid_len}, 5);
    memcpy(select_cmd + 5, aid, aid_len);
    if (apdu_interface_transmit(ctx, &resp, &resp_len, select_cmd, 5 + aid_len) != 0)
        return -1;
    if (resp_len < 2 || (resp[resp_len - 2] != 0x90 && resp[resp_len - 2] != 0x61))
        return -1;

    return channel_byte;
}

static void apdu_interface_logic_channel_close(struct euicc_ctx *ctx, const uint8_t channel) {
    const uint8_t manage_close[] = {0x00, 0x70, 0x80, channel, 0x00};
    uint8_t *resp = NULL;
    uint32_t resp_len = 0;
    apdu_interface_transmit(ctx, &resp, &resp_len, manage_close, sizeof(manage_close));
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

const struct euicc_driver driver_apdu_at_csim = {
    .type = DRIVER_APDU,
    .name = "at_csim",
    .init = (int (*)(void *))libapduinterface_init,
    .main = (int (*)(void *, int, char **))at_interface_main_entry,
    .fini = (void (*)(void *))at_interface_finished,
};
