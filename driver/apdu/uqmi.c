// SPDX-License-Identifier: MIT
/* Copyright (c) 2024 David Bauer <david.bauer@uniberg.com> */
#include "uqmi.h"

#include <cjson/cJSON_ex.h>
#include <euicc/hexutil.h>
#include <euicc/interface.h>
#include <lpac/utils.h>

#include <inttypes.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ENV_UQMI_PROGRAM APDU_ENV_NAME(UQMI, PROGRAM)
#define ENV_UQMI_DEBUG APDU_ENV_NAME(UQMI, DEBUG)
#define ENV_UQMI_DEVICE APDU_ENV_NAME(UQMI, DEVICE)
#define ENV_UQMI_SLOT APDU_ENV_NAME(UQMI, SLOT)

struct uqmi_userdata {
    char *program;
    char *client_id;
    char *uim_slot;
    char *device_path;
};

static int merge_argv(char *required_argv[], char *user_argv[], char **merged_argv[]) {
    int required_argc = 0, user_argc = 0;

    while (required_argv[required_argc] != NULL)
        required_argc++;
    while (user_argv[user_argc] != NULL)
        user_argc++;

    *merged_argv = calloc(required_argc + user_argc + 1, sizeof(char *));
    if (*merged_argv == NULL)
        return -1;

    for (int i = 0; i < required_argc; i++)
        (*merged_argv)[i] = required_argv[i];
    for (int i = 0; i < user_argc; i++)
        (*merged_argv)[required_argc + i] = user_argv[i];
    return 0;
}

static int uqmi_execute_command(const struct uqmi_userdata *userdata, char **buf, char *argv[]) {
    if (userdata == NULL || userdata->device_path == NULL)
        return -1;

    _cleanup_free_ char **merged_argv;
    merge_argv((char *[]){userdata->program, "--single", "--device", userdata->device_path, NULL},
               argv,          // user provided arguments
               &merged_argv); // merged arguments

    if (getenv_or_default(ENV_UQMI_DEBUG, (bool)false)) {
        fprintf(stderr, "UQMI_DEBUG_TX:");
        for (int i = 0; merged_argv[i] != NULL; ++i)
            fprintf(stderr, " %s", merged_argv[i]);
        fprintf(stderr, "\n");
    }

    pid_t pid;
    posix_spawn_file_actions_t file_actions;
    int fret = 0;
    int status;

    int pipefd[2];
    pipe(pipefd);

    posix_spawn_file_actions_init(&file_actions);
    posix_spawn_file_actions_adddup2(&file_actions, pipefd[1], STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&file_actions, pipefd[0]);

    if (posix_spawnp(&pid, userdata->program, &file_actions, NULL, merged_argv, NULL) != 0)
        goto err;
    if (waitpid(pid, &status, 0) == -1)
        goto err;

    if (WIFEXITED(status) && buf != NULL) {
        char buffer[1024];
        ssize_t bytes_read = 0;
        ssize_t bytes_written = 0;
        *buf = malloc(bytes_read);
        while (true) {
            bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1);
            if (bytes_read == -1)
                break;
            *buf = realloc(*buf, bytes_written + bytes_read);
            memcpy(*buf + bytes_written, buffer, bytes_read);
            bytes_written += bytes_read;
        }
        if (getenv_or_default(ENV_UQMI_DEBUG, (bool)false))
            fprintf(stderr, "UQMI_DEBUG_RX: %s\n", *buf);
    }

    goto exit;
err:
    fret = -1;
    posix_spawn_file_actions_destroy(&file_actions);
exit:
    return fret;
}

static int apdu_interface_connect(struct euicc_ctx *ctx) {
    struct uqmi_userdata *userdata = ctx->apdu.interface->userdata;

    char *client_id = NULL;

    // clang-format off
    char *argv[] = {
        "--get-client-id", "uim",
        NULL,
    };
    // clang-format on

    if (uqmi_execute_command(userdata, &client_id, argv) != 0)
        return -1;

    const int n = snprintf(NULL, 0, "uim,%s", userdata->client_id);
    userdata->client_id = calloc(n + 1, 1);
    snprintf(userdata->client_id, n + 1, "uim,%s", userdata->client_id);
    return 0;
}

static void apdu_interface_disconnect(struct euicc_ctx *ctx) {
    const struct uqmi_userdata *userdata = ctx->apdu.interface->userdata;

    // clang-format off
    char *argv[] = {
        "--set-client-id", userdata->client_id,
        "--release-client-id", "uim",
        NULL,
    };
    // clang-format on

    uqmi_execute_command(userdata, NULL, argv);
}

static int apdu_interface_transmit(struct euicc_ctx *ctx, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx,
                                   const uint32_t tx_len) {
    const struct uqmi_userdata *userdata = ctx->apdu.interface->userdata;

    *rx = NULL;
    *rx_len = 0;

    char channel_str[16];
    snprintf(channel_str, sizeof(channel_str), "%d", ctx->apdu._internal.logic_channel);

    char *tx_hex = calloc(tx_len * 2 + 1, 1);
    if (tx_hex == NULL)
        return -1;
    euicc_hexutil_bin2hex(tx_hex, tx_len * 2 + 1, tx, tx_len);

    char *buf = NULL;

    // clang-format off
    char *argv[] = {
        "--set-client-id", userdata->client_id,
        "--keep-client-id", "uim",
        "--uim-slot", userdata->uim_slot,
        "--uim-channel-id", channel_str,
        "--uim-apdu-send", tx_hex,
        NULL,
    };
    // clang-format on

    uqmi_execute_command(userdata, &buf, argv);

    _cleanup_cjson_ cJSON *jroot = cJSON_Parse(buf);
    if (jroot == NULL)
        return -1;

    _cleanup_cjson_ cJSON *jrx = cJSON_GetObjectItem(jroot, "response");
    if (jrx == NULL || !cJSON_IsString(jrx))
        return -1;

    const char *response = cJSON_GetStringValue(jrx);

    *rx_len = strlen(response) / 2;
    *rx = malloc(*rx_len);
    return euicc_hexutil_hex2bin_r(*rx, *rx_len, response, *rx_len * 2);
}

static int apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, const uint8_t aid_len) {
    const struct uqmi_userdata *userdata = ctx->apdu.interface->userdata;

    if (userdata->client_id == NULL)
        return -1;

    char *aid_hex = calloc(aid_len * 2 + 1, 1);
    euicc_hexutil_bin2hex(aid_hex, aid_len * 2 + 1, aid, aid_len);

    // clang-format off
    char *argv[] = {
        "--set-client-id", userdata->client_id,
        "--keep-client-id", "uim",
        "--uim-slot", userdata->uim_slot,
        "--uim-channel-open", aid_hex,
        NULL,
    };
    // clang-format on

    char *buf = NULL;
    uqmi_execute_command(userdata, &buf, argv);
    _cleanup_cjson_ cJSON *jroot = cJSON_Parse(buf);
    if (jroot == NULL)
        return -1;
    _cleanup_cjson_ cJSON *jchannelid = cJSON_GetObjectItem(jroot, "channel_id");
    if (!jchannelid || !cJSON_IsNumber(jchannelid))
        return -1;
    return cJSON_GetNumberValue(jchannelid);
}

static void apdu_interface_logic_channel_close(struct euicc_ctx *ctx, const uint8_t channel) {
    if (channel == 0)
        return;
    const struct uqmi_userdata *userdata = ctx->apdu.interface->userdata;

    const int n = snprintf(NULL, 0, "%d", channel);
    char *channel_str = calloc(n + 1, 1);
    snprintf(channel_str, n + 1, "%d", channel);

    // clang-format off
    char *argv[] = {
        "--set-client-id", userdata->client_id,
        "--keep-client-id", "uim",
        "--uim-slot", userdata->uim_slot,
        "--uim-channel-id", channel_str,
        "--uim-channel-close",
        NULL,
    };
    // clang-format on

    uqmi_execute_command(userdata, NULL, argv);
}

static int libapduinterface_init(struct euicc_apdu_interface *ifstruct) {
    memset(ifstruct, 0, sizeof(struct euicc_apdu_interface));

    struct uqmi_userdata *userdata = malloc(sizeof(struct uqmi_userdata));
    userdata->program = (char *)getenv_or_default(ENV_UQMI_PROGRAM, "uqmi");
    userdata->device_path = getenv(ENV_UQMI_DEVICE);
    userdata->uim_slot = (char *)getenv_or_default(ENV_UQMI_SLOT, "1");
    userdata->client_id = NULL;

    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = apdu_interface_disconnect;
    ifstruct->logic_channel_open = apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = apdu_interface_logic_channel_close;
    ifstruct->transmit = apdu_interface_transmit;
    ifstruct->userdata = userdata;

    return 0;
}

static void libapduinterface_fini(struct euicc_apdu_interface *ifstruct) {}

const struct euicc_driver driver_apdu_uqmi = {
    .type = DRIVER_APDU,
    .name = "uqmi",
    .init = (int (*)(void *))libapduinterface_init,
    .main = NULL,
    .fini = (void (*)(void *))libapduinterface_fini,
};
