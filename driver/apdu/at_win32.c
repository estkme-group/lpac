#include "at_common.h"

#include <cjson-ext/cJSON_ex.h>
#include <driver.h>
#include <euicc/hexutil.h>
#include <euicc/interface.h>
#include <lpac/utils.h>

#include <windows.h>
// windows.h MUST before other Windows headers
#include <devguid.h>
#include <initguid.h>
#include <inttypes.h>
#include <setupapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "setupapi.lib")

static HANDLE hComm;
static int logic_channel = 0;
static char *at_cmd_buffer;
static char at_read_buffer[AT_READ_BUFFER_SIZE];
static DWORD at_read_buffer_len = 0;

int starts_with(const char *str, const char *prefix) {
    size_t len_prefix = strlen(prefix);
    return strncmp(str, prefix, len_prefix) == 0;
}

static void enumerate_com_ports(cJSON *data) {
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA devInfoData;
    DWORD i;

    hDevInfo = SetupDiGetClassDevsA(&GUID_DEVCLASS_PORTS, 0, 0, DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE)
        return;

    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); i++) {
        char portName[256] = {0};
        char friendlyName[256] = {0};
        DWORD size = sizeof(portName);

        HKEY hKey = SetupDiOpenDevRegKey(hDevInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
        if (hKey != INVALID_HANDLE_VALUE) {
            DWORD type;
            if (RegQueryValueExA(hKey, "PortName", NULL, &type, (LPBYTE)portName, &size) != ERROR_SUCCESS
                || type != REG_SZ) {
                portName[0] = L'\0';
            }
        }

        if (!SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devInfoData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)friendlyName,
                                               sizeof(friendlyName), NULL)) {
            friendlyName[0] = L'\0';
        }

        if (starts_with(portName, "COM")) {
            cJSON *item = cJSON_CreateObject();
            if (item) {
                cJSON_AddStringToObject(item, "env", portName);
                cJSON_AddStringToObject(item, "name", friendlyName[0] ? friendlyName : portName);
                cJSON_AddItemToArray(data, item);
            } else {
                cJSON_Delete(item);
            }
        }
        RegCloseKey(hKey);
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
}

static int at_expect(char **response, const char *expected) {
    char line[AT_BUFFER_SIZE];
    DWORD bytes_read;
    char *found_response_data = NULL;
    int result = -1;

    if (response)
        *response = NULL;

    while (1) {
        char *newline = memchr(at_read_buffer, '\n', at_read_buffer_len);
        if (!newline) {
            if (at_read_buffer_len >= sizeof(at_read_buffer)) {
                fprintf(stderr, "AT response line too long or buffer full\n");
                at_read_buffer_len = 0;
                return -1;
            }
            if (!ReadFile(hComm, at_read_buffer + at_read_buffer_len, sizeof(at_read_buffer) - at_read_buffer_len,
                          &bytes_read, NULL)) {
                fprintf(stderr, "ReadFile error: %lu\n", GetLastError());
                return -1;
            }

            if (bytes_read == 0) {
                fprintf(stderr, "AT command timeout\n");
                return -1;
            }

            at_read_buffer_len += bytes_read;
            continue;
        }

        int line_len = (newline - at_read_buffer);
        memcpy(line, at_read_buffer, line_len);
        line[line_len] = '\0';

        memmove(at_read_buffer, newline + 1, at_read_buffer_len - line_len - 1);
        at_read_buffer_len -= (line_len + 1);

        line[strcspn(line, "\r")] = 0;

        if (strlen(line) == 0) {
            continue;
        }

        if (getenv_or_default(ENV_AT_DEBUG, (bool)false))
            fprintf(stderr, "AT_DEBUG_RX: %s\n", line);

        if (strcmp(line, "ERROR") == 0) {
            result = -1;
            goto end;
        } else if (strcmp(line, "OK") == 0) {
            result = 0;
            goto end;
        } else if (expected && strncmp(line, expected, strlen(expected)) == 0) {
            free(found_response_data);
            found_response_data = strdup(line + strlen(expected));
        }
    }

end:
    if (result == 0) {
        if (response) {
            *response = found_response_data;
        } else {
            free(found_response_data);
        }
    } else {
        free(found_response_data);
    }
    return result;
}

static int at_write_command(const char *cmd) {
    DWORD bytes_written;
    if (getenv_or_default(ENV_AT_DEBUG, (bool)false))
        fprintf(stderr, "AT_DEBUG_TX: %s", cmd);

    if (!WriteFile(hComm, cmd, strlen(cmd), &bytes_written, NULL)) {
        fprintf(stderr, "Failed to write to port, error: %lu\n", GetLastError());
        return -1;
    }
    return 0;
}

static int apdu_interface_connect(struct euicc_ctx *ctx) {
    const char *device = getenv_or_default(ENV_AT_DEVICE, "COM3");
    DCB dcb = {0};

    logic_channel = 0;

    char devname[64];
    snprintf(devname, sizeof(devname), "\\\\.\\%s", device);

    hComm = CreateFile(devname, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hComm == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Failed to open device: %s, error: %lu\n", devname, GetLastError());
        return -1;
    }

    PurgeComm(hComm, PURGE_TXCLEAR | PURGE_RXCLEAR);

    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(hComm, &dcb)) {
        fprintf(stderr, "GetCommState failed, error: %lu\n", GetLastError());
        CloseHandle(hComm);
        return -1;
    }

    if (at_write_command("AT\r\n") || at_expect(NULL, NULL)) {
        fprintf(stderr, "Device missing AT command support\n");
        CloseHandle(hComm);
        return -1;
    }
    if (at_write_command("AT+CCHO=?\r\n") || at_expect(NULL, NULL)) {
        fprintf(stderr, "Device missing AT+CCHO support\n");
        CloseHandle(hComm);
        return -1;
    }
    if (at_write_command("AT+CCHC=?\r\n") || at_expect(NULL, NULL)) {
        fprintf(stderr, "Device missing AT+CCHC support\n");
        CloseHandle(hComm);
        return -1;
    }
    if (at_write_command("AT+CGLA=?\r\n") || at_expect(NULL, NULL)) {
        fprintf(stderr, "Device missing AT+CGLA support\n");
        CloseHandle(hComm);
        return -1;
    }

    return 0;
}

static void apdu_interface_disconnect(struct euicc_ctx *ctx) {
    if (hComm != INVALID_HANDLE_VALUE) {
        CloseHandle(hComm);
        hComm = INVALID_HANDLE_VALUE;
    }
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

    size_t cmd_len = snprintf(at_cmd_buffer, AT_BUFFER_SIZE, "AT+CGLA=%d,%u,\"", logic_channel, tx_len * 2);
    char *p = at_cmd_buffer + cmd_len;
    for (uint32_t i = 0; i < tx_len; i++) {
        sprintf(p, "%02X", tx[i]);
        p += 2;
    }
    sprintf(p, "\"\r\n");

    if (at_write_command(at_cmd_buffer) || at_expect(&response, "+CGLA: ")) {
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
    _cleanup_free_ char *response = NULL;

    if (logic_channel) {
        return logic_channel;
    }

    for (int i = 1; i <= 4; i++) {
        snprintf(at_cmd_buffer, AT_BUFFER_SIZE, "AT+CCHC=%d\r\n", i);
        at_write_command(at_cmd_buffer);
        at_expect(NULL, NULL);
    }

    size_t cmd_len = snprintf(at_cmd_buffer, AT_BUFFER_SIZE, "AT+CCHO=\"");
    char *p = at_cmd_buffer + cmd_len;
    for (int i = 0; i < aid_len; i++) {
        sprintf(p, "%02X", aid[i]);
        p += 2;
    }
    sprintf(p, "\"\r\n");

    if (at_write_command(at_cmd_buffer) || at_expect(&response, "+CCHO: ")) {
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
    snprintf(at_cmd_buffer, AT_BUFFER_SIZE, "AT+CCHC=%d\r\n", logic_channel);
    at_write_command(at_cmd_buffer);
    at_expect(NULL, NULL);
    logic_channel = 0;
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

    at_cmd_buffer = malloc(AT_BUFFER_SIZE);
    if (!at_cmd_buffer) {
        fprintf(stderr, "Failed to allocate memory\n");
        return -1;
    }
    hComm = INVALID_HANDLE_VALUE;

    return 0;
}

static int libapduinterface_main(const struct euicc_apdu_interface *ifstruct, int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <list>\n", argv[0]);
        return -1;
    }

    if (strcmp(argv[1], "list") == 0) {
        cJSON *data = cJSON_CreateArray();

        enumerate_com_ports(data);

        jprint_enumerate_devices(data);

        return 0;
    }

    return 0;
}

static void libapduinterface_fini(struct euicc_apdu_interface *ifstruct) {
    free(at_cmd_buffer);
    if (hComm != INVALID_HANDLE_VALUE) {
        CloseHandle(hComm);
    }
}

const struct euicc_driver driver_apdu_at = {
    .type = DRIVER_APDU,
    .name = "at",
    .init = (int (*)(void *))libapduinterface_init,
    .main = (int (*)(void *, int, char **))libapduinterface_main,
    .fini = (void (*)(void *))libapduinterface_fini,
};
