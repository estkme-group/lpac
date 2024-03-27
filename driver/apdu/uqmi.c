// SPDX-License-Identifier: MIT
/* Copyright (c) 2024 David Bauer <david.bauer@uniberg.com> */

#include "at.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <cjson/cJSON_ex.h>
#include <euicc/interface.h>
#include <euicc/hexutil.h>

static FILE *fuart;
static int client_id = 0;
static int logic_channel = 0;
static char *devpath = NULL;


static int uqmi_execute_command(const char *command, char *buf, size_t bufsize)
{
    char *debug_str = getenv("LPAC_QMI_DEBUG");
    char final_command[2048] = {};
    FILE *fp;
    char *remaining;
    int debug = debug_str ? atoi(debug_str) : 0;

    if (debug)
        printf("UQMI_DEBUG_TX: %s\n", command);

    if (snprintf(final_command, sizeof(final_command), "uqmi -s -d %s %s", devpath, command) >= sizeof(final_command))
    {
        fprintf(stderr, "Command too long\n");
        return -1;
    }

    fp = popen(final_command, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Failed to execute command: %s\n", command);
        return -1;
    }

    remaining = buf;
    if (buf) {
        while (fgets(remaining, bufsize - (remaining - buf), fp) != NULL)
        {
            /* Read to buffer */
        }
        if (debug)
            printf("UQMI_DEBUG_RX: %s", buf);
    }

    pclose(fp);
    return 0;
}

static int uqmi_open_client()
{
    char buffer[2048] = {};
    int ret;

    ret = uqmi_execute_command("--get-client-id uim", buffer, sizeof(buffer));
    if (ret)
    {
        return -1;
    }

    client_id = atoi(buffer);
    if (client_id == 0)
    {
        return -1;
    }

    return 0;
}

static int apdu_interface_connect(struct euicc_ctx *ctx)
{
    const char *device;

    client_id = 0;
    logic_channel = 0;
    devpath = getenv("LPAC_QMI_DEV");

    return uqmi_open_client();
}

static void apdu_interface_disconnect(struct euicc_ctx *ctx)
{
    char command[64] = {};

    snprintf(command, sizeof(command), "--set-client-id uim,%d --release-client-id uim", client_id);
    uqmi_execute_command(command, NULL, 0);

    client_id = 0;
    logic_channel = 0;
}

static int apdu_interface_transmit(struct euicc_ctx *ctx, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx, uint32_t tx_len)
{
    cJSON *root = NULL, *jtmp = NULL;
    char cmd[2048] = {};
    char buf[2048] = {};
    char *json;

    *rx = NULL;
    *rx_len = 0;

    if (!client_id)
    {
        return -1;
    }

    if (!logic_channel)
    {
        return -1;
    }

    for (int i = 0; i < tx_len; i++)
    {
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%02X", (uint8_t)(tx[i] & 0xFF));
    }

    snprintf(cmd, sizeof(cmd), "--set-client-id uim,%d --keep-client-id uim --uim-slot 1 --uim-channel-id %d --uim-apdu-send \"%s\"",
             client_id, logic_channel, buf);
    uqmi_execute_command(cmd, buf, sizeof(buf));

    json = strstr(buf, "{");
    if (!json)
    {
        printf("No JSON found\n");
        goto fail;
    }

    root = cJSON_Parse(json);
    if (!root)
    {
        printf("Failed to parse JSON\n");
        goto fail;
    }

    jtmp = cJSON_GetObjectItem(root, "response");
    if (!jtmp || !cJSON_IsString(jtmp))
    {
        printf("Failed to get response\n");
        goto fail;
    }

    *rx_len = strlen(jtmp->valuestring) / 2;
    *rx = malloc(*rx_len);
    if (euicc_hexutil_hex2bin_r(*rx, *rx_len, jtmp->valuestring, strlen(jtmp->valuestring)) < 0)
    {
        printf("Failed to convert hex '%s' to binary\n", jtmp->valuestring);
        goto fail;
    }

    cJSON_Delete(root);
    return 0;
fail:
    if (root)
    {
        cJSON_Delete(root);
    }

    if (*rx)
    {
        free(*rx);
    }
    *rx_len = 0;
    return -1;
}

static int apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, uint8_t aid_len)
{
    char *response, *json;
    char buf[2048] = {};
    char cmd[2048] = {};
    cJSON *root, *jtmp;

    if (!client_id)
    {
        return -1;
    }

    if (logic_channel)
    {
        return logic_channel;
    }

    snprintf(cmd, sizeof(cmd), "--set-client-id uim,%d --keep-client-id uim --uim-slot 1 --uim-channel-open ", client_id);
    for (int i = 0; i < aid_len; i++)
    {
        snprintf(cmd + strlen(cmd), sizeof(cmd) - strlen(cmd), "%02X", (uint8_t)(aid[i] & 0xFF));
    }

    uqmi_execute_command(cmd, buf, sizeof(buf));

    json = strstr(buf, "{");
    if (!json)
    {
        printf("No JSON found\n");
        return -1;
    }

    root = cJSON_Parse(json);
    if (!root)
    {
        printf("Failed to parse JSON\n");
        return -1;
    }

    jtmp = cJSON_GetObjectItem(root, "channel_id");
    if (!jtmp || !cJSON_IsNumber(jtmp))
    {
        printf("Failed to get channel_id\n");
        cJSON_Delete(root);
        return -1;
    }

    logic_channel = jtmp->valueint;

    cJSON_Delete(root);
    return logic_channel;
}

static void apdu_interface_logic_channel_close(struct euicc_ctx *ctx, uint8_t channel)
{
    char cmd[2048] = {};

    if (!logic_channel)
    {
        return;
    }

    snprintf(cmd, sizeof(cmd), "--set-client-id uim,%d --keep-client-id uim --uim-slot 1 --uim-channel-id %d --uim-channel-close", client_id, logic_channel);
    uqmi_execute_command(cmd, NULL, 0);
    logic_channel = 0;
    return;
}

static int libapduinterface_init(struct euicc_apdu_interface *ifstruct)
{
    memset(ifstruct, 0, sizeof(struct euicc_apdu_interface));

    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = apdu_interface_disconnect;
    ifstruct->logic_channel_open = apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = apdu_interface_logic_channel_close;
    ifstruct->transmit = apdu_interface_transmit;

    return 0;
}

static int libapduinterface_main(int argc, char **argv)
{
    return 0;
}

static void libapduinterface_fini(void)
{
}

const struct euicc_driver driver_apdu_uqmi = {
    .type = DRIVER_APDU,
    .name = "uqmi",
    .init = (int (*)(void *))libapduinterface_init,
    .main = libapduinterface_main,
    .fini = libapduinterface_fini,
};
