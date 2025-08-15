#include "process.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <main.h>
#include <stdlib.h>

#include <euicc/es10b.h>
#include <euicc/es10c.h>
#include <lpac/utils.h>

#include "helpers.h"
#include "euicc/es9p.h"

#ifdef _WIN32
ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
    if (!lineptr || !n || !stream) {
        return -1; // Invalid arguments
    }

    // Initial buffer size
    size_t initial_size = 128;
    if (*lineptr == NULL || *n == 0) {
        *n = initial_size;
        *lineptr = (char *) malloc(*n);
        if (*lineptr == NULL) {
            return -1; // Memory allocation failed
        }
    }

    size_t len = 0;
    int c;

    while ((c = fgetc(stream)) != EOF) {
        if (len + 1 >= *n) {
            // +1 for null terminator
            size_t new_size = *n * 2;
            char *new_ptr = (char *) realloc(*lineptr, new_size);
            if (new_ptr == NULL) {
                return -1; // Memory reallocation failed
            }
            *lineptr = new_ptr;
            *n = new_size;
        }

        (*lineptr)[len++] = (char) c;

        if (c == '\n') {
            break; // End of line
        }
    }

    if (c == EOF) {
        return -1; // No newline found and EOF reached
    }

    (*lineptr)[len] = '\0'; // Null-terminate the string
    return len;
}
#endif

static int handle_notification(const uint32_t seqNumber, const struct es10b_pending_notification notification) {
    char str_seqNumber[11];

    snprintf(str_seqNumber, sizeof(str_seqNumber), "%u", seqNumber);

    euicc_ctx.http.server_address = notification.notificationAddress;

    jprint_progress("es9p_handle_notification", str_seqNumber);
    if (es9p_handle_notification(&euicc_ctx, notification.b64_PendingNotification)) {
        jprint_error("es9p_handle_notification", NULL);
        return -1;
    }

    return 0;
}

static int applet_main(const int argc, char **argv) {
    if (isatty(fileno(stdin))) {
        jprint_error("This applet must be run with input redirection from a file or pipe.", NULL);
        return -1;
    }

    int fret = 0;
    char *input = NULL;
    char *eid = NULL;
    uint32_t seqNumber = 0;

    if (es10c_get_eid(&euicc_ctx, &eid) != 0) {
        jprint_error("es10c_get_eid", NULL);
        return -1;
    }

    size_t n;

    _cleanup_(es10b_pending_notification_free) struct es10b_pending_notification notification;

    while (getline(&input, &n, stdin) != -1) {
        _cleanup_cjson_ cJSON *jroot = cJSON_ParseWithLength(input, n);
        if (jroot == NULL) {
            jprint_error("cJSON_ParseWithLength", NULL);
            goto error;
        }
        if (parse_notification(jroot, eid, &seqNumber, &notification) != 0) {
            jprint_error("parse_notification", NULL);
            goto error;
        }
        if (handle_notification(seqNumber, notification) != 0) {
            jprint_error("handle_notification", NULL);
            goto error;
        }
    }

    jprint_success(NULL);

    goto exit;

error:
    fret = -1;
exit:
    return fret;
}

struct applet_entry applet_notification_replay = {
    .name = "replay",
    .main = applet_main,
};
