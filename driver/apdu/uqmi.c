// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 David Bauer <david.bauer@uniberg.com>
#include <driver.h>
#include <euicc/hexutil.h>
#include <euicc/interface.h>
#include <lpac/utils.h>

#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define ENV_UQMI_PROGRAM APDU_ENV_NAME(UQMI, PROGRAM)
#define ENV_UQMI_DEBUG APDU_ENV_NAME(UQMI, DEBUG)
#define ENV_QMI_DEVICE APDU_ENV_NAME(QMI, DEVICE)
#define ENV_QMI_UIM_SLOT APDU_ENV_NAME(QMI, UIM_SLOT)

struct uqmi_userdata {
    char *program;
    char *client_id;
    char *uim_slot;
    char *device_path;
};

static int uqmi_execute_command(const struct uqmi_userdata *userdata, char **buf, char *argv[]) {
    if (userdata == NULL || userdata->device_path == NULL)
        return -1;

    _cleanup_free_ char **merged_argv = merge_array_of_str(
        // requires arguments
        (char *[]){userdata->program, "--single", "--device", userdata->device_path, NULL},
        // user provided arguments
        argv);

    if (getenv_or_default(ENV_UQMI_DEBUG, (bool)false)) {
        fprintf(stderr, "UQMI_DEBUG_TX:");
        for (int i = 0; merged_argv[i] != NULL; ++i)
            fprintf(stderr, " %s", merged_argv[i]);
        fprintf(stderr, "\n");
    }

    _cleanup_(posix_spawn_file_actions_destroy) posix_spawn_file_actions_t file_actions;
    int pipefd[2];
    if (pipe(pipefd) != 0)
        return -1;

    posix_spawn_file_actions_init(&file_actions);
    posix_spawn_file_actions_adddup2(&file_actions, pipefd[1], STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&file_actions, pipefd[1]);
    posix_spawn_file_actions_addclose(&file_actions, pipefd[0]);

#ifdef HAVE_PIDFD
    int pidfd;
    if (pidfd_spawnp(&pidfd, userdata->program, &file_actions, NULL, merged_argv, NULL) != 0)
        return -1;
#else
    pid_t pid;
    if (posix_spawnp(&pid, userdata->program, &file_actions, NULL, merged_argv, NULL) != 0)
        return -1;
#endif

    close(pipefd[1]);

    if (buf != NULL) {
        *buf = NULL;
        char buffer[1024];
        size_t bytes_read;
        size_t bytes_written = 0;
        while ((bytes_read = (size_t)read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            if (*buf == NULL)
                *buf = calloc(bytes_read + 1, 1);
            else
                *buf = realloc(*buf, bytes_written + bytes_read + 1);
            memcpy(*buf + bytes_written, buffer, bytes_read);
            bytes_written += bytes_read;
        }
        (*buf)[bytes_written - 1] = '\0';
        if (getenv_or_default(ENV_UQMI_DEBUG, (bool)false))
            fprintf(stderr, "UQMI_DEBUG_RX: %s\n", *buf);
    }

    close(pipefd[0]);

#ifdef HAVE_PIDFD
    siginfo_t info;
    waitid(P_PIDFD, (id_t)pidfd, &info, WEXITED);
    return info.si_status;
#else
    int pstatus = 0;
    waitpid(pid, &pstatus, 0);
    return pstatus;
#endif
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

    if (uqmi_execute_command(userdata, &client_id, argv) != 0 || client_id == NULL)
        return -1;

    const size_t n = (size_t)snprintf(NULL, 0, "uim,%s", client_id);
    userdata->client_id = calloc(n + 1, sizeof(char));
    if (userdata->client_id == NULL)
        return -1;
    snprintf(userdata->client_id, n + 1, "uim,%s", client_id);
    return 0;
}

static void apdu_interface_disconnect(struct euicc_ctx *ctx) {
    struct uqmi_userdata *userdata = ctx->apdu.interface->userdata;

    // clang-format off
    char *argv[] = {
        "--set-client-id", userdata->client_id,
        "--release-client-id", "uim",
        NULL,
    };
    // clang-format on

    uqmi_execute_command(userdata, NULL, argv);

    userdata->client_id = NULL;
}

static int apdu_interface_transmit(struct euicc_ctx *ctx, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx,
                                   const uint32_t tx_len) {
    const struct uqmi_userdata *userdata = ctx->apdu.interface->userdata;

    *rx = NULL;
    *rx_len = 0;

    char channel_str[16];
    snprintf(channel_str, sizeof(channel_str), "%d", ctx->apdu._internal.logic_channel);

    _cleanup_free_ char *tx_hex = calloc(tx_len * 2 + 1, sizeof(char));
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

    if (uqmi_execute_command(userdata, &buf, argv) == -1 || buf == NULL)
        return -1;

    _cleanup_cjson_ cJSON *jroot = cJSON_Parse(buf);
    if (jroot == NULL || !cJSON_IsObject(jroot)) {
        fprintf(stderr, "Failed to parse uqmi response\n\n%s\n", buf);
        return -1;
    }

    cJSON *jrx = cJSON_GetObjectItem(jroot, "response");
    if (jrx == NULL || !cJSON_IsString(jrx))
        return -1;

    const char *response = cJSON_GetStringValue(jrx);
    const uint32_t response_len = (uint32_t)strlen(response);

    *rx_len = response_len / 2;
    *rx = malloc(*rx_len);
    return euicc_hexutil_hex2bin_r(*rx, *rx_len, response, response_len);
}

static int apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, const uint8_t aid_len) {
    const struct uqmi_userdata *userdata = ctx->apdu.interface->userdata;

    if (userdata->client_id == NULL)
        return -1;

    _cleanup_free_ char *aid_hex = calloc((size_t)(aid_len * 2 + 1), sizeof(char));
    if (aid_hex == NULL)
        return -1;
    euicc_hexutil_bin2hex(aid_hex, (uint32_t)(aid_len * 2 + 1), aid, (uint32_t)aid_len);

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

    if (uqmi_execute_command(userdata, &buf, argv) == -1 || buf == NULL)
        return -1;

    _cleanup_cjson_ cJSON *jroot = cJSON_Parse(buf);
    if (jroot == NULL || !cJSON_IsObject(jroot)) {
        fprintf(stderr, "Failed to parse uqmi response\n\n%s\n", buf);
        return -1;
    }

    cJSON *jchannelid = cJSON_GetObjectItem(jroot, "channel_id");
    if (!jchannelid || !cJSON_IsNumber(jchannelid))
        return -1;

    return (int)cJSON_GetNumberValue(jchannelid);
}

static void apdu_interface_logic_channel_close(struct euicc_ctx *ctx, const uint8_t channel) {
    if (channel == 0)
        return;
    const struct uqmi_userdata *userdata = ctx->apdu.interface->userdata;

    const size_t n = (size_t)snprintf(NULL, 0, "%d", channel);
    _cleanup_free_ char *channel_str = calloc(n + 1, sizeof(char));
    if (channel_str == NULL)
        return;
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
    set_deprecated_env_name(ENV_QMI_DEVICE, "LPAC_QMI_DEV");
    set_deprecated_env_name(ENV_UQMI_DEBUG, "LPAC_QMI_DEBUG");

    const uint8_t uim_slot = (uint8_t)getenv_or_default(ENV_QMI_UIM_SLOT, (int)1);

    memset(ifstruct, 0, sizeof(struct euicc_apdu_interface));

    struct uqmi_userdata *userdata = malloc(sizeof(struct uqmi_userdata));
    userdata->program = (char *)getenv_or_default(ENV_UQMI_PROGRAM, "uqmi");
    userdata->device_path = (char *)getenv_or_default(ENV_QMI_DEVICE, "/dev/cdc-wdm0");
    if (access(userdata->device_path, F_OK) != 0) {
        fprintf(stderr, "qmi: device path '%s' does not exist.\n", userdata->device_path);
        return -1;
    }
    userdata->client_id = NULL;
    if (uim_slot > 0) {
        const size_t n = (size_t)snprintf(NULL, 0, "%d", uim_slot);
        userdata->uim_slot = calloc(n + 1, sizeof(char));
        if (userdata->uim_slot == NULL)
            return -1;
        snprintf(userdata->uim_slot, n + 1, "%d", uim_slot);
    } else {
        fprintf(stderr, "qmi: invalid %d uim slot\n", uim_slot);
        return -1;
    }

    ifstruct->connect = apdu_interface_connect;
    ifstruct->disconnect = apdu_interface_disconnect;
    ifstruct->logic_channel_open = apdu_interface_logic_channel_open;
    ifstruct->logic_channel_close = apdu_interface_logic_channel_close;
    ifstruct->transmit = apdu_interface_transmit;
    ifstruct->userdata = userdata;

    return 0;
}

static void libapduinterface_fini(struct euicc_apdu_interface *ifstruct) {
    struct uqmi_userdata *userdata = ifstruct->userdata;
    if (userdata == NULL)
        return;
    free(userdata);
}

const struct euicc_driver driver_if = {
    .type = DRIVER_APDU,
    .name = "uqmi",
    .init = (int (*)(void *))libapduinterface_init,
    .main = NULL,
    .fini = (void (*)(void *))libapduinterface_fini,
};
