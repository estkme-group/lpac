#include "at_common.h"

#include "cjson/cJSON_ex.h"
#include "lpac/utils.h"

#include <driver.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int at_emit_command(struct at_userdata *userdata, const char *fmt, ...) {
    va_list args, args_length;
    va_start(args, fmt);
    va_copy(args_length, args);
    const int n = vsnprintf(NULL, 0, fmt, args_length);
    va_end(args_length);
    char formatted[n + 3];
    vsnprintf(formatted, n + 1, fmt, args);
    va_end(args);
    formatted[n + 0] = '\n';
    formatted[n + 1] = '\r';
    formatted[n + 2] = '\0';
    return at_write_command(userdata, formatted);
}

bool at_test_commands(struct at_userdata *userdata, const char **commands) {
    if (getenv_or_default(ENV_AT_TEST_CMD, (bool)false))
        return true;

    if (userdata == NULL || commands == NULL)
        return false;

    for (int i = 0; commands[i] != NULL; i++) {
        at_emit_command(userdata, "%s", commands[i]);
        if (at_expect(userdata, NULL, NULL) != 0) {
            fprintf(stderr, "Command '%s' failed\n", commands[i]);
            return false;
        }
    }
    return true;
}

void at_interface_disconnect(struct euicc_ctx *ctx) {
    struct at_userdata *userdata = ctx->apdu.interface->userdata;

    at_device_close(userdata);
}

int at_interface_main_entry(const struct euicc_apdu_interface *ifstruct, const int argc, char **argv) {
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

void at_interface_finished(struct euicc_apdu_interface *ifstruct) {
    struct at_userdata *userdata = ifstruct->userdata;

    at_cleanup_userdata(&userdata);
}
