#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "at_helpers.h"
#include <lpac/utils.h>

char *at_channel_get(struct at_userdata *userdata, const int index) {
    if (index <= 0 || index > AT_MAX_LOGICAL_CHANNELS)
        return NULL;

    char **channels = at_channels(userdata);
    return channels[index];
}

int at_channel_set(struct at_userdata *userdata, const int index, const char *identifier) {
    if (index <= 0 || index > AT_MAX_LOGICAL_CHANNELS)
        return -1;

    char **channels = at_channels(userdata);

    if (channels[index]) {
        free(channels[index]);
    }

    channels[index] = identifier ? strdup(identifier) : NULL;
    return 0;
}

int at_channel_next_id(struct at_userdata *userdata) {
    int index = 1;
    char **channels = at_channels(userdata);

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

    char *formatted = calloc(n + 2 /* CR+LF */ + 1, 1);
    if (formatted == NULL) {
        va_end(args);
        return -1;
    }

    vsnprintf(formatted, n + 1, fmt, args);
    va_end(args);

    formatted[n + 0] = '\r'; // CR
    formatted[n + 1] = '\n'; // LF
    formatted[n + 2] = '\0'; // NUL

    if (getenv_or_default(ENV_AT_DEBUG, (bool)false))
        fprintf(stderr, "AT_DEBUG_TX: %s", formatted);

    int ret = at_write_command(userdata, formatted);
    free(formatted);
    return ret;
}
