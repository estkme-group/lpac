#include "at_cmd.h"

#include <cjson/cJSON.h>
#include <lpac/utils.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h> // Required for serial port configuration
#include <unistd.h>

struct at_userdata {
    char *default_device;
    int fd; // Use file descriptor instead of FILE*

    // A persistent buffer for reading data from the serial port
    char at_read_buffer[AT_BUFFER_SIZE];
    size_t at_read_buffer_len;

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
        char *full_path = malloc(path_len);
        snprintf(full_path, path_len, "%s/%s", dir_path, entry->d_name);

        cJSON *device = cJSON_CreateObject();
        cJSON_AddStringToObject(device, "env", full_path);
        cJSON_AddStringToObject(device, "name", entry->d_name);
        cJSON_AddItemToArray(devices, device);
        free(full_path);
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

int at_write_command(struct at_userdata *userdata, const char *command) {
    if (userdata->fd < 0) {
        return -1;
    }
    ssize_t bytes_written = write(userdata->fd, command, strlen(command));
    if (bytes_written < 0) {
        return -1;
    }
    return 0;
}

int at_expect(struct at_userdata *userdata, char **response, const char *expected) {
    char line[AT_BUFFER_SIZE];
    char *found_response_data = NULL;
    int result = -1;

    if (response)
        *response = NULL;

    while (1) {
        char *newline = memchr(userdata->at_read_buffer, '\n', userdata->at_read_buffer_len);
        if (!newline) {
            if (userdata->at_read_buffer_len >= AT_BUFFER_SIZE) {
                fprintf(stderr, "AT response line too long or buffer full\n");
                userdata->at_read_buffer_len = 0;
                goto end;
            }

            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(userdata->fd, &read_fds);

            int rv = select(userdata->fd + 1, &read_fds, NULL, NULL, NULL);
            if (rv < 0) {
                fprintf(stderr, "select error: %s\n", strerror(errno));
                goto end;
            } else if (rv == 0) {
                fprintf(stderr, "AT command timeout\n");
                goto end;
            }

            ssize_t bytes_read = read(userdata->fd, userdata->at_read_buffer + userdata->at_read_buffer_len,
                                      AT_BUFFER_SIZE - userdata->at_read_buffer_len);
            if (bytes_read <= 0) {
                fprintf(stderr, "read error or connection closed\n");
                goto end;
            }
            userdata->at_read_buffer_len += bytes_read;
            continue;
        }

        size_t line_len = newline - userdata->at_read_buffer;
        if (line_len >= sizeof(line))
            line_len = sizeof(line) - 1;
        memcpy(line, userdata->at_read_buffer, line_len);
        line[line_len] = '\0';

        memmove(userdata->at_read_buffer, newline + 1, userdata->at_read_buffer_len - line_len - 1);
        userdata->at_read_buffer_len -= line_len + 1;

        line[strcspn(line, "\r")] = 0;

        if (strlen(line) == 0)
            continue;

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
            while (*found_response_data == ' ')
                memmove(found_response_data, found_response_data + 1, strlen(found_response_data));
        }
    }
end:
    if (result == 0 && response) {
        *response = found_response_data;
    } else {
        free(found_response_data);
    }
    return result;
}

int at_device_open(struct at_userdata *userdata, const char *device_name) {
    if (userdata->fd >= 0) {
        at_device_close(userdata);
    }

    // Open serial port. O_NDELAY is obsolete, use O_NONBLOCK if needed,
    // but for this logic, blocking open is fine.
    userdata->fd = open(device_name, O_RDWR | O_NOCTTY);
    if (userdata->fd < 0) {
        fprintf(stderr, "Failed to open device: %s (%s)\n", device_name, strerror(errno));
        return -1;
    }

    // Add termios configuration for the serial port.
    struct termios tty;
    if (tcgetattr(userdata->fd, &tty) != 0) {
        fprintf(stderr, "Failed to get term attributes: %s\n", strerror(errno));
        close(userdata->fd);
        userdata->fd = -1;
        return -1;
    }

    // Set serial port to raw mode.
    // This is crucial for AT command communication.
    cfmakeraw(&tty);

    // Set other essential parameters.
    tty.c_cflag |= (CLOCAL | CREAD); // Ignore modem control lines, enable receiver.
    tty.c_cflag &= ~CSTOPB;          // 1 stop bit.
    tty.c_cflag &= ~CRTSCTS;         // No hardware flow control.

    // Set read behavior for select(). VMIN=0, VTIME=0 makes read() non-blocking.
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    // Flush port and apply settings.
    tcflush(userdata->fd, TCIOFLUSH);
    if (tcsetattr(userdata->fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "Failed to set term attributes: %s\n", strerror(errno));
        close(userdata->fd);
        userdata->fd = -1;
        return -1;
    }

    userdata->at_read_buffer_len = 0;
    return 0;
}

int at_device_close(struct at_userdata *userdata) {
    if (userdata->fd >= 0) {
        close(userdata->fd);
        userdata->fd = -1;
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
    (*userdata)->fd = -1;
    (*userdata)->channels = calloc(AT_MAX_LOGICAL_CHANNELS + 1, sizeof(char *));
    if ((*userdata)->channels == NULL) {
        free(*userdata);
        *userdata = NULL;
        return -1;
    }
    return 0;
}

void at_cleanup_userdata(struct at_userdata **userdata) {
    if (userdata == NULL || *userdata == NULL)
        return;

    at_device_close(*userdata);

    if ((*userdata)->channels) {
        for (int index = 0; index < AT_MAX_LOGICAL_CHANNELS; index++) {
            free((*userdata)->channels[index]);
        }
        free((*userdata)->channels);
    }

    free(*userdata);
    *userdata = NULL;
}
