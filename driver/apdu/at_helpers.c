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

// ETSI TS 127 007 defines the APDU payloads of AT+CSIM/AT+CGLA (and the AID of
// AT+CCHO) as "hexadecimal character format; refer +CSCS", and its "HEX" TE
// character set is defined — and every example given ("00 to FF", "032FE6") —
// with uppercase digits only. Most modems accept either case, but some modems
// (e.g. Sierra EM7565, Foxconn T99W696, Quectel RG502Q-EA) reject lowercase
// payloads with a bare ERROR. See docs/DEVICE-SPECIFIC-CAVEATS.md.
void at_apply_hex_case(char *hex) {
    static const char upper[] = "UPPER";
    const char *mode = getenv_or_default(ENV_AT_HEX, "LOWER");

    for (size_t i = 0; i < sizeof(upper); i++) {
        if (toupper((unsigned char)mode[i]) != upper[i]) {
            return;
        }
    }

    for (char *p = hex; *p != '\0'; p++) {
        if (*p >= 'a' && *p <= 'f') {
            *p -= 'a' - 'A';
        }
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
