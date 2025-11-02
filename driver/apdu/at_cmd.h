#pragma once

#include <cjson/cJSON.h>
#include <lpac/utils.h>

#include <inttypes.h>

#define AT_BUFFER_SIZE 20480

#define AT_MAX_LOGICAL_CHANNELS 20

#define ENV_AT_DEBUG APDU_ENV_NAME(AT, DEBUG)
#define ENV_AT_DEVICE APDU_ENV_NAME(AT, DEVICE)

struct at_userdata {
    char *default_device;
#if defined(_WIN32)
    HANDLE hComm;
#else
    int fd;
#endif

    char *at_cmd_buffer;
    // A persistent buffer for reading data from the serial port
    char at_read_buffer[AT_BUFFER_SIZE];
    size_t at_read_buffer_len;

    char **channels;

    struct timespec wall;
};

#define AT_DEBUG(direct, line)                                                                       \
    do {                                                                                             \
        if (getenv_or_default(ENV_AT_DEBUG, (bool)false)) {                                          \
            struct timespec t = get_wall_time(userdata->wall);                                       \
            fprintf(stderr, "AT_DEBUG_" direct "(%" PRId64 ".%" PRId64 "): %s\n", (int64_t)t.tv_sec, \
                    (int64_t)t.tv_nsec, line);                                                       \
        }                                                                                            \
    } while (0)
#define AT_DEBUG_TX(line) AT_DEBUG("TX", line)
#define AT_DEBUG_RX(line) AT_DEBUG("RX", line)

int enumerate_serial_device(cJSON *device);

int at_write_command(struct at_userdata *userdata, const char *command);

int at_expect(struct at_userdata *userdata, char **response, const char *expected);

int at_device_open(struct at_userdata *userdata, const char *device_name);

int at_device_close(struct at_userdata *userdata);

int at_setup_userdata(struct at_userdata **userdata);

void at_cleanup_userdata(struct at_userdata **userdata);
