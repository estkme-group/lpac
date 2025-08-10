#include <iso646.h>

#include "process.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <main.h>
#include <stdlib.h>

#include <euicc/es10b.h>
#include <euicc/es10c.h>

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

    if (len == 0 && c == EOF) {
        return -1; // No characters read and EOF reached
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
    int fret = 0;
    char *input = NULL;
    char *eid = NULL;
    uint32_t seqNumber = 0;

    if (es10c_get_eid(&euicc_ctx, &eid) != 0) {
        jprint_error("es10c_get_eid", NULL);
        return -1;
    }

    cJSON *jroot;
    size_t n;

    struct es10b_pending_notification notification;

    while (getline(&input, &n, stdin) != -1) {
        jroot = cJSON_ParseWithLength(input, n);
        if (parse_notification(jroot, eid, &seqNumber, &notification) != 0) goto error;
        if (handle_notification(seqNumber, notification) != 0) goto error;
        cJSON_Delete(jroot);
    }

    es10b_pending_notification_free(&notification);

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
