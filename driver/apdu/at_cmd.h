#pragma once

#include <cjson/cJSON.h>
#include <lpac/utils.h>

#include <inttypes.h>

#define AT_BUFFER_SIZE 20480

#define AT_MAX_LOGICAL_CHANNELS 20

/*
 * Pass as `deadline_ms` to wait for the response without a time limit.
 */
#define AT_DEADLINE_INFINITE (-1)
#define AT_RECOVERY_DEADLINE_MS 30000
#define AT_CGLA_DEADLINE_MS 240000
#define AT_PROBE_DEADLINE_MS 3000
#define AT_POLL_SLICE_MS 500

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

/*
 * Extracts the next complete line from `buf` into `line` and strips the trailing CR.
 * Returns 0 on success, -1 if no complete line is buffered yet.
 */
int at_pop_line(char *buf, size_t *buf_len, char *line, size_t line_size);

/*
 * Applies AT response bookkeeping (OK/ERROR detection, `expected` prefix capture)
 * to an already-extracted `line`.
 * Returns 0 once `result` is final, -1 if more lines are needed.
 */
int at_match_line(const char *line, const char *expected, char **found_response_data, int *result);

/*
 * Reads response lines until OK or ERROR, failing if `deadline_ms` passes first.
 * `deadline_ms` may be AT_DEADLINE_INFINITE.
 */
int at_expect_with_deadline(struct at_userdata *userdata, char **response, const char *expected, int deadline_ms);

/*
 * Same as at_expect_with_deadline() with AT_DEADLINE_INFINITE.
 */
int at_expect(struct at_userdata *userdata, char **response, const char *expected);

int at_device_open(struct at_userdata *userdata, const char *device_name);

int at_device_close(struct at_userdata *userdata);

int at_setup_userdata(struct at_userdata **userdata);

void at_cleanup_userdata(struct at_userdata **userdata);
