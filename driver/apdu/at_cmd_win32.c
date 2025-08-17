#include "at_cmd.h"

#include <cjson/cJSON_ex.h>
#include <lpac/utils.h>

#include <windows.h>
// windows.h MUST before other Windows headers
#include <devguid.h>
#include <setupapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "setupapi.lib")

struct at_userdata {
    HANDLE hComm;
    char *at_cmd_buffer;
    char at_read_buffer[AT_READ_BUFFER_SIZE];
    DWORD at_read_buffer_len;
    char *default_device;
};

static int enumerate_com_ports(cJSON *devices) {
    SP_DEVINFO_DATA devInfoData;

    const HDEVINFO hDevInfo = SetupDiGetClassDevsA(&GUID_DEVCLASS_PORTS, 0, 0, DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE)
        return -1;

    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); i++) {
        char portName[256] = {0};
        char friendlyName[256] = {0};
        DWORD size = sizeof(portName);

        HKEY hKey = SetupDiOpenDevRegKey(hDevInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
        if (hKey != INVALID_HANDLE_VALUE) {
            DWORD type;
            const LONG ret = RegQueryValueExA(hKey, "PortName", NULL, &type, (LPBYTE)portName, &size);
            if (ret != ERROR_SUCCESS || type != REG_SZ)
                portName[0] = '\0';
            RegCloseKey(hKey);
        }

        const WINBOOL hasProperty = SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devInfoData, SPDRP_FRIENDLYNAME, NULL,
                                                                      (PBYTE)friendlyName, sizeof(friendlyName), NULL);
        if (!hasProperty)
            friendlyName[0] = '\0';

        if (strncmp(portName, "COM", 3) == 0) {
            cJSON *device = cJSON_CreateObject();
            if (device == NULL)
                continue;
            cJSON_AddStringToObject(device, "env", portName);
            cJSON_AddStringToObject(device, "name", friendlyName[0] ? friendlyName : portName);
            cJSON_AddItemToArray(devices, device);
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    return 0;
}

int (*enumerate_serial_device)(cJSON *) = enumerate_com_ports;

char *get_at_default_device(struct at_userdata *userdata) { return userdata->default_device; }

int at_write_command(struct at_userdata *userdata, const char *command) {
    if (WriteFile(userdata->hComm, command, strlen(command), NULL, NULL))
        return 0;
    fprintf(stderr, "Failed to write to port, error: %lu\n", GetLastError());
    return -1;
}

int at_expect(struct at_userdata *userdata, char **response, const char *expected) {
    char line[AT_BUFFER_SIZE];
    DWORD bytes_read;
    char *found_response_data = NULL;
    int result = -1;
    HANDLE hComm = userdata->hComm;
    char *at_read_buffer = userdata->at_read_buffer;
    DWORD *at_read_buffer_len_ptr = &userdata->at_read_buffer_len; // Use pointer for easier modification

    if (response)
        *response = NULL;

    while (true) {
        char *newline = memchr(at_read_buffer, '\n', *at_read_buffer_len_ptr);
        if (!newline) {
            if (*at_read_buffer_len_ptr >= sizeof(userdata->at_read_buffer)) {
                fprintf(stderr, "AT response line too long or buffer full\n");
                *at_read_buffer_len_ptr = 0;
                result = -1;
                goto end;
            }
            if (!ReadFile(hComm, at_read_buffer + *at_read_buffer_len_ptr,
                          sizeof(userdata->at_read_buffer) - *at_read_buffer_len_ptr, &bytes_read, NULL)) {
                fprintf(stderr, "ReadFile error: %lu\n", GetLastError());
                result = -1;
                goto end; // FIX: 错误路径也需要跳转到end进行清理
            }

            if (bytes_read == 0) {
                fprintf(stderr, "AT command timeout\n");
                result = -1;
                goto end;
            }

            *at_read_buffer_len_ptr += bytes_read;
            continue;
        }

        const int line_len = newline - at_read_buffer;
        memcpy(line, at_read_buffer, line_len);
        line[line_len] = '\0';

        memmove(at_read_buffer, newline + 1, *at_read_buffer_len_ptr - line_len - 1);
        *at_read_buffer_len_ptr -= line_len + 1;

        line[strcspn(line, "\r")] = 0;

        if (strlen(line) == 0) {
            continue;
        }

        if (getenv_or_default(ENV_AT_DEBUG, (bool)false))
            fprintf(stderr, "AT_DEBUG_RX: %s\n", line);

        if (strcmp(line, "ERROR") == 0) {
            result = -1;
            goto end;
        }
        if (strcmp(line, "OK") == 0) {
            result = 0;
            goto end;
        }
        if (expected && strncmp(line, expected, strlen(expected)) == 0) {
            free(found_response_data);
            found_response_data = strdup(line + strlen(expected));
        }
    }

end:
    if (result == 0 && response != NULL) {
        *response = found_response_data;
    } else {
        free(found_response_data);
    }
    return result;
}

int at_device_open(struct at_userdata *userdata, const char *device_name) {
    char dev_name[64];
    snprintf(dev_name, sizeof(dev_name), "\\\\.\\%s", device_name);

    userdata->hComm = CreateFile(dev_name,                     // lpFileName
                                 GENERIC_READ | GENERIC_WRITE, // dwDesiredAccess
                                 0,                            // dwShareMode
                                 NULL,                         // lpSecurityAttributes
                                 OPEN_EXISTING,                // dwCreationDisposition
                                 FILE_ATTRIBUTE_NORMAL,        // dwFlagsAndAttributes
                                 NULL);                        // hTemplateFile

    if (userdata->hComm == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Failed to open device: %s, error: %lu\n", dev_name, GetLastError());
        return -1;
    }

    PurgeComm(userdata->hComm, PURGE_TXCLEAR | PURGE_RXCLEAR);

    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(userdata->hComm, &dcb)) {
        fprintf(stderr, "GetCommState failed, error: %lu\n", GetLastError());
        CloseHandle(userdata->hComm);
        return -1;
    }

    return 0;
}

int at_device_close(struct at_userdata *userdata) {
    if (userdata->hComm != INVALID_HANDLE_VALUE) {
        CloseHandle(userdata->hComm);
        userdata->hComm = INVALID_HANDLE_VALUE;
    }
    if (userdata->at_cmd_buffer != NULL) {
        free(userdata->at_cmd_buffer);
        userdata->at_cmd_buffer = NULL;
    }
    memset(userdata->at_read_buffer, 0, sizeof(userdata->at_read_buffer));
    userdata->at_read_buffer_len = 0;
    return 0;
}

int at_setup_userdata(struct at_userdata **userdata) {
    if (userdata == NULL)
        return -1;
    *userdata = calloc(1, sizeof(struct at_userdata)); // 使用 calloc 初始化为0
    if (*userdata == NULL)
        return -1;
    (*userdata)->default_device = "COM3";
    (*userdata)->hComm = INVALID_HANDLE_VALUE;
    (*userdata)->at_cmd_buffer = calloc(AT_BUFFER_SIZE, 1);
    (*userdata)->at_read_buffer_len = 0;
    return 0;
}

void at_cleanup_userdata(struct at_userdata **userdata) {
    if (userdata == NULL || *userdata == NULL)
        return;
    at_device_close(*userdata);
    free(*userdata);
    *userdata = NULL;
}
