#include "applet.h"

#include <euicc/es10b.h>
#include <lpac/utils.h>

#include <errno.h>
#include <main.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int _delete_single(uint32_t seqNumber) {
    char str_seqNumber[11];
    int ret;

    snprintf(str_seqNumber, sizeof(str_seqNumber), "%u", seqNumber);

    jprint_progress("es10b_remove_notification_from_list", str_seqNumber);
    if ((ret = es10b_remove_notification_from_list(&euicc_ctx, seqNumber))) {
        const char *reason;
        switch (ret) {
        case 1:
            reason = "seqNumber not found";
            break;
        default:
            reason = "unknown";
            break;
        }
        jprint_error("es10b_remove_notification_from_list", reason);
        return -1;
    }

    return 0;
}

int applet_notification_remove_main(int argc, char **argv) {
    static const char *opt_string = "ah?";

    int fret = 0;
    int all = 0;
    int opt = 0;

    while ((opt = getopt(argc, argv, opt_string)) != -1) {
        switch (opt) {
        case 'a':
            all = 1;
            break;
        case 'h':
        case '?':
            printf("Usage: %s [OPTIONS] [seqNumber_0] [seqNumber_1]...\n", argv[0]);
            printf("\t -a All notifications\n");
            return -1;
        default:
            break;
        }
    }

    if (all) {
        _cleanup_es10b_notification_metadata_list_ struct es10b_notification_metadata_list *notifications, *rptr;

        jprint_progress("es10b_list_notification", NULL);
        if (es10b_list_notification(&euicc_ctx, &notifications)) {
            jprint_error("es10b_list_notification", NULL);
            return -1;
        }

        rptr = notifications;
        while (rptr) {
            if (_delete_single(rptr->seqNumber)) {
                fret = -1;
                break;
            }
            rptr = rptr->next;
        }
    } else {
        for (int i = optind; i < argc; i++) {
            unsigned long seqNumber;

            errno = 0;
            char *str_end;
            seqNumber = strtoul(argv[i], &str_end, 10);
            // Although POSIX said user should check errno instead of return value,
            // but errno may not be set when no conversion is performed according to C99.
            // Check nptr is same as str_end to ensure there is no conversion.
            if ((seqNumber == 0 && strcmp(argv[i], str_end)) || errno != 0) {
                continue;
            }

            if (_delete_single(seqNumber)) {
                fret = -1;
                break;
            }
        }
    }

    if (fret == 0) {
        jprint_success(NULL);
    }

    return fret;
}
