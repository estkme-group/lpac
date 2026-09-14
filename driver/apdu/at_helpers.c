#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "at_helpers.h"

#include <lpac/utils.h>
#include <unistd.h>

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

static int at_line_is_decimal_channel(const char *line) {
    if (!line || !*line)
        return 0;
    for (; *line; line++) {
        if (!isdigit((unsigned char)*line))
            return 0;
    }
    return 1;
}

/* Fibocom FM350-GL answers AT+CCHO with a bare decimal channel line instead of the
 * standard "+CCHO: <id>". Accept either. */
int at_expect_ccho_channel(struct at_userdata *userdata, char **out) {
    return at_expect_with_deadline_ex(userdata, out, "+CCHO: ", at_line_is_decimal_channel, AT_RECOVERY_DEADLINE_MS);
}
