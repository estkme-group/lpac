#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "at_helpers.h"

#include <lpac/utils.h>
#include <unistd.h>

#if !defined(_WIN32)
#include <errno.h>
#include <poll.h>
#endif

inline void at_warning_message(void) {
    static char *message =
        "WARNING: AT driver is for demo purposes only.\n"
        "WARNING: AT driver strictly complies with \"ETSI TS 127 007\" specification.\n"
        "WARNING: Some operations (e.g: download, delete, etc.), may fail due to insufficient response time.\n";

    if (isatty(fileno(stdin))) {
        fprintf(stderr, "\033[0;31m%s\033[0m", message);
    } else {
        fprintf(stderr, "%s", message);
    }
}

char *at_channel_get(struct at_userdata *userdata, const int index) {
    if (index <= 0 || index > AT_MAX_LOGICAL_CHANNELS)
        return NULL;

    char **channels = userdata->channels;
    return channels[index];
}

int at_channel_set(struct at_userdata *userdata, const int index, const char *identifier) {
    if (index <= 0 || index > AT_MAX_LOGICAL_CHANNELS)
        return -1;

    char **channels = userdata->channels;

    if (channels[index]) {
        free(channels[index]);
    }

    channels[index] = identifier ? strdup(identifier) : NULL;
    return 0;
}

int at_channel_next_id(struct at_userdata *userdata) {
    int index = 1;
    char **channels = userdata->channels;

    while (index <= AT_MAX_LOGICAL_CHANNELS && channels[index] != NULL)
        index++;

    if (index > AT_MAX_LOGICAL_CHANNELS)
        return -1;

    return index;
}

int at_emit_command(struct at_userdata *userdata, const char *fmt, ...) {
    va_list args, args_length;
    va_start(args, fmt);

    va_copy(args_length, args);
    const int n = vsnprintf(NULL, 0, fmt, args_length);
    va_end(args_length);

    _cleanup_free_ char *formatted = calloc(n + 2 /* CR+LF */ + 1, 1);
    if (formatted == NULL) {
        va_end(args);
        return -1;
    }

    vsnprintf(formatted, n + 1, fmt, args);
    va_end(args);

    formatted[n + 0] = '\r'; // CR
    formatted[n + 1] = '\n'; // LF
    formatted[n + 2] = '\0'; // NUL

    AT_DEBUG_TX(formatted);

    int ret = at_write_command(userdata, formatted);
    return ret;
}

void at_probe_capability_optional(struct at_userdata *userdata, const char *command) {
    at_emit_command(userdata, "%s=?", command);
    if (at_expect_with_deadline(userdata, NULL, NULL, AT_PROBE_DEADLINE_MS) != 0)
        fprintf(stderr, "AT capability probe %s: no response (continuing)\n", command);
}

static int at_line_is_decimal_channel(const char *s) {
    if (!s || !*s)
        return 0;
    for (; *s; s++) {
        if (*s < '0' || *s > '9')
            return 0;
    }
    return 1;
}

/* Accept standard "+CCHO: <id>" or a bare decimal channel line (Fibocom FM350). */
int at_expect_ccho_channel(struct at_userdata *userdata, char **out) {
#if defined(_WIN32)
    return at_expect_with_deadline(userdata, out, "+CCHO: ", AT_RECOVERY_DEADLINE_MS);
#else
    char line[AT_BUFFER_SIZE];
    _cleanup_free_ char *found_channel = NULL;
    int result = -1;
    const struct timespec start = get_current_clock(CLOCK_MONOTONIC);

    if (out)
        *out = NULL;

    while (1) {
        char *newline = memchr(userdata->at_read_buffer, '\n', userdata->at_read_buffer_len);
        if (!newline) {
            if (userdata->at_read_buffer_len >= AT_BUFFER_SIZE) {
                userdata->at_read_buffer_len = 0;
                goto end;
            }

            const struct timespec now = get_current_clock(CLOCK_MONOTONIC);
            const int elapsed =
                (int)((now.tv_sec - start.tv_sec) * 1000 + (now.tv_nsec - start.tv_nsec) / 1000000);
            if (elapsed >= AT_RECOVERY_DEADLINE_MS)
                goto end;

            struct pollfd pfd = {.fd = userdata->fd, .events = POLLIN};
            const int remain = AT_RECOVERY_DEADLINE_MS - elapsed;
            const int poll_ms = remain > AT_POLL_SLICE_MS ? AT_POLL_SLICE_MS : remain;
            if (poll(&pfd, 1, poll_ms) <= 0)
                continue;

            const ssize_t bytes_read =
                read(userdata->fd, userdata->at_read_buffer + userdata->at_read_buffer_len,
                     AT_BUFFER_SIZE - userdata->at_read_buffer_len);
            if (bytes_read <= 0)
                goto end;
            userdata->at_read_buffer_len += bytes_read;
            continue;
        }

        const size_t line_len = newline - userdata->at_read_buffer;
        if (line_len >= sizeof(line))
            goto end;
        memcpy(line, userdata->at_read_buffer, line_len);
        line[line_len] = '\0';

        memmove(userdata->at_read_buffer, newline + 1, userdata->at_read_buffer_len - line_len - 1);
        userdata->at_read_buffer_len -= line_len + 1;
        line[strcspn(line, "\r")] = 0;

        if (strlen(line) == 0)
            continue;

        AT_DEBUG_RX(line);

        if (strcmp(line, "ERROR") == 0)
            goto end;
        if (strcmp(line, "OK") == 0) {
            result = (found_channel != NULL) ? 0 : -1;
            goto end;
        }
        if (strncmp(line, "+CCHO: ", 8) == 0) {
            free(found_channel);
            found_channel = strdup(line + 8);
            while (found_channel && *found_channel == ' ')
                memmove(found_channel, found_channel + 1, strlen(found_channel));
        } else if (at_line_is_decimal_channel(line)) {
            free(found_channel);
            found_channel = strdup(line);
        }
    }

end:
    if (result == 0 && out) {
        *out = found_channel;
        found_channel = NULL;
    }
    return result;
#endif
}
