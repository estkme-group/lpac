#include "at_cmd.h"

#include <lpac/utils.h>

#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct at_userdata {
    char *default_device;
    FILE *fuart;
    char *buffer;

    char **channels;
};

static int enumerate_serial_device_for_linux(cJSON *devices) {
    const char *dir_path = "/dev/serial/by-id";
    DIR *dir = opendir(dir_path);
    if (dir == NULL)
        return -1;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        const size_t path_len = strlen(dir_path) + 1 /* SEP */ + strlen(entry->d_name) + 1 /* NUL */;
        _cleanup_free_ char *full_path = malloc(path_len);
        snprintf(full_path, path_len, "%s/%s", dir_path, entry->d_name);

        cJSON *device = cJSON_CreateObject();
        cJSON_AddStringToObject(device, "env", full_path);
        cJSON_AddStringToObject(device, "name", entry->d_name);
        cJSON_AddItemToArray(devices, device);
    }
    closedir(dir);
    return 0;
}

#ifdef __linux__
int (*enumerate_serial_device)(cJSON *) = enumerate_serial_device_for_linux;
#else
int (*enumerate_serial_device)(cJSON *) = NULL;
#endif

char *get_at_default_device(struct at_userdata *userdata) { return userdata->default_device; }

char **at_channels(struct at_userdata *userdata) { return userdata->channels; }

int at_write_command(struct at_userdata *userdata, const char *command) { return fputs(command, userdata->fuart); }

int at_expect(struct at_userdata *userdata, char **response, const char *expected) {
    FILE *fuart = userdata->fuart;
    char *buffer = userdata->buffer;

    memset(buffer, 0, AT_BUFFER_SIZE);

    if (response)
        *response = NULL;

    while (true) {
        if (fgets(buffer, AT_BUFFER_SIZE, fuart) == NULL) {
            return -1;
        }
        buffer[strcspn(buffer, "\r\n")] = 0;
        if (getenv_or_default(ENV_AT_DEBUG, (bool)false))
            printf("AT_DEBUG_RX: %s\n", buffer);

        if (strcmp(buffer, "ERROR") == 0)
            return -1;
        if (strcmp(buffer, "OK") == 0)
            return 0;

        if (expected != NULL && strncmp(buffer, expected, strlen(expected)) == 0) {
            if (response == NULL || *response != NULL)
                continue;
            *response = strdup(buffer + strlen(expected));
        }
    }
}

int at_device_open(struct at_userdata *userdata, const char *device_name) {
    if (userdata->buffer != NULL || userdata->fuart != NULL) {
        at_device_close(userdata);
    }

    userdata->buffer = calloc(AT_BUFFER_SIZE, 1);
    if (userdata->buffer == NULL) {
        fprintf(stderr, "Error allocating buffer\n");
        return -1;
    }
    userdata->fuart = fopen(device_name, "r+");
    if (userdata->fuart == NULL) {
        fprintf(stderr, "Failed to open device: %s\n", device_name);
        free(userdata->buffer);
        userdata->buffer = NULL;
        return -1;
    }
    setbuf(userdata->fuart, NULL);
    return 0;
}

int at_device_close(struct at_userdata *userdata) {
    if (userdata->fuart != NULL) {
        fclose(userdata->fuart);
        userdata->fuart = NULL;
    }
    if (userdata->buffer != NULL) {
        free(userdata->buffer);
        userdata->buffer = NULL;
    }
    return 0;
}

int at_setup_userdata(struct at_userdata **userdata) {
    if (userdata == NULL)
        return -1;
    *userdata = malloc(sizeof(struct at_userdata));
    if (*userdata == NULL)
        return -1;
    memset(*userdata, 0, sizeof(struct at_userdata));
    (*userdata)->default_device = "/dev/ttyUSB0";
    (*userdata)->fuart = NULL;
    (*userdata)->buffer = NULL;
    (*userdata)->channels = calloc(21, sizeof(char *));
    if ((*userdata)->channels == NULL)
        return -1;
    return 0;
}

void at_cleanup_userdata(struct at_userdata **userdata) {
    if (userdata == NULL || *userdata == NULL)
        return;
    at_device_close(*userdata);
    for (int index = 0; index < AT_MAX_LOGICAL_CHANNELS; index++)
        if ((*userdata)->channels[index] != NULL)
            free((*userdata)->channels[index]);
    free((*userdata)->channels);
    free(*userdata);
    *userdata = NULL;
}
