#include "at_common.h"

#include <driver.h>
#include <euicc/hexutil.h>
#include <euicc/interface.h>
#include <lpac/utils.h>

#include <dirent.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static FILE *fuart;
static int logic_channel = 0;
static char *buffer;

static void enumerate_serial_device_linux(cJSON *data) {
    const char *dir_path = "/dev/serial/by-id";
    DIR *dir = opendir(dir_path);
    if (dir == NULL)
        return;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        _cleanup_free_ char *full_path = path_concat(dir_path, entry->d_name);

        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "env", full_path);
        cJSON_AddStringToObject(item, "name", entry->d_name);
        cJSON_AddItemToArray(data, item);
    }
    closedir(dir);
}

static int at_expect(char **response, const char *expected) {
    memset(buffer, 0, AT_BUFFER_SIZE);

    if (response)
        *response = NULL;

    while (1) {
        fgets(buffer, AT_BUFFER_SIZE, fuart);
        buffer[strcspn(buffer, "\r\n")] = 0;
        if (getenv_or_default(ENV_AT_DEBUG, (bool)false))
            printf("AT_DEBUG: %s\n", buffer);
        if (strcmp(buffer, "ERROR") == 0) {
            return -1;
        } else if (strcmp(buffer, "OK") == 0) {
            return 0;
        } else if (expected && strncmp(buffer, expected, strlen(expected)) == 0) {
            if (response)
                *response = strdup(buffer + strlen(expected));
        }
    }
    return 0;
}

static int apdu_interface_connect(struct euicc_ctx *ctx) {
    const char *device = getenv_or_default(ENV_AT_DEVICE, "/dev/ttyUSB0");

    logic_channel = 0;

    fuart = fopen(device, "r+");
    if (fuart == NULL) {
        fprintf(stderr, "Failed to open device: %s\n", device);
        return -1;
    }
    setbuf(fuart, NULL);

    fprintf(fuart, "AT+CCHO=?\r\n");
    if (at_expect(NULL, NULL)) {
        fprintf(stderr, "Device missing AT+CCHO support\n");
        return -1;
    }
    fprintf(fuart, "AT+CCHC=?\r\n");
    if (at_expect(NULL, NULL)) {
        fprintf(stderr, "Device missing AT+CCHC support\n");
        return -1;
    }
    fprintf(fuart, "AT+CGLA=?\r\n");
    if (at_expect(NULL, NULL)) {
        fprintf(stderr, "Device missing AT+CGLA support\n");
        return -1;
    }

    return 0;
}

static void apdu_interface_disconnect(struct euicc_ctx *ctx) {
    fclose(fuart);
    fuart = NULL;
    logic_channel = 0;
}

static int apdu_interface_transmit(struct euicc_ctx *ctx, uint8_t **rx, uint32_t *rx_len, const uint8_t *tx,
                                   uint32_t tx_len) {
    int fret = 0;
    int ret;
    _cleanup_free_ char *response = NULL;
    char *hexstr = NULL;

    *rx = NULL;
    *rx_len = 0;

    if (!logic_channel) {
        return -1;
    }

    fprintf(fuart, "AT+CGLA=%d,%u,\"", logic_channel, tx_len * 2);
    for (uint32_t i = 0; i < tx_len; i++) {
        fprintf(fuart, "%02X", (uint8_t)(tx[i] & 0xFF));
    }
    fprintf(fuart, "\"\r\n");
    if (at_expect(&response, "+CGLA:")) {
        goto err;
    }
    if (response == NULL) {
        goto err;
    }

    strtok(response, ",");
    hexstr = strtok(NULL, ",");
    if (!hexstr) {
        goto err;
    }
    if (hexstr[0] == '"') {
        hexstr++;
    }
    hexstr[strcspn(hexstr, "\"")] = '\0';

    *rx_len = strlen(hexstr) / 2;
    *rx = malloc(*rx_len);
    if (!*rx) {
        goto err;
    }

    ret = euicc_hexutil_hex2bin_r(*rx, *rx_len, hexstr, strlen(hexstr));
    if (ret < 0) {
        goto err;
    }
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

static int apdu_interface_logic_channel_open(struct euicc_ctx *ctx, const uint8_t *aid, uint8_t aid_len) {
    char *response;

    if (logic_channel) {
        return logic_channel;
    }

    for (int i = 1; i <= 4; i++) {
        fprintf(fuart, "AT+CCHC=%d\r\n", i);
        at_expect(NULL, NULL);
    }
    fprintf(fuart, "AT+CCHO=\"");
    for (int i = 0; i < aid_len; i++) {
        fprintf(fuart, "%02X", (uint8_t)(aid[i] & 0xFF));
    }
    fprintf(fuart, "\"\r\n");
    if (at_expect(&response, "+CCHO: ")) {
        return -1;
    }
    if (response == NULL) {
        return -1;
    }
    logic_channel = atoi(response);

    return logic_channel;
}

static void apdu_interface_logic_channel_close(struct euicc_ctx *ctx, uint8_t channel) {
    if (!logic_channel) {
        return;
    }
    fprintf(fuart, "AT+CCHC=%d\r\n", logic_channel);
    at_expect(NULL, NULL);
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

    buffer = malloc(AT_BUFFER_SIZE);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory\n");
        return -1;
    }

    return 0;
}

static int libapduinterface_main(const struct euicc_apdu_interface *ifstruct, int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <list>\n", argv[0]);
        return -1;
    }

    if (strcmp(argv[1], "list") == 0) {
        _cleanup_cjson_ cJSON *data = cJSON_CreateArray();

#ifdef __linux__
        enumerate_serial_device_linux(data);
#else
        fprintf(stderr, "Serial device enumeration not implemented on this platform.\n");
        fflush(stderr);
#endif

        jprint_enumerate_devices(data);
    }

    return 0;
}

static void libapduinterface_fini(struct euicc_apdu_interface *ifstruct) { free(buffer); }

const struct euicc_driver driver_if = {
    .type = DRIVER_APDU,
    .name = "at",
    .init = (int (*)(void *))libapduinterface_init,
    .main = (int (*)(void *, int, char **))libapduinterface_main,
    .fini = (void (*)(void *))libapduinterface_fini,
};
